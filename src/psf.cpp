#include "psf.hpp"

namespace bip = boost::interprocess;

namespace psf {

    TypeDef::TypeDef(uint32_t id, std::string name, uint32_t array_type, uint32_t data_type,
                     std::vector<TypeDef> typedef_tuple, PropDict prop_dict) : m_id(id), m_name(name),
                                                                               m_array_type(array_type),
                                                                               m_data_type(data_type),
                                                                               m_typedef_tuple(typedef_tuple),
                                                                               m_prop_dict(prop_dict) {}
    
    TypeDef::TypeDef() {}

    TypeDef::~TypeDef() {}

    
    TypePointer::TypePointer(uint32_t id, std::string name, uint32_t type_id,
                             PropDict prop_dict) : m_id(id), m_name(name), m_type_id(type_id),
                                                   m_prop_dict(prop_dict) {}
    
    TypePointer::TypePointer() {}

    TypePointer::~TypePointer() {}

    Group::Group(uint32_t id, std::string name, TypePtrList vec) : m_id(id), m_name(name), m_vec(vec) {}
    
    Group::Group() {}

    Group::~Group() {}

    
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
        m_prop_dict = read_header(data_pointer, mmap_data);
        
        DEBUG_MSG("Reading types");
        m_type_list = read_type_section(data_pointer, mmap_data, false);

        DEBUG_MSG("Reading sweeps");
        // DEBUG_MSG("Current Position = " << (data_pointer - mmap_data));
        m_sweep_list = read_sweep(data_pointer, mmap_data);

        DEBUG_MSG("Reading traces");
        m_trace_list = read_trace_section(data_pointer, mmap_data, true);

        DEBUG_MSG("Reading valuess");
        read_sweep_values_test(data_pointer, mmap_data);
        
    }

    void read_sweep_values_test(char *& data, const char * orig) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid sweepval section code %d, expected %d") % code %
                               MAJOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;
        
        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("sweepval end position = " << end_pos);

        int windowsize = 4096;

        uint32_t zp_code = read_uint32(data);
        DEBUG_MSG("zero padding code = " << zp_code);

        uint32_t zp_size = read_uint32(data);
        DEBUG_MSG("zero padding size = " << zp_size << ", skipping");

        data += zp_size;
        uint32_t vali, np;
        std::string vals;
        double vald;

        vali = read_uint32(data);
        DEBUG_MSG(boost::format("%d %x") % vali % vali);
        vali = read_uint32(data);
        DEBUG_MSG(boost::format("%d %x") % vali % vali);
        np = (vali >> 16) & 0xffff;
        DEBUG_MSG(boost::format("%d") % np);
        for (int i = 0; i < np; i++) {
            vald = read_double(data);
            DEBUG_MSG(boost::format("%.6g") % vald);
        }
        data += windowsize - 8 * np;
        
        for (int i = 0; i < np; i++) {
            vald = read_double(data);
            DEBUG_MSG(boost::format("%.6g") % vald);
        }
        
        
    }

    
    /**
     * This functions reads the header section and returns the
     * property dictionary.
     *
     * header section format:
     * int code = major_section
     * int end_pos (end position of section).
     * PropEntry entry1
     * PropEntry entry2
     * ...
     * int end_marker = HEADER_END
     */
    std::unique_ptr<PropDict> read_header(char *& data, const char * orig) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid header section code %d, expected %d") % code %
                               MAJOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;
                
        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Header end position = " << end_pos);

        auto ans = PropDict::read(data);
        
        uint32_t end_marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << end_marker << " (should be " << HEADER_END << ")");
        if ((data - orig) != end_pos) {
            std::string msg = (boost::format("Header end position is not %d, something's wrong") %
                               end_pos).str();
            throw new std::runtime_error(msg);
        }
        
        return ans;
    }
    
    /**
     * This functions reads the type section and returns the
     * list of type definitions
     *
     * type section format:
     * int code = major_section
     * int end_pos (end position of section).
     * int code = minor_section
     * int end_pos (end position of sub-section).
     * TypeDef type1
     * TypeDef type2
     * ...
     * int index_type
     * int index_size
     * int index_id1
     * int index_offset1
     * int index_id2
     * int index_offset2
     * ...
     * int end_marker = TYPE_END
     */
    std::unique_ptr<TypeList> read_type_section(char *& data, const char * orig, bool is_trace) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid type section code %d, expected %d") % code %
                               MAJOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;

        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Type end position = " << end_pos);

        num_read = 0;
        code = peek_uint32(data, num_read);
        if (code != MINOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid type subsection code %d, expected %d") % code %
                               MINOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;
        
        uint32_t sub_end_pos = read_uint32(data);
        DEBUG_MSG("Type subsection end position = " << sub_end_pos);

        auto ans = std::unique_ptr<TypeList>(new TypeList());
        bool valid_type = true;
        while (valid_type && (data - orig) < sub_end_pos) {
            TypeDef temp = read_type(data, valid_type);
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

        uint32_t end_marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << end_marker << " (should be " << TYPE_END << ")");
        if ((data - orig) != end_pos) {
            std::string msg = (boost::format("type end position is not %d, something's wrong") %
                               end_pos).str();
            throw new std::runtime_error(msg);
        }
        
        return ans;
    }
    
    /**
     * This functions reads the sweep section and returns the
     * sweep list.
     *
     * sweep section format:
     * int code = major_section
     * int end_pos (end position of section).
     * TypePointer type1
     * TypePointer type2
     * ...
     * int end_marker = SWEEP_END
     */
    std::unique_ptr<TypePtrList> read_sweep(char *& data, const char * orig) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid sweep section code %d, expected %d") % code %
                               MAJOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;

        auto ans = std::unique_ptr<TypePtrList>(new TypePtrList());
        
        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Sweep end position = " << end_pos);

        DEBUG_MSG("Reading sweep types");
        bool valid_type = true;
        while (valid_type) {
            TypePointer temp = read_type_pointer(data, valid_type);
            if (valid_type) {
                ans->push_back(temp);
            }
        }

        uint32_t end_marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << end_marker << " (should be " << SWEEP_END << ")");
        if ((data - orig) != end_pos) {
            std::string msg = (boost::format("sweep end position is not %d, something's wrong") %
                               end_pos).str();
            throw new std::runtime_error(msg);
        }
        
        return ans;
    }
    
    /**
     * This functions reads the trace section and returns the
     * list of group or type pointers.
     *
     * type section format:
     * int code = major_section
     * int end_pos (end position of section).
     * int code = minor_section
     * int end_pos (end position of sub-section).
     * (TypePointer or Group) type1
     * (TypePointer or Group) type2
     * ...
     * int index_type
     * int index_size
     * int index_id1
     * int index_offset1
     * int extra1
     * int extra1
     * int index_id2
     * int index_offset2
     * int extra2
     * int extra2
     * ...
     * int end_marker = TRACE_END
     */
    std::unique_ptr<TraceList> read_trace_section(char *& data, const char * orig, bool is_trace) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != MAJOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid trace section code %d, expected %d") % code %
                               MAJOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;

        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("Trace end position = " << end_pos);

        num_read = 0;
        code = peek_uint32(data, num_read);
        if (code != MINOR_SECTION_CODE) {
            std::string msg = (boost::format("Invalid trace subsection code %d, expected %d") % code %
                               MINOR_SECTION_CODE).str();
            throw new std::runtime_error(msg);
        }
        data += num_read;
        
        uint32_t sub_end_pos = read_uint32(data);
        DEBUG_MSG("Trace subsection end position = " << sub_end_pos);

        auto ans = std::unique_ptr<TraceList>(new TraceList());
        bool valid_type = true;
        while (valid_type && (data - orig) < sub_end_pos) {
            Trace temp = read_trace(data, valid_type);
            if (valid_type) {
                ans->push_back(temp);
            }
        }
        
        uint32_t index_type = read_uint32(data);
        DEBUG_MSG("Trace index type = " << index_type);
        uint32_t index_size = read_uint32(data);
        DEBUG_MSG("Trace index size = " << index_size);
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

        uint32_t end_marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << end_marker << " (should be " << TRACE_END << ")");
        if ((data - orig) != end_pos) {
            std::string msg = (boost::format("trace end position is not %d, something's wrong") %
                               end_pos).str();
            throw new std::runtime_error(msg);
        }
        
        return ans;
    }
    
    /**
     * This functions reads the value section when no sweep is defined, and returns the
     * list of values
     *
     * value (no sweep) section format:
     * int code = major_section
     * int end_pos (end position of section).
     * int code = minor_section
     * int end_pos (end position of sub-section).
     * NonsweepValue val1
     * NonsweepValue val2
     * ...
     * int index_type
     * int index_size
     * int index_id1
     * int index_offset1
     * int index_id2
     * int index_offset2
     * ...
     * int end_marker = VALUE_END
     */
    
    /**
     * NonesweepValue format:
     * int code = nonsweep_value_code
     * int id
     * string name
     * int type_id
     * (char | int | double | string | complex | composite) value, depends on type_id
     * PropEntry entry1
     * PropEntry entry2
     * ...
     * 
     */

    /**
     * This functions reads the value section when sweep is defined, and returns the
     * list of values
     *
     * value (sweep, windowed mode) section format 1:
     * int code = major_section
     * int end_pos (end position of section).
     * ZeroPadding pad
     * int end_pos (end position of sub-section).
     * NonsweepValue val1
     * NonsweepValue val2
     * ...
     * int index_type
     * int index_size
     * int index_id1
     * int index_offset1
     * int index_id2
     * int index_offset2
     * ...
     * int end_marker = VALUE_END
     */

    /**
     * ZeroPadding format:
     * int code = zeropad_code (20)
     * int size
     * 0000000... (${size} number of 0 bytes).
     */

    
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
    std::vector<TypeDef> read_type_list(char *& data) {
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
            TypeDef temp = read_type(data, valid_type);
            if (valid_type) {
                ans.push_back(temp);                
            }
            
        }
        return ans;
    }
    
    /**
     * This function reads a TypeDef object from file.
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
    TypeDef read_type(char *& data, bool& valid) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != TypeDef::code) {
            valid = false;
            DEBUG_MSG("Invalid TypeDef code " << code << ", expected " << TypeDef::code);
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
            typedef_tuple = read_type_list(data);
        }
        
        // serialize properties
        DEBUG_MSG("Reading TypeDef Properties");
        bool valid_entry = true;
        auto prop_dict = PropDict::read(data);
        
        return TypeDef(id, name, array_type, data_type, typedef_tuple, *prop_dict.release());
    }

    /**
     * This function reads a TypePointer object from file.
     *
     * TypePointer format:
     * int code = typepointer_code
     * int id
     * string name
     * int type_id
     * PropEntry entry1
     * PropEntry entry2
     * ...
     *
     */
    TypePointer read_type_pointer(char *& data, bool& valid) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != TypePointer::code) {
            valid = false;
            DEBUG_MSG("Invalid TypePointer code " << code << ", expected " << TypePointer::code);
            return TypePointer();
        }
        data += num_read;

        valid = true;

        uint32_t id = read_uint32(data);
        std::string name = read_str(data);
        uint32_t type_id = read_uint32(data);
        
        DEBUG_MSG("TypePointer = (" << id << ", " << name << ", " << type_id << ")");
        
        // serialize properties
        DEBUG_MSG("Reading TypePointer Properties");
        bool valid_entry = true;
        auto prop_dict = PropDict::read(data);
        
        return TypePointer(id, name, type_id, *prop_dict.release());
    }

    /**
     * This function reads a Group object from file.
     *
     * Group format:
     * int code = group_code
     * int id
     * string name
     * int length
     * TypePointer pointer1
     * TypePointer pointer2
     * ...
     *
     */
    Group read_group(char *& data, bool& valid) {
        std::size_t num_read = 0;
        uint32_t code = peek_uint32(data, num_read);
        if (code != Group::code) {
            valid = false;
            DEBUG_MSG("Invalid Group code " << code << ", expected " << Group::code);
            return Group();
        }
        data += num_read;

        valid = true;
        uint32_t id = read_uint32(data);
        std::string name = read_str(data);
        uint32_t len = read_uint32(data);

        DEBUG_MSG("Group = (" << id << ", " << name << ", " << len << ")");

        DEBUG_MSG("Reading TypePointer Properties");
        bool valid_pointer = true;
        TypePtrList vec;
        for( int i = 0; i < len; i++) {
            TypePointer temp = read_type_pointer(data, valid_pointer);
            if (valid_pointer) {
                vec.push_back(temp);
            } else {
                std::string msg = (boost::format("Group expects %d types, but only got %d") %
                                   len % i).str();
                throw new std::runtime_error(msg);
            }
        }

        return Group(id, name, vec);
    }

    /**
     * This function reads a Trace object from file, which is either a
     * Group or a TypePointer.
     */    
    Trace read_trace(char *& data, bool& valid) {
        // don't increment, as either Group or TypePointer will
        // increment for us.
        std::size_t num_read;
        uint32_t code = peek_uint32(data, num_read);
        
        valid = true;
        Trace val;
        switch(code) {
        case Group::code :
            val = read_group(data, valid);
            return val;
        case TypePointer::code :
            val = read_type_pointer(data, valid);
            return val;
        default :
            valid = false;
            DEBUG_MSG("Cannot parse trace, invalid trace code " << code);
            return val;
        }
    }
        
}
