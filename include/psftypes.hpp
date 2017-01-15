#ifndef LIBPSF_TYPEDEF_H_
#define LIBPSF_TYPEDEF_H_

/**
 *  This header file define methods to read variable and type objects
 */

#include <vector>
#include <complex>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <boost/variant.hpp>
#include <boost/format.hpp>

#include "psfcommon.hpp"

namespace psf {

	

	typedef boost::variant<int8_t, int32_t, double, std::complex<double>,
		std::string> PSFScalar;

    // a class representing a data type definition.
    class TypeDef {
    public:
		// mapping from data type to m_data_type code.
		static constexpr uint32_t TYPEID_INT8 = 1;
		static constexpr uint32_t TYPEID_STRING = 2;
		static constexpr uint32_t TYPEID_ARRAY = 3;
		static constexpr uint32_t TYPEID_INT32 = 5;
		static constexpr uint32_t TYPEID_DOUBLE = 11;
		static constexpr uint32_t TYPEID_COMPLEXDOUBLE = 12;
		static constexpr uint32_t TYPEID_STRUCT = 16;

        static constexpr uint32_t code = 16;
        static constexpr uint32_t struct_code = 16;
        static constexpr uint32_t tuple_code = 18;

        TypeDef() {}
        ~TypeDef() {}

        bool read(std::ifstream & data, std::map<const uint32_t, TypeDef> * type_lookup);
		
		bool is_scalar_type() const {
			switch (m_data_type) {
			case TypeDef::TYPEID_INT8:
			case TypeDef::TYPEID_INT32:
			case TypeDef::TYPEID_DOUBLE:
			case TypeDef::TYPEID_COMPLEXDOUBLE:
				return true;
			default:
				return false;
			}
		}

		PSFScalar read_scalar(std::ifstream & data) const;

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
        static constexpr uint32_t code = 16;
        
        Variable() {}
        ~Variable() {}

        bool read(std::ifstream & data);

        uint32_t m_id;
        std::string m_name;
        uint32_t m_type_id;
        PropDict m_prop_dict;
    };

    typedef std::vector<Variable> VarList;
    
    // a collection of Variables.
    class Group {
    public:
        static constexpr uint32_t code = 17;

        Group() {}
        ~Group() {}
        
        bool read(std::ifstream & data);

        uint32_t m_id;
        std::string m_name;
        VarList m_vec;
    };
    
}

#endif
