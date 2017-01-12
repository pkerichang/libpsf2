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
        
        cur_idx += deserialize_prop_section(mmap_data, cur_idx, 21, 1, m_prop_dict.get());
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
        
        DEBUG_MSG("Prop section ID = " << type_id);
        
        // check that ID is valid    
        if (type_id != ps_type) {
            char msg[100];
            snprintf(msg, 100, "Type ID mismatch; expect %d, got %d", ps_type, type_id);
            throw std::runtime_error(msg);
        }
        
        std::size_t cur_idx = start_idx + WORD_SIZE;

        std::size_t end_idx = GET_I32(start + cur_idx);
        cur_idx += WORD_SIZE;
        DEBUG_MSG("Prop section end location = " << end_idx);

        while(cur_idx < end_idx) {
            cur_idx += deserialize_entry(start + cur_idx, end_marker, pdict);
            DEBUG_MSG("Cursor location = " << cur_idx);
        }
        
        return cur_idx - start_idx;
    }

    // Decodes a single name-value entry.
    std::size_t deserialize_entry(const char* start, uint32_t end_marker, PropDict* pdict) {
        uint32_t type_id = GET_I32(start);
        std::size_t loc_idx = WORD_SIZE;

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
            pdict->emplace(name, val);
            return loc_idx;
        case 34 :
            name = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry name = " << name);
            // int type
            val = static_cast<int>(deserialize_int(start + loc_idx, loc_idx));
            DEBUG_MSG("Entry val = " << val);
            pdict->emplace(name, val);
            return loc_idx;
        case 35 :
            name = deserialize_str(start + loc_idx, loc_idx);
            DEBUG_MSG("Entry name = " << name);
            // double type
            val = static_cast<double>(deserialize_double(start + loc_idx, loc_idx));
            DEBUG_MSG("Entry val = " << val);
            pdict->emplace(name, val);
            return loc_idx;
        default :
            if (type_id == end_marker) {
                // end marker, nothing to read.
                DEBUG_MSG("Entry is end marker");
                return loc_idx;
            }
            // error
            char msg[100];
            snprintf(msg, 100, "Unrecognized Type ID: %d", type_id);
            throw std::runtime_error(msg);
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
