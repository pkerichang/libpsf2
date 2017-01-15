#include "psfproperty.hpp"
#include "psftypes.hpp"

using namespace psf;


/**
 * This function reads a TypeDefList from file.
 *
 * TypeDefList format:
 * int code1 = typedeflist_member_code
 * TypeDef type1
 * int code2 = typedeflist_member_code
 * TypeDef type2
 * ...
 *
 */
std::vector<int> read_type_list(std::ifstream & data, std::map<const uint32_t, TypeDef> * type_lookup) {

    std::vector<int> ans;
    bool valid_type = true;
    while (valid_type) {
        uint32_t code = read_uint32(data);
        if (code != TypeDef::tuple_code) {
            DEBUG_MSG("Read code = " << code << " != " <<
                      TypeDef::tuple_code << ", Stopping");
			undo_read_uint32(data);
            return ans;                
        }

        DEBUG_MSG("Reading element of TypeDef Tuple");
        TypeDef temp;
        valid_type = temp.read(data, type_lookup);
        if (valid_type) {
            ans.push_back(temp.m_id);
            type_lookup->emplace(temp.m_id, temp);
        }
    }
    return ans;
}

/**
 * This function reads a TypeDef object from file, then save it in
 * type lookup table.
 *
 * TypeDef format 1:
 * int code = typedef_code
 * int id
 * string name
 * int array_type
 * int data_type
 * PropEntry entry1
 * PropEntry entry2
 * ...
 *
 * TypeDef format 2 (with struct data type): 
 * int code = typedef_code
 * int id
 * string name
 * int array_type
 * int data_type = typedeflist_code
 * TypeDefList subtypes 
 * PropEntry entry1
 * PropEntry entry2
 * ...
 *
 */
bool TypeDef::read(std::ifstream & data, std::map<const uint32_t, TypeDef> * type_lookup) {
    uint32_t code = read_uint32(data);
    if (code != TypeDef::code) {
        DEBUG_MSG("Invalid TypeDef code " << code << ", expected " << TypeDef::code);
		undo_read_uint32(data);
        return false;
    }

    m_id = read_uint32(data);
    m_name = read_str(data);
    m_array_type = read_uint32(data);
    m_data_type = read_uint32(data);
    
    DEBUG_MSG("TypeDef = (" << m_id << ", " << m_name <<
              ", " << m_array_type << ", " << m_data_type << ")");

    if (m_data_type == TypeDef::struct_code) {
        // data type is a struct
        DEBUG_MSG("Reading TypeDef subtypes");
        m_subtypes = read_type_list(data, type_lookup);
    }
    
    // serialize properties
    DEBUG_MSG("Reading TypeDef Properties");
    m_prop_dict.read(data);
    type_lookup->emplace(m_id, *this);
    return true;
}

/**
 * Reads a scalar of this type from the given binary data.
 */
PSFScalar TypeDef::read_scalar(std::ifstream & data) const {
	PSFScalar ans;
	double re, im;
	switch (m_data_type) {
	case TypeDef::TYPEID_INT8 :
		ans = read_int8(data);
		return ans;
	case TypeDef::TYPEID_INT32 :
		ans = read_int32(data);
		return ans;
	case TypeDef::TYPEID_DOUBLE :
		ans = read_double(data);
		return ans;
	case TypeDef::TYPEID_COMPLEXDOUBLE :
		re = read_double(data);
		im = read_double(data);
		ans = std::complex<double>(re, im);
		return ans;
	default :
		// should never get here; we have checks before already.  Just return empty scalar. 
		return ans;
	}
}

/**
 * This function reads a Variable object from file.
 *
 * Variable format:
 * int code = typepointer_code
 * int id
 * string name
 * int type_id
 * PropEntry entry1
 * PropEntry entry2
 * ...
 *
 */
bool Variable::read(std::ifstream & data) {
    uint32_t code = read_uint32(data);
    if (code != Variable::code) {
        DEBUG_MSG("Invalid Variable code " << code << ", expected " << Variable::code);
		undo_read_uint32(data);
        return false;
    }

    m_id = read_uint32(data);
    m_name = read_str(data);
    m_type_id = read_uint32(data);
    
    DEBUG_MSG("Variable = (" << m_id << ", " << m_name << ", " << m_type_id << ")");
    
    // serialize properties
    DEBUG_MSG("Reading Variable Properties");
    m_prop_dict.read(data);

    return true;
}

/**
 * This function reads a Group object from file.
 *
 * Group format:
 * int code = group_code
 * int id
 * string name
 * int length
 * Variable pointer1
 * Variable pointer2
 * ...
 *
 */
bool Group::read(std::ifstream & data) {
    uint32_t code = read_uint32(data);
    if (code != Group::code) {
        DEBUG_MSG("Invalid Group code " << code << ", expected " << Group::code);
		undo_read_uint32(data);
        return false;
    }
    
    m_id = read_uint32(data);
    m_name = read_str(data);
    uint32_t len = read_uint32(data);
    
    DEBUG_MSG("Group = (" << m_id << ", " << m_name << ", " << len << ")");
    
    DEBUG_MSG("Reading Variable list");
    bool valid = true;
    for(uint32_t i = 0; i < len; i++) {
        Variable temp;
        valid = temp.read(data);
        if (valid) {
            m_vec.push_back(temp);
        } else {
            std::string msg = (boost::format("Group expects %d types, but only got %d") %
                               len % i).str();
            throw std::runtime_error(msg);
        }
    }
    
    return true;
}

