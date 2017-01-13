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
    
    void PSFDataSet::open(std::string filename) {
        // initialize members
        m_prop_dict = std::unique_ptr<PropDict>(new PropDict());
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
        const char* mmap_data = static_cast<char*>(mapped_rgn.get_address());
        std::size_t const mmap_size = mapped_rgn.get_size();

        // read first word and throw away
        uint32_t first_word = GET_I32(mmap_data);
        DEBUG_MSG("First word is: " << first_word);
        std::size_t cur_idx = WORD_SIZE;

        DEBUG_MSG("Deserializing header section");
        // type id = 21, end marker = 1
        cur_idx += deserialize_prop_section(mmap_data, cur_idx, 21, 1, m_prop_dict.get());
        DEBUG_MSG("Cursor location = " << cur_idx);

        DEBUG_MSG("Deserializing type section");
        cur_idx += deserialize_indexed_section(mmap_data, cur_idx, 21, 1, false, m_prop_dict.get());
        DEBUG_MSG("Cursor location = " << cur_idx);
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
    std::size_t deserialize_prop_section(const char* start, std::size_t start_idx, uint32_t ps_type,
                                         uint32_t end_marker, PropDict* pdict) {
        uint32_t type_id = GET_I32(start + start_idx);
        std::size_t cur_idx = start_idx + WORD_SIZE;
        DEBUG_MSG("Prop section ID = " << type_id);
        // check that ID is valid    
        if (type_id != ps_type) {
            char msg[100];
            snprintf(msg, 100, "Type ID mismatch; expect %d, got %d", ps_type, type_id);
            throw std::runtime_error(msg);
        }
        
        std::size_t end_idx = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("Prop section end location = " << end_idx);

        bool end_reached = false;
        while(!end_reached && cur_idx < end_idx) {
            cur_idx += deserialize_entry(start + cur_idx, end_marker, end_reached);
            DEBUG_MSG("Cursor location = " << cur_idx);
        }
        
        return cur_idx - start_idx;
    }

    /**
     *
     */
    std::size_t deserialize_indexed_section(const char* start, std::size_t start_idx, uint32_t ps_type,
                                            uint32_t end_marker, bool is_trace, PropDict* pdict) {
        uint32_t type_id = GET_I32(start + start_idx);        
        std::size_t cur_idx = start_idx + WORD_SIZE;
        DEBUG_MSG("Index section ID = " << type_id);
        // check that ID is valid    
        if (type_id != ps_type) {
            char msg[100];
            snprintf(msg, 100, "Type ID mismatch; expect %d, got %d", ps_type, type_id);
            throw std::runtime_error(msg);
        }
        // get end index

        std::size_t end_idx = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("Index section end location = " << end_idx);

        // get sub section type
        constexpr uint32_t subsec_type = 22;
        type_id = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("sub section ID = " << type_id);
        // check that ID is valid    
        if (type_id != subsec_type) {
            char msg[100];
            snprintf(msg, 100, "Type ID mismatch; expect %d, got %d", subsec_type, type_id);
            throw std::runtime_error(msg);
        }

        // get sub section end index
        std::size_t subsec_end_idx = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("sub section end location = " << subsec_end_idx);

        while(cur_idx < subsec_end_idx) {
            cur_idx += deserialize_type(start + cur_idx, end_marker);
            DEBUG_MSG("Cursor location = " << cur_idx);
        }

        // read index type
        std::size_t index_type = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("index type = " << index_type);
        // read index size
        std::size_t index_size = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("index size = " << index_size);
        if (is_trace) {
            // read trace information
            uint32_t id, offset, extra1, extra2;
            for(int i = 0; i < index_size; i += 4 * WORD_SIZE) {
                id = GET_I32(start + cur_idx + i);
                offset = GET_I32(start + cur_idx + i + WORD_SIZE);
                extra1 = GET_I32(start + cur_idx + i + 2 * WORD_SIZE);
                extra2 = GET_I32(start + cur_idx + i + 3 * WORD_SIZE);
                DEBUG_MSG("trace index: (" << id << ", " << offset << ", " << extra1 << ", " << extra2 << ")");
            }
        } else {
            // read index information
            uint32_t id, offset;
            for(int i = 0; i < index_size; i += 2 * WORD_SIZE) {
                id = GET_I32(start + cur_idx + i);
                offset = GET_I32(start + cur_idx + i + WORD_SIZE);
                DEBUG_MSG("index: (" << id << ", " << offset << ")");
            }
        }
        cur_idx += index_size;
        return cur_idx - start_idx;
    }

    // Decodes a single name-value entry.
    int deserialize_struct(const char* start, uint32_t end_marker, std::size_t& num_read) {
        bool finished = false;
        std::size_t loc_idx = 0;
        do {
            uint32_t typedef_id = GET_I32(start);
            std::size_t loc_idx = WORD_SIZE;
            DEBUG_MSG("TypeDef Type ID = " << typedef_id);
            // check that ID is valid
            constexpr uint32_t expect_id = 18;
            if (typedef_id != expect_id) {
                finished = true;
            } else {
                loc_idx += deserialize_type(start + loc_idx, end_marker);
            }
        } while (!finished);

        return 0;
    }

    
    // Decodes a single name-value entry.
    std::size_t deserialize_type(const char* start, uint32_t end_marker) {
        uint32_t type_id = GET_I32(start);
        std::size_t loc_idx = WORD_SIZE;
        DEBUG_MSG("TypeDef Type ID = " << type_id);
        // check that ID is valid
        constexpr uint32_t expect_id = 16;
        if (type_id != expect_id) {
            char msg[100];
            snprintf(msg, 100, "TypeDef Type ID mismatch; expect %d, got %d", expect_id, type_id);
            throw std::runtime_error(msg);
        }

        // TypeDef ID
        uint32_t typedef_id = GET_I32(start + loc_idx);
        loc_idx += WORD_SIZE;
        DEBUG_MSG("TypeDef ID = " << typedef_id);
        // TypeDef Name
        std::string name = deserialize_str(start + loc_idx, loc_idx);
        DEBUG_MSG("TypeDef name = " << name);
        // TypeDef array type
        uint32_t arr_type = GET_I32(start + loc_idx);
        loc_idx += WORD_SIZE;
        DEBUG_MSG("TypeDef array type = " << arr_type);
        // TypeDef data type ID
        uint32_t data_type = GET_I32(start + loc_idx);
        loc_idx += WORD_SIZE;
        DEBUG_MSG("TypeDef data type ID = " << data_type);

        constexpr uint32_t struct_type = 16;
        if (data_type == struct_type) {
            // TypeDef is a struct
            int my_struct = deserialize_struct(start + loc_idx, end_marker, loc_idx);
        }
        
        // serialize properties
        bool end_reached = false;
        do {
            loc_idx += deserialize_entry(start + loc_idx, end_marker, end_reached);
        } while (!end_reached);
        
        return loc_idx;
    }
    
    // Decodes a single name-value entry.
    std::size_t deserialize_entry(const char* start, uint32_t end_marker, bool& end_reached) {
        uint32_t type_id = GET_I32(start);
        std::size_t loc_idx = WORD_SIZE;
        end_reached = false;
        
        DEBUG_MSG("Entry ID = " << type_id);

        // read name or end marker
        std::string name;
        PSFScalar val;
        switch(type_id) {
        case 33 :
            name = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry name = " << name);
            // string type
            val = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry val = " << val);
            // pdict->emplace(name, val);
            return loc_idx;
        case 34 :
            name = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry name = " << name);
            // int type
            val = deserialize_int32(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry val = " << val);
            // pdict->emplace(name, val);
            return loc_idx;
        case 35 :
            name = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry name = " << name);
            // double type
            val = deserialize_double(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry val = " << val);
            // pdict->emplace(name, val);
            return loc_idx;
        default :
            end_reached = true;
            if (type_id == end_marker) {
                // end marker, nothing to read.
                DEBUG_MSG("Entry is end marker");
                return loc_idx;
            }
            DEBUG_MSG("Cannot parse entry");
            return 0;
        }
        
    }
    
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
}
