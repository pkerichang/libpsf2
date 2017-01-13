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
std::vector<int> read_type_list(char *& data, std::map<const uint32_t, TypeDef> * type_lookup) {

    std::vector<int> ans;
    bool valid_type = true;
    while (valid_type) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != TypeDef::tuple_code) {
            DEBUG_MSG("Read code = " << code << " != " <<
                      TypeDef::tuple_code << ", Stopping");
            return ans;                
        }
        data += num_read;
        
        DEBUG_MSG("Reading element of TypeDef Tuple");
        TypeDef temp;
        valid_type = temp.read(data, type_lookup);
        if (valid_type) {
            ans.push_back(temp.id());
            type_lookup->emplace(temp.id(), temp);
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
bool TypeDef::read(char *& data, std::map<const uint32_t, TypeDef> * type_lookup) {
    std::size_t num_read = 0;
    uint32_t code = peek_uint32(data, num_read);
    if (code != TypeDef::code) {
        DEBUG_MSG("Invalid TypeDef code " << code << ", expected " << TypeDef::code);
        return false;
    }
    data += num_read;
    
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
bool Variable::read(char *& data) {
    std::size_t num_read = 0;
    uint32_t code = peek_uint32(data, num_read);
    if (code != Variable::code) {
        DEBUG_MSG("Invalid Variable code " << code << ", expected " << Variable::code);
        return false;
    }
    data += num_read;
    
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
bool Group::read(char *& data) {
    std::size_t num_read = 0;
    uint32_t code = peek_uint32(data, num_read);
    if (code != Group::code) {
        DEBUG_MSG("Invalid Group code " << code << ", expected " << Group::code);
        return false;
    }
    data += num_read;
    
    m_id = read_uint32(data);
    m_name = read_str(data);
    uint32_t len = read_uint32(data);
    
    DEBUG_MSG("Group = (" << m_id << ", " << m_name << ", " << len << ")");
    
    DEBUG_MSG("Reading Variable list");
    bool valid = true;
    for( int i = 0; i < len; i++) {
        Variable temp;
        valid = temp.read(data);
        if (valid) {
            m_vec.push_back(temp);
        } else {
            std::string msg = (boost::format("Group expects %d types, but only got %d") %
                               len % i).str();
            throw new std::runtime_error(msg);
        }
    }
    
    return true;
}

/**
 * This function reads a Trace object from file, which is either a
 * Group or a Variable.
 */    
bool Trace::read(char *& data) {
    // don't increment, as either Group or Variable will
    // increment for us.
    std::size_t num_read;
    uint32_t code = peek_uint32(data, num_read);

    Group grp;
    Variable var;
    bool valid;
    switch(code) {
    case Group::code :
        valid = grp.read(data);
        if (valid) {
            boost::variant<Variable, Group>::operator=(grp);
        }
        return valid;
    case Variable::code :
        valid = var.read(data);
        if (valid) {
            boost::variant<Variable, Group>::operator=(var);
        }
        return valid;
    default :
        DEBUG_MSG("Cannot parse trace, invalid trace code " << code);
        return false;
    }
}
