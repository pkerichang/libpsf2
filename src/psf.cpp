#include "psf.hpp"

namespace bip = boost::interprocess;

namespace psf {


    
    PSFDataSet::PSFDataSet(std::string filename) : m_invert_struct(false) {
        open(filename);
    }
    
    PSFDataSet::PSFDataSet(std::string filename, bool invert_struct) : m_invert_struct(invert_struct) {
        open(filename);
    }
    
    PSFDataSet::~PSFDataSet() {}

    PropDict* PSFDataSet::get_prop_dict() const {
        return m_prop_dict.get();
    }
    
    NestPropDict* PSFDataSet::get_scalar_prop_dict() const {
        return m_scalar_prop_dict.get();
    }
    
    StrVector* PSFDataSet::get_sweep_names() const {
        return m_swp_vars.get();
    }
    
    PSFVector* PSFDataSet::get_sweep_values() const {
        return m_swp_vals.get();
    }
    
    int PSFDataSet::get_num_sweeps() const {
        return m_num_sweeps;
    }
    
    int PSFDataSet::get_num_points() const {
        return m_num_points;
    }
    
    bool PSFDataSet::is_swept() const {
        return m_swept;
    }
    
    bool PSFDataSet::is_struct_inverted() const {
        return m_invert_struct;
    }
    
    PropDict* PSFDataSet::get_scalar_dict() const {
        return m_scalar_dict.get();
    }
    
    VecDict* PSFDataSet::get_vector_dict() const {
        return m_vector_dict.get();
    }

    TypeDef::TypeDef(uint32_t id, std::string name, uint32_t array_type, uint32_t data_type,
                     std::vector<TypeDef> typedef_tuple, PropDict prop_dict) : m_id(id), m_name(name),
                                                                               m_array_type(array_type),
                                                                               m_data_type(data_type),
                                                                               m_typedef_tuple(typedef_tuple),
                                                                               m_prop_dict(prop_dict) {
    }
    
    TypeDef::TypeDef() {}

    TypeDef::~TypeDef() {}
    
    void PSFDataSet::open(std::string filename) {
        // initialize members
        
        m_scalar_prop_dict = std::unique_ptr<NestPropDict>(new NestPropDict());
        m_scalar_dict = std::unique_ptr<PropDict>(new PropDict());
        m_vector_dict = std::unique_ptr<VecDict>(new VecDict());
        m_swp_vars = std::unique_ptr<StrVector>(new StrVector());
        m_swp_vals = std::unique_ptr<PSFVector>(new PSFVector());
        
        m_num_sweeps = 1;
        m_num_points = 1;
        m_swept = false;

        // create memory map file
        bip::file_mapping mapping(filename.c_str(), bip::read_only);
        bip::mapped_region mapped_rgn(mapping, bip::read_only);
        char* data_pointer = static_cast<char*>(mapped_rgn.get_address());
        const char* mmap_data = data_pointer;
        std::size_t const mmap_size = mapped_rgn.get_size();

        // read first word and throw away
        uint32_t first_word = read_uint32(data_pointer);
        DEBUG_MSG("First word value = " << first_word);

        DEBUG_MSG("Reading header");
        m_prop_dict = read_header(data_pointer, mmap_data, 1);
        if (m_prop_dict == nullptr) {
            throw new std::runtime_error("Error reading header section");
        }
        DEBUG_MSG("Reading types");
        m_type_list = read_type_section(data_pointer, mmap_data, 1, false);
        if (m_type_list == nullptr) {
            throw new std::runtime_error("Error reading type section");
        }        
    }
    
    /**
     * header properties section format:
     * first word is the header section code, second word is the last index of the header section.
     * the last word of the header section has the value of m_end_marker.
     * the middle is a series of (type_word, name_data, value_data) bytes.
     * type_word is a single word that indicates the value type.
     * name_data is always a string, and value_data can be an int, a
     * double, or a string.
     * an int is always one word, a double is always two words, and a
     * string has one word encoding the length, followed by the
     * word-aligned (i.e. multiple of 4) char array data.
     */
    std::unique_ptr<PropDict> read_header(char *& data, const char * orig, uint32_t end_marker) {
        std::unique_ptr<PropDict> ans;
        std::size_t num_read = 0;
        uint32_t id = peek_uint32(data, num_read);
        if (id != MAJOR_SECTION_CODE) {
            DEBUG_MSG("Invalid header section code " << id <<
                      ", expected " << MAJOR_SECTION_CODE);
            return ans;
        }
        data += num_read;
        ans = std::unique_ptr<PropDict>(new PropDict());
        
        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Header end position = " << end_pos);

        bool valid_entry = true;
        while (valid_entry && (data - orig) < end_pos) {
            PropEntry entry = read_entry(data, end_marker, valid_entry);
            if (valid_entry) {
                ans->insert(entry);
            }
        }
        
        return ans;
    }

    std::unique_ptr<TypeList> read_type_section(char *& data, const char * orig, uint32_t end_marker,
                                                bool is_trace) {
        std::unique_ptr<TypeList> ans;
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            DEBUG_MSG("Invalid type section code " << code <<
                      ", expected " << MAJOR_SECTION_CODE);
            return ans;
        }
        data += num_read;

        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Type end position = " << end_pos);

        num_read = 0;
        code = peek_uint32(data, num_read);
        if (code != MINOR_SECTION_CODE) {
            DEBUG_MSG("Invalid type subsection code " << code <<
                      ", expected " << MINOR_SECTION_CODE);
            return ans;
        }
        data += num_read;
        
        uint32_t sub_end_pos = read_uint32(data);
        DEBUG_MSG("Type subsection end position = " << sub_end_pos);

        ans = std::unique_ptr<TypeList>(new TypeList());
        bool valid_type = true;
        while (valid_type && (data - orig) < sub_end_pos) {
            TypeDef temp = read_type(data, end_marker, valid_type);
            if (valid_type) {
                ans->push_back(temp);
            }
        }
        
        uint32_t index_type = read_uint32(data);
        DEBUG_MSG("Type index type = " << index_type);
        uint32_t index_size = read_uint32(data);
        DEBUG_MSG("Type index size = " << index_size);
        if (is_trace) {
            // read trace information
            uint32_t id, offset, extra1, extra2;
            for(int i = 0; i < index_size; i += 4 * WORD_SIZE) {
                id = read_uint32(data);
                offset = read_uint32(data);
                extra1= read_uint32(data);
                extra2 = read_uint32(data);
                DEBUG_MSG("trace index: (" << id << ", " << offset <<
                          ", " << extra1 << ", " << extra2 << ")");
            }
        } else {
            // read index information
            uint32_t id, offset;
            for(int i = 0; i < index_size; i += 2 * WORD_SIZE) {
                id = read_uint32(data);
                offset = read_uint32(data);
                DEBUG_MSG("index: (" << id << ", " << offset << ")");
            }
        }
        
    }
    
    // Decodes a single name-value entry.
    std::vector<TypeDef> read_type_list(char *& data, uint32_t end_marker) {
        bool valid_type = true;
        std::vector<TypeDef> ans;
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
            TypeDef temp = read_type(data, end_marker, valid_type);
            if (valid_type) {
                ans.push_back(temp);                
            }
            
        }
        return ans;
    }
    
    // Decodes a single name-value entry.
    TypeDef read_type(char *& data, uint32_t end_marker, bool& valid) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != TypeDef::code) {
            valid = false;
            DEBUG_MSG("Invalid TypeDef code" << code <<
                      ", expected " << TypeDef::code);
            return TypeDef();
        }
        data += num_read;

        valid = true;

        uint32_t id = read_uint32(data);
        std::string name = read_str(data);
        uint32_t array_type = read_uint32(data);
        uint32_t data_type = read_uint32(data);
        
        DEBUG_MSG("TypeDef = (" << id << ", " << name <<
                  ", " << array_type << ", " << data_type << ")");
        
        std::vector<TypeDef> typedef_tuple;
        if (data_type == TypeDef::struct_code) {
            // data type is a struct
            DEBUG_MSG("Reading TypeDef Tuple");
            typedef_tuple = read_type_list(data, end_marker);
        }
        
        // serialize properties
        PropDict prop_dict;
        bool valid_entry = true;
        DEBUG_MSG("Reading TypeDef Properties");
        do {
            PropEntry entry = read_entry(data, end_marker, valid_entry);
            if (valid_entry) {
                prop_dict.insert(entry);
            }
        } while (valid_entry);

        return TypeDef(id, name, array_type, data_type, typedef_tuple, prop_dict);
    }

    PropEntry read_entry(char *& data, uint32_t end_marker, bool& valid) {
        std::size_t num_read = 0;
        uint32_t id = peek_uint32(data, num_read);
        data += num_read;
        if (id == end_marker) {
            valid = false;
            DEBUG_MSG("Read end marker entry");
            return PropEntry();
        }

        valid = true;
        std::string name;
        PSFScalar val;
        switch(id) {
        case 33 :
            name = read_str(data);
            val = read_str(data);
            DEBUG_MSG("Read entry (" << name << ", " << val << ")");
            return PropEntry(name, val);
        case 34 :
            name = read_str(data);
            val = read_int32(data);
            DEBUG_MSG("Read entry (" << name << ", " << val << ")");
            return PropEntry(name, val);
        case 35 :
            name = read_str(data);
            val = read_double(data);
            DEBUG_MSG("Read entry (" << name << ", " << val << ")");
            return PropEntry(name, val);
        default :
            valid = false;
            data -= num_read;
            DEBUG_MSG("Cannot parse entry");
            return PropEntry();
        }
    }
    
}
