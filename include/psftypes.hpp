#ifndef LIBPSF_TYPEDEF_H_
#define LIBPSF_TYPEDEF_H_

/**
 *  This header file define methods to read property objects
 */

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <boost/variant.hpp>
#include <boost/format.hpp>

#include "psfcommon.hpp"

namespace psf {

    // a class representing a data type definition.
    class TypeDef {
    public:
        static const uint32_t code = 16;
        static const uint32_t struct_code = 16;
        static const uint32_t tuple_code = 18;

        TypeDef() {}
        ~TypeDef() {}

        bool read(char *& data, std::map<const uint32_t, TypeDef> * type_lookup);

        uint32_t id() { return m_id; }
        
    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_array_type;
        uint32_t m_data_type;
        std::vector<int> m_subtypes;
        PropDict m_prop_dict;
    };

    typedef std::map<const uint32_t, TypeDef> TypeMap;
    
    // a class referencing a defined type
    class Variable {
    public:
        static const uint32_t code = 16;
        
        Variable() {}
        ~Variable() {}

        bool read(char *& data);

    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_type_id;
        PropDict m_prop_dict;
    };

    typedef std::vector<Variable> VarList;
    
    // a collection of Variables.
    class Group {
    public:
        static const uint32_t code = 17;

        Group() {}
        ~Group() {}
        
        bool read(char *& data);

    private:
        uint32_t m_id;
        std::string m_name;
        VarList m_vec;
    };

    class Trace : public boost::variant<Variable, Group> {
    public:
        Trace() {}
        ~Trace() {}

        bool read(char *& data);
    };

    typedef std::vector<Trace> TraceList;
    
}

#endif
