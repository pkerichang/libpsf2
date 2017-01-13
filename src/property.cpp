#include "psf/property.hpp"

using namespace psf;

Property::Property(std::string name, PropValue value) : std::pair<std::string, PropValue>(name, value) {}

Property Property::read(char *& data, bool& valid) {
    std::size_t num_read = 0;
    uint32_t code = peek_uint32(data, num_read);
    data += num_read;
    
    valid = true;
    std::string name;
    PropValue val;
    switch(code) {
    case 33 :
        name = read_str(data);
        val = read_str(data);
        DEBUG_MSG("Read property (" << name << ", " << val << ")");
        return Property(name, val);
    case 34 :
        name = read_str(data);
        val = read_int32(data);
        DEBUG_MSG("Read property (" << name << ", " << val << ")");
        return Property(name, val);
    case 35 :
        name = read_str(data);
        val = read_double(data);
        DEBUG_MSG("Read property (" << name << ", " << val << ")");
        return Property(name, val);
    default :
        valid = false;
        data -= num_read;
        DEBUG_MSG("Cannot parse property");
        return Property();
    }
}

std::unique_ptr<PropDict> PropDict::read(char *& data) {
    auto ans = std::unique_ptr<PropDict>(new PropDict());

    bool valid = true;
    while (valid) {
        Property prop = Property::read(data, valid);
        if (valid) {
            ans->insert(prop);
        }
    }
    return ans;
}

