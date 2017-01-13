#include "psfproperty.hpp"

using namespace psf;

Property::Property(std::string name, PropValue value) : std::pair<std::string, PropValue>(name, value) {}

bool Property::read(char *& data) {
    std::size_t num_read = 0;
    uint32_t code = peek_uint32(data, num_read);
    data += num_read;
    
    PropValue val;
    switch(code) {
    case 33 :
        first = read_str(data);
        val = read_str(data);
        second = val;
        DEBUG_MSG("Read property (" << first << ", " << val << ")");
        return true;
    case 34 :
        first = read_str(data);
        val = read_int32(data);
        second = val;
        DEBUG_MSG("Read property (" << first << ", " << val << ")");
        return true;
    case 35 :
        first = read_str(data);
        val = read_double(data);
        second = val;
        DEBUG_MSG("Read property (" << first << ", " << val << ")");
        return true;
    default :
        data -= num_read;
        DEBUG_MSG("Cannot parse property");
        return false;
    }
}

bool PropDict::read(char *& data) {
    bool valid = true;
    while (valid) {
        Property prop;
        valid = prop.read(data);
        if (valid) {
            insert(prop);
        }
    }
    return true;
}
