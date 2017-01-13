#ifndef LIBPSF_PRIMITIVES_H_
#define LIBPSF_PRIMITIVES_H_

/**
 *  This header file define methods to read primitive types from file.
 */

#include <string>
#include <boost/asio.hpp>

#define DEBUG 1

#ifdef DEBUG
#include <iostream>
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

namespace psf {

    static const uint32_t WORD_SIZE = sizeof(uint32_t);
    
    inline uint32_t read_uint32(char *& data) {
        uint32_t ans = ntohl(*(reinterpret_cast<uint32_t*>(data)));
        data += WORD_SIZE;
        return ans;
    }
    
    inline uint32_t peek_uint32(const char * data, std::size_t& num_read) {
        num_read += WORD_SIZE;
        return ntohl(*(reinterpret_cast<const uint32_t*>(data)));
    }

    inline int32_t read_int32(char *& data) {
        return static_cast<int32_t>(read_uint32(data));
    }

    inline int8_t read_int8(char *& data) {
        return static_cast<int8_t>(read_uint32(data));
    }
    
    inline double read_double(char *& data) {
        uint64_t ans = be64toh(*(reinterpret_cast<uint64_t*>(data)));
        data += sizeof(uint64_t);
        return *reinterpret_cast<double*>(&ans);
    }
    
    inline std::string read_str(char *& data) {
        uint32_t len = read_int32(data);
        std::string ans(data, len);
        // align to word boundary (4 bytes)
        data += (len + 3) & ~0x03;
        return ans;
    }
    
}

#endif