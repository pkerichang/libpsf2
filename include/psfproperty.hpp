#ifndef LIBPSF_PROPERTY_H_
#define LIBPSF_PROPERTY_H_

/**
 *  This header file define methods to read property objects
 */

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "psfcommon.hpp"

namespace psf {
    
    // A class that holds a key/value pair.
    class Property {
    public:
        Property() : m_ival(0), m_dval(0.0), m_name(""), m_sval("") {}
        ~Property() {}

        bool read(std::ifstream & data);
		int m_ival;
		double m_dval;
		std::string m_name;
		std::string m_sval;
    };

    // container of properties as map for easy access.
    class PropDict : public std::unordered_map<std::string, Property> {
    public:
        PropDict() {}
        ~PropDict() {}
        
        bool read(std::ifstream & data);
    };

    typedef std::unordered_map<std::string, std::unique_ptr<PropDict>> NestPropDict;    
}

    
#endif
