#include "writer.h"

#include <cerrno>

#include <logging/logging.h>
#include <clientserver/udaDefines.h>
#include <clientserver/streamCompression.h>

#include "createXDRStream.h"

#if !defined(__GNUC__)
#  include <io.h>
#  define read _read
#  define write _write
#endif

int serverSocket = 0;

#ifdef UDA_ZLIB_COMPRESSION
namespace
{

// Decompressed bytes received from the client that have not yet been consumed by the XDR layer
uda::compression::ReadBuffer g_ServerReadBuffer;

// Read exactly n bytes from the socket. Returns 1 on success, 0 on end of stream and -1 on error.
int read_exact(int fd, char* buf, std::size_t n)
{
    std::size_t received = 0;
    while (received < n) {
        int rc;
        while (((rc = (int)read(fd, buf + received, n - received)) == -1) && (errno == EINTR)) {}
        if (rc < 0) {
            return -1;
        }
        if (rc == 0) {
            return 0;
        }
        received += rc;
    }
    return 1;
}

} // namespace
#endif // UDA_ZLIB_COMPRESSION

void setSelectParms(int fd, fd_set* rfds, struct timeval* tv, int* server_tot_block_time)
{
    FD_ZERO(rfds);    // Initialise the File Descriptor set
    FD_SET(fd, rfds);    // Identify the Socket in the FD set
    tv->tv_sec = 0;
    tv->tv_usec = MIN_BLOCK_TIME;    // minimum wait microsecs (1ms)
    *server_tot_block_time = 0;
}

void updateSelectParms(int fd, fd_set* rfds, struct timeval* tv, int server_tot_block_time)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    if (server_tot_block_time < MAXBLOCK) {
        // (ms) For the First blocking period have rapid response (clientserver/udaDefines.h == 1000)
        tv->tv_sec = 0;
        tv->tv_usec = MIN_BLOCK_TIME;    // minimum wait (1ms)
    } else {
        tv->tv_sec = 0;
        tv->tv_usec = MAX_BLOCK_TIME;    // maximum wait (10ms)
    }
}

/*
//-----------------------------------------------------------------------------------------
// This routine is only called when the Server expects to Read something from the Client
//
// There are two time constraints:
//
//    The Maximum Blocking period is 1ms when reading
//    A Maximum number (MAXLOOP) of blocking periods is allowed before this time
//    is modified: It is extended to 100ms to minimise server resource consumption.
//
// When the Server is in a Holding state, it is listening to the Socket for either a
// Closedown or a Data request.
//
// Three Global variables are used to control the Blocking timeout
//
//    min_block_time
//    max_block_time
//    tot_block_time
//
// A Maximum time (MAXBLOCK) from the last Data Request is permitted before the Server Automatically
// closes down.
//-----------------------------------------------------------------------------------------
*/

int server_read(void* iohandle, char* buf, int count)
{
    fd_set rfds;        // File Descriptor Set for Reading from the Socket
    timeval tv = {};
    timeval tvc = {};

    auto io_data = reinterpret_cast<IoData*>(iohandle);

#ifdef UDA_ZLIB_COMPRESSION
    // Hand back any previously decompressed data before touching the socket

    if (!g_ServerReadBuffer.empty()) {
        return g_ServerReadBuffer.take(buf, count);
    }
#endif

    // Wait until there are data to be read from the socket

    setSelectParms(serverSocket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    while (select(serverSocket + 1, &rfds, nullptr, nullptr, &tvc) <= 0) {
        *io_data->server_tot_block_time += (int)tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = %d (ms)\n", *io_data->server_timeout * 1000);
            return -1;
        }

        updateSelectParms(serverSocket, &rfds, &tv, *io_data->server_tot_block_time);        // Keep trying ...
        tvc = tv;
    }

#ifdef UDA_ZLIB_COMPRESSION
    // Read a complete compressed frame, inflate it and return as much as the caller asked for

    unsigned char header[uda::compression::g_FrameHeaderSize];
    std::uint32_t compressed_size = 0;
    std::uint32_t original_size = 0;

    if (read_exact(serverSocket, reinterpret_cast<char*>(header), sizeof(header)) != 1) {
        return -1;
    }
    if (!uda::compression::parse_frame_header(header, compressed_size, original_size)) {
        UDA_LOG(UDA_LOG_DEBUG, "Invalid compressed frame header received\n");
        return -1;
    }

    std::vector<char> payload(compressed_size);
    if (read_exact(serverSocket, payload.data(), payload.size()) != 1) {
        return -1;
    }
    if (!uda::compression::decompress_frame(payload.data(), compressed_size, original_size,
                                            g_ServerReadBuffer.reset())) {
        g_ServerReadBuffer.clear();
        UDA_LOG(UDA_LOG_DEBUG, "Failed to decompress data received from client\n");
        return -1;
    }

    return g_ServerReadBuffer.take(buf, count);
#else
    int rc = 0;
    // Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int)read(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
    }

    return rc;
#endif // UDA_ZLIB_COMPRESSION
}

int server_write(void* iohandle, char* buf, int count)
{

    // This routine is only called when there is something to write back to the Client

    int rc = 0;
    int BytesSent = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    timeval tv = {};

    auto io_data = reinterpret_cast<IoData*>(iohandle);

#ifdef UDA_ZLIB_COMPRESSION
    // Compress the buffer into a single frame; this is what actually goes on the wire

    std::vector<char> frame;
    if (!uda::compression::compress_frame(buf, count, frame)) {
        UDA_LOG(UDA_LOG_DEBUG, "Failed to compress data for client\n");
        return -1;
    }
    const char* out = frame.data();
    int total = (int)frame.size();
#else
    const char* out = buf;
    int total = count;
#endif

    // Block IO until the Socket is ready to write to Client

    setSelectParms(serverSocket, &wfds, &tv, io_data->server_tot_block_time);

    while (select(serverSocket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
        *io_data->server_tot_block_time += tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: %d (ms)\n", *io_data->server_tot_block_time);
            return -1;
        }
        updateSelectParms(serverSocket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < total) {
        while (((rc = (int)write(serverSocket, out, total - BytesSent)) == -1) && (errno == EINTR)) {}
        if (rc < 0) {
            break;
        }
        BytesSent += rc;
        out += rc;
    }

#ifdef UDA_ZLIB_COMPRESSION
    // The XDR layer expects the number of (uncompressed) bytes it supplied to be returned on success

    return rc < 0 ? rc : count;
#else
    return rc;
#endif
}
