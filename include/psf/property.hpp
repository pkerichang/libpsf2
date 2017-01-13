#ifndef LIBPSF_PROPERTY_H_
#define LIBPSF_PROPERTY_H_

/**
 *  This header file define methods to read property objects
 */

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

#include <boost/variant.hpp>
#include "psf/common.hpp"

namespace psf {

    typedef boost::variant<int32_t, double, std::string> PropValue;
    
    // A class that holds a key/value pair.
    class Property : public std::pair<std::string, PropValue> {
    public:
        Property() {}
        Property(std::string name, PropValue value);
        ~Property() {}

        static Property read(char *& data, bool& valid);
    };

    // container of properties as map for easy access.
    class PropDict : public std::unordered_map<std::string, PropValue> {
    public:
        PropDict() {}
        ~PropDict() {}
        
        static std::unique_ptr<PropDict> read(char *& data);
    };

    typedef std::unordered_map<std::string, std::unique_ptr<PropDict>> NestPropDict;    
}

    
#endif
