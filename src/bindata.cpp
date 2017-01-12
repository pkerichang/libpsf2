
#include <cstdio>

#include "psf.hpp"
#include "bindata.hpp"


std::string deserialize_str(const char* start, std::size_t &num_read) {
    uint32_t len = GET_I32(start);
    // align to word boundary (4 bytes)
    num_read += sizeof(uint32_t) + len + ((sizeof(uint32_t) - len) & 3);
    return std::string(start + sizeof(uint32_t), len);
}

uint32_t deserialize_int(const char* start, std::size_t &num_read) {
    num_read += sizeof(uint32_t);
    return GET_I32(start);
}

uint64_t deserialize_double(const char* start, std::size_t &num_read) {
    num_read += sizeof(uint64_t);
    return GET_I64(start);
}


// Decodes a single name-value entry.
std::size_t deserialize_entry(const char* start, uint32_t end_marker, PropDict* pdict) {
    uint32_t type_id = GET_I32(start);
    std::size_t num_read = sizeof(uint32_t);

    // read name or end marker
    std::string name;
    PSFScalar val;
    switch(type_id) {
    case 33 :
        name = deserialize_str(start + num_read, num_read);
        // string type
        val = deserialize_str(start + num_read, num_read);
        pdict->emplace(name, val);
        return num_read;
    case 34 :
        name = deserialize_str(start + num_read, num_read);
        // int type
        val = static_cast<int>(deserialize_int(start + num_read, num_read));
        pdict->emplace(name, val);
        return num_read;
    case 35 :
        name = deserialize_str(start + num_read, num_read);
        // double type
        val = static_cast<double>(deserialize_double(start + num_read, num_read));
        pdict->emplace(name, val);
        return num_read;
    default :
        if (type_id == end_marker) {
            // end marker, nothing to read.
            return num_read;
        }
        // error
        char msg[100];
        snprintf(msg, 100, "Unrecognized Type ID: %d", type_id);
        throw std::runtime_error(msg);
    }
    
}

/**
 * header properties section format:
 * first word is the header section code, second word is the last index of the header section.
 * the last word of the header section has the value of m_end_marker.
 * the middle is a series of (type_word, name_data, value_data) bytes.
 * type_word is a single word that indicates the value type.
 * name_data is always a string, and value_data can be an int, a
 * double, or a string.
 * an int is always one word, a double is always two words, and a
 * string has one word encoding the length, followed by the
 * word-aligned (i.e. multiple of 4) char array data.
 */
std::size_t PropSection::deserialize(const char* start, std::size_t start_idx, PropDict* pdict) {
    uint32_t type_id = GET_I32(start);

    // check that ID is valid    
    if (type_id != m_type) {
        char msg[100];
        snprintf(msg, 100, "Type ID mismatch; expect %d, got %d", m_type, type_id);
        throw std::runtime_error(msg);
    }
    
    const char* cur = start + sizeof(uint32_t);
    
    uint32_t end_idx = GET_I32(cur);
    cur += sizeof(uint32_t);
    while(start_idx + (cur - start) < end_idx) {
        cur += deserialize_entry(cur, m_end_marker, pdict);
    }

    return cur - start;
}
