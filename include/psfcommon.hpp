#ifndef LIBPSF_COMMON_H_
#define LIBPSF_COMMON_H_

/**
 *  This header file define methods to read primitive types from binary file.
 */

#include <cstring>
#include <string>
#include <algorithm>
#include <fstream>

#define DEBUG 1

#ifdef DEBUG
#include <iostream>
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

namespace psf {

	static constexpr int32_t DOUB_SIZE = sizeof(uint64_t);
    static constexpr int32_t WORD_SIZE = sizeof(uint32_t);
	static constexpr int32_t BYTE_SIZE = sizeof(uint8_t);

    inline uint32_t read_uint32(std::ifstream& data) {
		char buf[WORD_SIZE];
		data.read(buf, WORD_SIZE);
		// convert from BE to LE
		return uint32_t((uint32_t(buf[0] & 255) << 24) |
			(uint32_t(buf[1] & 255) << 16) |
			(uint32_t(buf[2] & 255) << 8) |
			(uint32_t(buf[3] & 255)));
    }

	inline void undo_read_uint32(std::ifstream & data) {
		data.seekg(-WORD_SIZE, std::ios::cur);
	}

    inline int32_t read_int32(std::ifstream & data) {
        return static_cast<int32_t>(read_uint32(data));
    }

    inline int8_t read_int8(std::ifstream & data) {
		char buf[WORD_SIZE];
		data.read(buf, WORD_SIZE);
		uint8_t ans = *(reinterpret_cast<uint8_t*>(buf + WORD_SIZE - BYTE_SIZE));
		return static_cast<int8_t>(ans);
    }

    inline double read_double(std::ifstream & data) {
        char buf[DOUB_SIZE];
        data.read(buf, DOUB_SIZE);
        // convert from BE to LE
        uint64_t val = uint64_t((uint64_t((buf[0] & 255)) << 56) +
                                (uint64_t(buf[1] & 255) << 48) +
                                (uint64_t(buf[2] & 255) << 40) +
                                (uint64_t(buf[3] & 255) << 32) +
                                (uint64_t(buf[4] & 255) << 24) +
                                (uint64_t(buf[5] & 255) << 16) +
                                (uint64_t(buf[6] & 255) << 8) +
                                (uint64_t(buf[7] & 255)));
        double ans;
        // ans = *reinterpret_cast<double*>(&val);
        memcpy((void *)&ans, (void *)&val, sizeof(ans));
        return ans;
    }
    
    inline std::string read_str(std::ifstream & data) {
        uint32_t len = read_int32(data);
		// number of extra bytes to read to word-align the string length.
		uint32_t extras = ((len + 3) & ~0x00000003) - len;
		constexpr uint32_t buf_size = 100;
		char buf[buf_size];
		data.read(buf, std::min(len, buf_size));
		uint32_t num_read = static_cast<uint32_t>(data.gcount());
		std::string ans(buf, num_read);
		len -= num_read;
		while (len > 0) {
			data.read(buf, std::min(len, buf_size));
			num_read = static_cast<uint32_t>(data.gcount());
			ans.append(buf, num_read);
			len -= num_read;
		}
		// finish reading up to round len
		data.read(buf, extras);
        return ans;
    }
    
}

#endif
