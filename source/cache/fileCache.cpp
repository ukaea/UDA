/*---------------------------------------------------------------
* UDA Client Data Cache
*----------------------------------------------------------------
Contents of DATA_BLOCK structures written to a local cache
Date/Time Stamp used to build in automatic obsolescence.
Unique hash key used to identify cached data using signal/source arguments
Cache located in directory given by environment variables UDA_CACHE_DIR, UDA_CACHE_TABLE
Maximum number of cached files prevents ...
Maximum size ...
Cache is communal: shared database table

Database table: 7 columns separated by a delimiting character ';'
Record 1 has table statistics: Record count, dead record count and eof offset.
The first record has a fixed length of 30 characters with a termination line feed

Columns: (each string is of arbitrary length)

unsigned int status
unsigned int Hash key
unsigned long long timestamp
char Filename[]
unsigned long long properties
char signal[]    - cannot contain a ';' character
char source[]
*/

#include "fileCache.h"

#ifdef _WIN32
DATA_BLOCK* udaFileCacheRead(const REQUEST_DATA* request, LOGMALLOCLIST* logmalloclist,
                             USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion,
                             LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    return nullptr;
}

int udaFileCacheWrite(const DATA_BLOCK* data_block, const REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                      USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                      unsigned int private_flags, int malloc_source)
{
    return 0;
}
#else

#  include "cache.h"

#  include <boost/algorithm/string.hpp>
#  include <boost/optional.hpp>
#  include <cerrno>
#  include <cstdlib>
#  include <fcntl.h>
#  include <string>
#  include <vector>

#  include <include/errorLog.h>
#  include <clientserver/stringUtils.h>
#  include <sstream>

constexpr int CACHE_MAXCOUNT = 100; // Max Attempts at obtaining a database table lock
constexpr int CACHE_HOURSVALID = 0;
constexpr int CACHE_MAXDEADRECORDS = 10;
constexpr int CACHE_MAXRECORDS = 1000; // Including comment records
constexpr int CACHE_STATSLENGTH = 30;
constexpr int CACHE_MAXLOCKTIME = 100; // Maximum Grace period after time expiry when a file record is locked

namespace
{

enum class LockActionType : short { READ = F_RDLCK, WRITE = F_WRLCK, UNLOCK = F_UNLCK };

struct CacheStats {
    unsigned long recordCount;
    unsigned long deadCount;
    long endOffset;
};

enum class EntryState {
    DEAD = 0,
    LIVE = 1,
    LOCKED = 2,
};

struct CacheEntry {
    size_t file_position = 0;
    EntryState state = EntryState::DEAD;
    std::string signal;
    std::string source;
    std::string filename;
    unsigned long long timestamp = 0;
    unsigned long long properties = 0;
    unsigned long dbkey = 0;
};

} // namespace

static int set_db_file_lock_state(FILE* db, LockActionType type);
static bool is_cache_time_valid(unsigned long long timestamp);
static bool is_cache_locked_time_valid(unsigned long long timestamp);
static bool is_cache_file_valid(const std::string& filename);
static boost::optional<CacheStats> get_cache_stats(FILE* db);
static int update_cache_stats(FILE* db, CacheStats stats);
static boost::optional<CacheStats> purge_cache(FILE* db);
static boost::optional<CacheEntry> find_cache_entry(const REQUEST_DATA* request);
static unsigned int xcrc32(const unsigned char* buf, int len, unsigned int init);
static int set_entry_state(const CacheEntry& entry, EntryState state);
static std::string get_file_path(const std::string& filename);

constexpr const char* delimiter = ";";

// Cache database table lock/unlock
// nullptr File handle returned if there is no cache

std::string get_file_path(const std::string& filename)
{
    const char* dir = getenv("UDA_CACHE_DIR");

    if (dir == nullptr) {
        return "";
    }

    return std::string{dir} + "/" + filename;
}

FILE* open_db_file(bool create)
{
    // Open the Database Table and apply an Exclusive lock
    const char* table = getenv("UDA_CACHE_TABLE"); // List of cached files
    if (table == nullptr) {
        return nullptr;
    }

    std::string dbfile = get_file_path(table);
    if (dbfile.empty()) {
        return nullptr;
    }

    // Open the Cache database table

    errno = 0;
    FILE* fh = fopen(dbfile.c_str(), "r+"); // ASCII file: Read with update (over-write)
    if (fh == nullptr && create) {
        errno = 0;
        fh = fopen(dbfile.c_str(), "w");
    }

    return fh;
}

int set_db_file_lock_state(FILE* db, LockActionType type)
{
    int fd = fileno(db);

    struct flock lock = {};
    lock.l_whence = SEEK_SET;               // Relative to the start of the file
    lock.l_start = 0;                       // Lock applies from this byte
    lock.l_len = 0;                         // To the end of the file
    lock.l_type = static_cast<short>(type); // Lock type to apply

    int rc = fcntl(fd, F_SETLK, &lock); // Manage the Cache table lock

    if (rc == 0) {
        // Lock managed OK
        return 0;
    }

    if (type == LockActionType::UNLOCK) {
        int err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, __func__, err, "cache file lock not released indicating problem with cache");
        return err;
    }

    // Problem obtaing an exclusive file lock - wait for a random time delay and try again
    int count = 0;
    do {
        int delay = 1 + (int)(10.0 * (rand() / (RAND_MAX + 1.0))); // Random delay [1,10]

        timespec requested = {};
        requested.tv_sec = 0;
        requested.tv_nsec = 1000 * delay; // Microsecs

        timespec remaining = {};
        nanosleep(&requested, &remaining);
        rc = fcntl(fd, F_SETLK, &lock);
    } while (rc == -1 && count++ < CACHE_MAXCOUNT);

    if (rc == -1 || count >= CACHE_MAXCOUNT) {
        int err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, __func__, err, "unable to lock the cache database");
        return err;
    }

    return 0; // Lock obtained
}

// Time stamp test

bool is_cache_time_valid(unsigned long long timestamp)
{
    if (timestamp == 0) {
        return true;
    }
    timeval current = {};
    gettimeofday(&current, nullptr);
    if (timestamp >= (unsigned long long)current.tv_sec) {
        // Timestamp OK
        return true;
    }
    return false;
}

// Check the locked file time is within a reasonable time period - protect against possible bugs in file management

bool is_cache_locked_time_valid(unsigned long long timestamp)
{
    timeval current = {};
    gettimeofday(&current, nullptr);
    if ((timestamp + CACHE_MAXLOCKTIME) >= (unsigned long long)current.tv_sec) {
        // Timestamp OK
        return true;
    }
    return false;
}

// Test the File exists

bool is_cache_file_valid(const std::string& filename)
{
    auto path = get_file_path(filename);
    return access(path.c_str(), F_OK) != -1;
}

// Current table statistics

boost::optional<CacheStats> get_cache_stats(FILE* db)
{
    rewind(db);
    char buffer[CACHE_STATSLENGTH + 1];
    if (fgets(buffer, CACHE_STATSLENGTH, db) == nullptr) {
        fseek(db, 0, SEEK_END);
        if (ftell(db) == 0) {
            update_cache_stats(db, {});
            return CacheStats{};
        }
        return {};
    }

    std::vector<std::string> tokens;
    boost::split(tokens, buffer, boost::is_any_of(delimiter));

    CacheStats stats = {};
    stats.recordCount = std::stoul(tokens[0]);
    stats.deadCount = std::stoul(tokens[1]);
    stats.endOffset = std::stol(tokens[2]);

    // If there are too many dead records, then compact the database table
    if (stats.deadCount >= CACHE_MAXDEADRECORDS) {
        auto maybe_updated_stats = purge_cache(db);
        if (!maybe_updated_stats) {
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "failed to purge cache");
            return {};
        }
        stats = maybe_updated_stats.get();
    }

    return stats;
}

/**
 * Update statistics
 * @param db cache file
 * @param stats
 * @return 0 on success, else error code
 */
int update_cache_stats(FILE* db, CacheStats stats)
{
    std::stringstream ss;
    ss << stats.recordCount << delimiter << stats.deadCount << delimiter << stats.endOffset;

    std::string stats_line = ss.str();

    size_t size = stats_line.size();
    if (size < CACHE_STATSLENGTH - 1) {
        rewind(db);
        fwrite(stats_line.c_str(), sizeof(char), size, db);
        for (size_t i = size; i < CACHE_STATSLENGTH - 1; i++) {
            // Pack remaining record with space chars
            fputc(' ', db);
        }
        fputc('\n', db);
    } else {
        UDA_THROW_ERROR(999, "invalid cache stats record");
    }

    fflush(db);
    return 0;
}

/**
 * Remove dead records to compact the database table
 * @param db
 * @return
 */
boost::optional<CacheStats> purge_cache(FILE* db)
{
    // rewind the file to the beginning of the records
    fseek(db, CACHE_STATSLENGTH, SEEK_SET);

    std::vector<std::pair<unsigned long long, std::string>> table;

    char* line = nullptr;
    size_t n = 0;
    while (getline(&line, &n, db) >= 0) {
        if (line[0] == '#') {
            // Preserve comments
            table.emplace_back(std::make_pair(0, line));
            continue;
        }

        if (line[0] == '\n') {
            continue;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter));

        unsigned short state = std::stoul(tokens[0]);
        unsigned long long timestamp = std::stoull(tokens[2]);
        std::string filename = tokens[3];

        if (state == static_cast<int>(EntryState::DEAD)) {
            std::string cache_file = get_file_path(filename);
            remove(cache_file.c_str());
            continue;
        }

        table.emplace_back(std::make_pair(timestamp, line));
    }

    using pair_type = std::pair<unsigned long long, std::string>;
    std::sort(table.begin(), table.end(), [](const pair_type& a, const pair_type& b) { return a.first < b.first; });

    while (table.size() > CACHE_MAXRECORDS) {
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(delimiter));

        std::string filename = tokens[3];
        std::string cache_file = get_file_path(filename);
        remove(cache_file.c_str());

        table.pop_back();
    }

    fseek(db, CACHE_STATSLENGTH, SEEK_SET); // Position at the start of records

    errno = 0;
    for (const auto& pair : table) {
        const auto& record = pair.second;
        size_t count = fwrite(record.c_str(), sizeof(char), record.size(), db);
        if (count != record.size() || errno != 0) {
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "Failed to write cache record");
            return {};
        }
    }

    CacheStats stats = {};
    stats.endOffset = ftell(db);
    stats.recordCount = table.size();
    stats.deadCount = 0;

    update_cache_stats(db, stats);

    return stats;
}

DATA_BLOCK* udaFileCacheRead(const REQUEST_DATA* request, LOGMALLOCLIST* logmalloclist,
                             USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion,
                             LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    auto maybe_entry = find_cache_entry(request);
    if (!maybe_entry) {
        return nullptr;
    }

    auto entry = maybe_entry.get();

    if (!is_cache_file_valid(entry.filename) ||
        (entry.state == EntryState::LIVE && !is_cache_time_valid(entry.timestamp)) ||
        (entry.state == EntryState::LOCKED && !is_cache_locked_time_valid(entry.timestamp))) {
        set_entry_state(entry, EntryState::DEAD);
        return nullptr;
    } else {
        set_entry_state(entry, EntryState::LOCKED);
    }

    std::string path = get_file_path(entry.filename);

    errno = 0;

    FILE* xdrfile;
    if ((xdrfile = fopen(path.c_str(), "rb")) == nullptr || errno != 0) {
        UDA_THROW_ERROR(0, "Unable to Open the Cached Data File");
    }

    auto data_block = readCacheData(xdrfile, logmalloclist, userdefinedtypelist, protocolVersion, log_struct_list,
                                    private_flags, malloc_source);

    fclose(xdrfile);

    set_entry_state(entry, EntryState::LIVE);

    return data_block;
}

boost::optional<CacheEntry> processLine(const std::string& line, size_t position)
{
    if (line[0] == '#' || line[0] == '\n') {
        return {};
    }

    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of(delimiter));

    CacheEntry entry;
    entry.file_position = position;

    int i = 0;
    for (const auto& token : tokens) {
        switch (i) {
            case 0:
                entry.state = static_cast<EntryState>(std::stoi(token));
                break;
            case 1:
                entry.dbkey = static_cast<unsigned int>(std::stoul(token));
                break;
            case 2:
                entry.timestamp = std::stoull(token);
                break;
            case 3:
                entry.filename = token;
                break;
            case 4:
                entry.properties = std::stoull(token);
                break;
            case 5:
                entry.signal = token;
                break;
            case 6:
                if (!token.empty() && token[token.size() - 1] == '\n') {
                    entry.source = token.substr(0, token.size() - 1);
                } else {
                    entry.source = token;
                }
                break;
            default:
                break;
        }
        if (entry.state == EntryState::DEAD) {
            // Don't need the remaining information on this record!
            break;
        }
        ++i;
    }

    return entry;
}

unsigned int generate_hash_key(const REQUEST_DATA* request)
{
    // Generate a Hash Key (not guaranteed unique)
    unsigned int key = 0;
    // combine CRC has keys
    key = xcrc32((const unsigned char*)request->signal, (int)strlen(request->signal), key);
    key = xcrc32((const unsigned char*)request->source, (int)strlen(request->source), key);
    return key;
}

int set_entry_state(const CacheEntry& entry, EntryState state)
{
    int rc = 0;
    FILE* db = open_db_file(false);
    if (db == nullptr || (rc = set_db_file_lock_state(db, LockActionType::WRITE)) != 0) {
        return rc;
    }

    fseek(db, entry.file_position, SEEK_SET);
    char buffer[3];
    snprintf(buffer, 3, "%d%c", static_cast<int>(state), delimiter[0]);
    fwrite(buffer, sizeof(char), 2, db);

    rc = set_db_file_lock_state(db, LockActionType::UNLOCK); // release lock
    fclose(db);

    return rc;
}

// Identify the name of the required cache file
boost::optional<CacheEntry> find_cache_entry(const REQUEST_DATA* request)
{
    // Lock the database
    FILE* db = open_db_file(false);
    if (db == nullptr || set_db_file_lock_state(db, LockActionType::READ) != 0) {
        return {};
    }

    unsigned int key = generate_hash_key(request);

    fseek(db, CACHE_STATSLENGTH, SEEK_SET); // Position at the start of record 2

    boost::optional<CacheEntry> found_entry;

    char* line = nullptr;
    size_t n = 0;
    size_t position = CACHE_STATSLENGTH;
    size_t len;
    while ((len = getline(&line, &n, db)) != static_cast<size_t>(-1)) {
        auto maybe_entry = processLine(line, position);
        position += len;
        if (!maybe_entry) {
            continue;
        }

        auto& entry = maybe_entry.get();
        if (entry.state == EntryState::DEAD) {
            continue;
        }

        if (key == entry.dbkey && (entry.signal == request->signal && entry.source == request->source)) {
            found_entry = entry;
            break;
        }
    }

    free(line);
    set_db_file_lock_state(db, LockActionType::UNLOCK);
    fclose(db);

    return found_entry;
}

int add_cache_record(const REQUEST_DATA* request, const char* filename)
{
    // Generate a Hash Key (not guaranteed unique)
    unsigned int key = 0;
    // combine CRC has keys
    key = xcrc32((const unsigned char*)request->signal, (int)strlen(request->signal), key);
    key = xcrc32((const unsigned char*)request->source, (int)strlen(request->source), key);

    // Generate a timestamp
    unsigned long long timestamp = 0;
    if (CACHE_HOURSVALID != 0) {
        timeval current = {};
        gettimeofday(&current, nullptr);
        timestamp = (unsigned long long)current.tv_sec + CACHE_HOURSVALID * 3600 + 60;
    }

    // Create the new record string
    std::stringstream ss;
    constexpr int properties = 0;
    ss << static_cast<int>(EntryState::LIVE) << delimiter << key << delimiter << timestamp << delimiter << filename
       << delimiter << properties << delimiter << request->signal << delimiter << request->source << '\n';
    std::string record = ss.str();

    // Append the new record to the database table
    int rc = 0;
    FILE* db = open_db_file(true);
    if (db == nullptr || (rc = set_db_file_lock_state(db, LockActionType::WRITE)) != 0) {
        UDA_THROW_ERROR(rc, "unable to get lock cache file");
    }

    // Current table statistics
    auto maybe_stats = get_cache_stats(db);
    if (!maybe_stats) {
        rc = set_db_file_lock_state(db, LockActionType::UNLOCK); // Free the Lock and File
        UDA_THROW_ERROR(rc, "unable to get cache stats");
    }
    CacheStats stats = maybe_stats.get();

    // Append a new record at the file position (always the end of the valid set of records)
    if (stats.endOffset == 0) {
        fseek(db, 0, SEEK_END);
        stats.endOffset = ftell(db);
    }

    fseek(db, stats.endOffset, SEEK_SET);
    fwrite(record.c_str(), sizeof(char), record.size(), db);
    stats.endOffset = ftell(db);

    fflush(db);
    stats.recordCount++;

    rc = update_cache_stats(db, stats);
    if (rc != 0) {
        UDA_THROW_ERROR(rc, "unable to update cache stats");
    }

    return set_db_file_lock_state(db, LockActionType::UNLOCK);
}

std::string generate_cache_filename(const REQUEST_DATA* request)
{
    unsigned int key = generate_hash_key(request);
    return std::string{"uda_"} + std::to_string(key) + ".cache";
}

int udaFileCacheWrite(const DATA_BLOCK* data_block, const REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                      USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                      unsigned int private_flags, int malloc_source)
{
    REQUEST_DATA* request = &request_block->requests[0];

    auto maybe_entry = find_cache_entry(request);
    if (maybe_entry) {
        // Entry already exists, do not add another
        return 0;
    }

    std::string filename = generate_cache_filename(request);
    std::string path = get_file_path(filename);

    FILE* xdrfile;
    errno = 0;
    if ((xdrfile = fopen(path.c_str(), "wb")) == nullptr || errno != 0) {
        UDA_THROW_ERROR(0, "unable to create the Cached Data File");
    }

    writeCacheData(xdrfile, logmalloclist, userdefinedtypelist, data_block, protocolVersion, log_struct_list,
                   private_flags, malloc_source);

    fclose(xdrfile);

    int rc = add_cache_record(request, filename.c_str());
    if (rc != 0) {
        UDA_THROW_ERROR(rc, "unable to add cache record");
    }

    return 0;
}

/*
//=========================================================================================
// Test the cache

   lock the cache database using fcntl (wait with random delay until lock is free if locked)
   query the table for the required data

   if the data is available from the cache
        test the timestamp
    if timestamp OK
        read the data
        free the lock
    else
        remove database entry
        remove cached file
        free the lock
        access the original data source
   else
    free the lock
    access the original data source

   if new data accessed
       lock the cache database
    add new record
    write new cache file
    free the lock
*/

//=========================================================================================
/* crc32.c
   Copyright (C) 2009, 2011 Free Software Foundation, Inc.

   This file is part of the libiberty library.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   In addition to the permissions in the GNU General Public License, the
   Free Software Foundation gives you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combined
   executable.)

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
*/

// #ifdef HAVE_CONFIG_H
// #include "Config.h"
// #endif

// #include "libiberty.h"

/* This table was generated by the following program.  This matches
   what gdb does.

   #include <cstdio>

   int
   main ()
   {
     unsigned int c;
     int table[256];

     for (int i = 0; i < 256; i++)
       {
     for (c = i << 24, j = 8; j > 0; --j)
       c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
     table[i] = c;
       }

     printf ("static const unsigned int crc32_table[] =\n{\n");
     for (int i = 0; i < 256; i += 4)
       {
     printf ("  0x%08x, 0x%08x, 0x%08x, 0x%08x",
         table[i + 0], table[i + 1], table[i + 2], table[i + 3]);
     if (i + 4 < 256)
       putchar (',');
     putchar ('\n');
       }
     printf ("};\n");
     return 0;
   }

   For more information on CRC, see, e.g.,
   http://www.ross.net/crc/download/crc_v3.txt.  */

static const unsigned int crc32_table[] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005, 0x2608edb8,
    0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f, 0x639b0da6,
    0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84,
    0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a,
    0xec7dd02d, 0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07,
    0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba, 0xaca5c697, 0xa864db20, 0xa527fdf9,
    0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b,
    0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c,
    0x774bb0eb, 0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b, 0x0315d626,
    0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8,
    0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a,
    0x8cf30bad, 0x81b02d74, 0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093,
    0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679,
    0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09, 0x8d79e0be, 0x803ac667,
    0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

/*

@deftypefn Extension {unsigned int} crc32 (const unsigned char *@var{buf}, @
  int @var{len}, unsigned int @var{init})

Compute the 32-bit CRC of @var{buf} which has length @var{len}.  The
starting value is @var{init}; this may be used to compute the CRC of
data split across multiple buffers by passing the return value of each
call as the @var{init} parameter of the next.

This is intended to match the CRC used by the @command{gdb} remote
protocol for the @samp{qCRC} command.  In order to get the same
results as gdb for a block of data, you must pass the first CRC
parameter as @code{0xffffffff}.

This CRC can be specified as:

  Width  : 32
  Poly   : 0x04c11db7
  Init   : parameter, typically 0xffffffff
  RefIn  : false
  RefOut : false
  XorOut : 0

This differs from the "standard" CRC-32 algorithm in that the values
are not reflected, and there is no final XOR value.  These differences
make it easy to compose the values of multiple blocks.

@end deftypefn

*/

unsigned int xcrc32(const unsigned char* buf, int len, unsigned int init)
{
    unsigned int crc = init;
    while (len--) {
        crc = (crc << 8u) ^ crc32_table[((crc >> 24u) ^ *buf) & 255u];
        buf++;
    }
    return crc;
}

#endif // _WIN32
