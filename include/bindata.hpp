#ifndef _PSFBINDATA
#define _PSFBINDATA

#include <boost/asio.hpp>

#define GET_I32(buf) ntohl(*(reinterpret_cast<const uint32_t*>(buf)))
// #define GET_I32(buf) *(reinterpret_cast<const uint32_t *>(buf))
#define GET_I64(buf) be64toh(*(reinterpret_cast<const uint64_t*>(buf)))
// #define GET_I64(buf) *(reinterpret_cast<const uint64_t *>(buf))


class PropSection {
 public:
    PropSection(uint32_t type_id, uint32_t end_marker) : m_type(type_id), m_end_marker(end_marker) {}
    ~PropSection() {}
    
    std::size_t deserialize(const char* buf, std::size_t offset, PropDict* pdict);

 private:
    uint32_t m_type;
    uint32_t m_end_marker;
};

    
#endif
