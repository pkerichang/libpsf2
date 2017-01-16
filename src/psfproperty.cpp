#include "psfproperty.hpp"

using namespace psf;


bool Property::read(std::ifstream & data) {
    uint32_t code = read_uint32(data);

    switch (code) {
    case 33:
        m_name = read_str(data);
        m_sval = read_str(data);
        m_type = type::STRING;
        LOG(TRACE) << "Read property (" << m_name << ", " << m_sval << ")";
        return true;
    case 34:
        m_name = read_str(data);
        m_ival = read_int32(data);
        m_type = type::INT;
        LOG(TRACE) << "Read property (" << m_name << ", " << m_ival << ")";
        return true;
    case 35:
        m_name = read_str(data);
        m_dval = read_double(data);
        m_type = type::DOUBLE;
        LOG(TRACE) << "Read property (" << m_name << ", " << m_dval << ")";
        return true;
    default:
        undo_read_uint32(data);
        LOG(TRACE) << "Cannot parse property";
        return false;
    }
}

bool PropDict::read(std::ifstream & data) {
    bool valid = true;
    while (valid) {
        Property prop;
        valid = prop.read(data);
        if (valid) {
            emplace(prop.m_name, prop);
        }
    }
    return true;
}
