#include "psfproperty.hpp"

using namespace psf;

Property::Property(std::string name, PropValue value) : std::pair<std::string, PropValue>(name, value) {}

bool Property::read(std::ifstream & data) {
    uint32_t code = read_uint32(data);
    
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
		undo_read_uint32(data);
        DEBUG_MSG("Cannot parse property");
        return false;
    }
}

bool PropDict::read(std::ifstream & data) {
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
