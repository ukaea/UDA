#ifndef UDA_CLIENTSERVER_STREAMCOMPRESSION_H
#define UDA_CLIENTSERVER_STREAMCOMPRESSION_H

// Optional zlib compression of the byte stream passed between the UDA client and server.
//
// Enabled by defining UDA_ZLIB_COMPRESSION (CMake option ENABLE_ZLIB_COMPRESSION). The client and server must both
// be built with the same setting as the wire format changes.
//
// Wire format: every buffer handed to the socket write function is sent as one frame:
//
//     uint32 (big endian)  compressed payload length
//     uint32 (big endian)  original (uncompressed) length
//     payload              zlib compressed bytes
//
// The reader decompresses a whole frame at a time and hands the bytes back to the XDR layer in whatever chunk sizes it
// asks for, so the XDR record stream does not need to know about the framing.

#ifdef UDA_ZLIB_COMPRESSION

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <zlib.h>

namespace uda
{
namespace compression
{

constexpr std::size_t g_FrameHeaderSize = 8;
constexpr std::uint32_t g_MaxFrameSize = 1u << 28; // Sanity limit (256MB) to protect against corrupt headers

inline void put_u32(unsigned char* p, std::uint32_t v)
{
    p[0] = static_cast<unsigned char>(v >> 24);
    p[1] = static_cast<unsigned char>(v >> 16);
    p[2] = static_cast<unsigned char>(v >> 8);
    p[3] = static_cast<unsigned char>(v);
}

inline std::uint32_t get_u32(const unsigned char* p)
{
    return (static_cast<std::uint32_t>(p[0]) << 24) | (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}

// Compress count bytes from data into a complete frame (header + payload). Returns false on failure.
inline bool compress_frame(const char* data, int count, std::vector<char>& frame)
{
    if (count <= 0 || static_cast<std::uint32_t>(count) > g_MaxFrameSize) {
        return false;
    }

    uLongf bound = compressBound(static_cast<uLong>(count));
    frame.resize(g_FrameHeaderSize + bound);

    uLongf compressed_size = bound;
    int rc = compress2(reinterpret_cast<Bytef*>(frame.data() + g_FrameHeaderSize), &compressed_size,
                       reinterpret_cast<const Bytef*>(data), static_cast<uLong>(count), Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) {
        return false;
    }

    frame.resize(g_FrameHeaderSize + compressed_size);
    put_u32(reinterpret_cast<unsigned char*>(frame.data()), static_cast<std::uint32_t>(compressed_size));
    put_u32(reinterpret_cast<unsigned char*>(frame.data()) + 4, static_cast<std::uint32_t>(count));
    return true;
}

// Decode and validate a frame header. Returns false if the header is not plausible.
inline bool parse_frame_header(const unsigned char* header, std::uint32_t& compressed_size,
                               std::uint32_t& original_size)
{
    compressed_size = get_u32(header);
    original_size = get_u32(header + 4);
    return compressed_size > 0 && original_size > 0 && original_size <= g_MaxFrameSize &&
           compressed_size <= compressBound(original_size);
}

// Inflate a frame payload into out. Returns false on failure or if the size does not match the header.
inline bool decompress_frame(const char* payload, std::uint32_t compressed_size, std::uint32_t original_size,
                             std::vector<char>& out)
{
    out.resize(original_size);
    uLongf dest_len = original_size;
    int rc = uncompress(reinterpret_cast<Bytef*>(out.data()), &dest_len, reinterpret_cast<const Bytef*>(payload),
                        compressed_size);
    return rc == Z_OK && dest_len == original_size;
}

// Holds decompressed bytes that have been read from the socket but not yet consumed by the XDR layer.
class ReadBuffer
{
  public:
    bool empty() const { return _pos >= _data.size(); }

    void clear()
    {
        _data.clear();
        _pos = 0;
    }

    // Discard any existing contents and return the vector so that it can be refilled.
    std::vector<char>& reset()
    {
        clear();
        return _data;
    }

    // Copy up to count bytes into buf and return the number copied.
    int take(char* buf, int count)
    {
        std::size_t n = std::min(_data.size() - _pos, static_cast<std::size_t>(count));
        std::memcpy(buf, _data.data() + _pos, n);
        _pos += n;
        return static_cast<int>(n);
    }

  private:
    std::vector<char> _data;
    std::size_t _pos = 0;
};

} // namespace compression
} // namespace uda

#endif // UDA_ZLIB_COMPRESSION

#endif // UDA_CLIENTSERVER_STREAMCOMPRESSION_H
