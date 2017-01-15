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

	m_is_supported = true;
	// construct HDF5 data type
	H5::CompType comp_read_type;
	H5::CompType comp_write_type;
	std::vector<int> subtypes;
	hsize_t read_size = 0, write_size = 0;
	std::ostringstream strbuilder;
	m_read_offset = 0;
	m_read_stride = 1;
	std::string rname("r"), iname("i");
	switch (m_data_type) {
	case TypeDef::TYPEID_INT8 :
		m_h5_read_type = H5::PredType::STD_I8BE;
		m_h5_write_type = H5::PredType::STD_I8LE;
		m_read_offset = WORD_SIZE - BYTE_SIZE;
		m_read_stride = WORD_SIZE;
		m_type_name = "int8";
		break;
	case TypeDef::TYPEID_INT32 :
		m_h5_read_type = H5::PredType::STD_I32BE;
		m_h5_write_type = H5::PredType::STD_I32LE;
		m_type_name = "int32";
		break;
	case TypeDef::TYPEID_DOUBLE :
		m_h5_read_type = H5::PredType::IEEE_F64BE;
		m_h5_write_type = H5::PredType::IEEE_F64LE;
		m_type_name = "double";
		break;
	case TypeDef::TYPEID_COMPLEXDOUBLE :
		comp_read_type = H5::CompType(2 * sizeof(double));
		comp_read_type.insertMember(rname, 0, H5::PredType::IEEE_F64BE);
		comp_read_type.insertMember(iname, sizeof(double), H5::PredType::IEEE_F64BE);
		comp_write_type = H5::CompType(2 * sizeof(double));
		comp_write_type.insertMember(rname, 0, H5::PredType::IEEE_F64LE);
		comp_write_type.insertMember(iname, sizeof(double), H5::PredType::IEEE_F64LE);

		m_h5_read_type = comp_read_type;
		m_h5_write_type = comp_write_type;
		m_type_name = "complex";
		break;
	case TypeDef::TYPEID_STRUCT :
		// read subtype definitions first
		subtypes = read_type_list(data, type_lookup);
		// check all subtypes are supported, calculate total size, and build string name.
		strbuilder << "struct( ";
		for (int sub_id : subtypes) {
			const TypeDef & sub = type_lookup->at(sub_id);
			strbuilder << sub.m_type_name << ", ";
			if (!sub.m_is_supported) {
				// all subtypes must be supported.
				m_is_supported = false;
			} else {
				read_size += sub.m_h5_read_type.getSize();
				write_size += sub.m_h5_write_type.getSize();
			}
		}
		strbuilder << ")";
		m_type_name = strbuilder.str();
		if (m_is_supported) {
			// all subtypes are supported.
			comp_read_type = H5::CompType(read_size);
			comp_write_type = H5::CompType(write_size);
			// reuse read_size and write_size as offsets
			read_size = write_size = 0;
			for (int sub_id : subtypes) {
				const TypeDef & sub = type_lookup->at(sub_id);
				comp_read_type.insertMember(sub.m_name, read_size, sub.m_h5_read_type);
				read_size += sub.m_h5_read_type.getSize();
				comp_write_type.insertMember(sub.m_name, write_size, sub.m_h5_write_type);
				write_size += sub.m_h5_write_type.getSize();
			}
			m_h5_read_type = comp_read_type;
			m_h5_write_type = comp_write_type;
		}
		break;
	case TypeDef::TYPEID_STRING :
		m_type_name = "string";
		m_is_supported = false;
	case TypeDef::TYPEID_ARRAY :
		m_type_name = "array";
		m_is_supported = false;
	default :
		strbuilder << "unknown (id = " << m_data_type << ")";
		m_type_name = strbuilder.str();
		m_is_supported = false;
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
			std::ostringstream builder;
			builder << "Group expects " << len << 
				" types, but only got " << i << " valid types.";
            throw std::runtime_error(builder.str());
        }
    }
    
    return true;
}

