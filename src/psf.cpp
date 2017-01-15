#include "psf.hpp"

namespace bip = boost::interprocess;


namespace psf {
    
	inline uint32_t read_section_preamble(char *& data, uint32_t section_code);
	inline void read_section_end(char *& data, const char * orig, uint32_t end_pos, uint32_t end_marker);
	inline void read_index(char *& data, bool is_trace);

    void read_psf(std::string filename) {
        // initialize members
    
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
        auto prop_dict = read_header(data_pointer, mmap_data);
    
        DEBUG_MSG("Reading types");
        auto type_map = read_type(data_pointer, mmap_data);
    
        DEBUG_MSG("Reading sweeps");
        auto sweep_list = read_sweep(data_pointer, mmap_data);
    
        DEBUG_MSG("Reading traces");
        auto trace_list = read_trace(data_pointer, mmap_data);
    
		if (sweep_list->size() == 0) {
			throw new std::runtime_error("Non-sweep PSF file is not supported yet.  Contact developers.");
		}

		if (sweep_list->size() > 1) {
			throw new std::runtime_error("Non-single sweep PSF file is not supported.  If you use ADEXL for parametric sweep this shouldn't happen.");
		}

		auto prop_iter = prop_dict->find("PSF window size");
		if (prop_iter == prop_dict->end()) {
			throw new std::runtime_error("Non-windowed sweep is not supported yet.  Contact developers.");
		}
		uint32_t win_size = static_cast<uint32_t>(boost::get<int32_t>(prop_iter->second));

		prop_iter = prop_dict->find("PSF sweep points");
		if (prop_iter == prop_dict->end()) {
			throw new std::runtime_error("Cannot find property PSF \"PSF sweep points\".");
		}
		uint32_t num_points_data = static_cast<uint32_t>(boost::get<int32_t>(prop_iter->second));

        DEBUG_MSG("Reading valuess");
        read_values_swp_window(data_pointer, mmap_data, num_points_data, win_size);
    
    }

	/**
	* This functions reads the header section and returns the
	* property dictionary.
	*
	* header section body format:
	* PropEntry entry1
	* PropEntry entry2
	* ...
	*/
	std::unique_ptr<PropDict> read_header(char *& data, const char * orig) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);

		auto ans = std::unique_ptr<PropDict>(new PropDict());
		ans->read(data);

		read_section_end(data, orig, end_pos, HEADER_END);

		return ans;
	}

	/**
	* This functions reads the type section and returns the
	* list of type definitions.
	*
	* type section body format:
	* subsection{
	* TypeDef type1
	* TypeDef type2
	* ...
	* }
	* int index_type
	* int index_size
	* int index_id1
	* int index_offset1
	* int index_id2
	* int index_offset2
	* ...
	* int end_marker = TYPE_END
	*/
	std::unique_ptr<TypeMap> read_type(char *& data, const char * orig) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);
		uint32_t sub_end_pos = read_section_preamble(data, MINOR_SECTION_CODE);

		auto ans = std::unique_ptr<TypeMap>(new TypeMap());
		bool valid_type = true;
		while (valid_type && (data - orig) < sub_end_pos) {
			TypeDef temp;
			valid_type = temp.read(data, ans.get());
		}

		read_index(data, false);
		read_section_end(data, orig, end_pos, TYPE_END);

		return ans;
	}

	/**
	* This functions reads the sweep section and returns the
	* sweep list.
	*
	* sweep body format:
	* Variable type1
	* Variable type2
	* ...
	*/
	std::unique_ptr<VarList> read_sweep(char *& data, const char * orig) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);

		DEBUG_MSG("Reading sweep types");
		auto ans = std::unique_ptr<VarList>(new VarList());
		bool valid_type = true;
		while (valid_type) {
			Variable temp;
			valid_type = temp.read(data);
			if (valid_type) {
				ans->push_back(temp);
			}
		}

		read_section_end(data, orig, end_pos, SWEEP_END);

		return ans;
	}

	/**
	* This functions reads the trace section and returns the
	* list of group or type pointers.
	*
	* trace section body format:
	* subsection{
	* (Variable or Group) type1
	* (Variable or Group) type2
	* ...
	* }
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
	std::unique_ptr<VarList> read_trace(char *& data, const char * orig) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);
		uint32_t sub_end_pos = read_section_preamble(data, MINOR_SECTION_CODE);

		// each trace entry is either a Variable or Group.
		// however, since we're just translating PSF to HDF5,
		// we don't really care about hierarchy, so we're just
		// going to flatten everything to Variables.
		auto ans = std::unique_ptr<VarList>(new VarList());
		bool valid_type = true;
		while (valid_type && (data - orig) < sub_end_pos) {
			// try reading as Group
			Group grp;
			valid_type = grp.read(data);
			if (valid_type) {
				// flatten Group into our variable list
				ans->insert(ans->end(), grp.m_vec.begin(), grp.m_vec.end());
			} else {
				// Group failed, try reading as Variable.
				Variable var;
				valid_type = var.read(data);
				if (valid_type) {
					ans->push_back(var);
				}
			}
		}

		read_index(data, true);
		read_section_end(data, orig, end_pos, TRACE_END);

		return ans;
	}

	void read_values_swp_window(char *& data, const char * orig, uint32_t np_tot, uint32_t windowsize) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);

		uint32_t zp_code = read_uint32(data);
		DEBUG_MSG("zero padding code = " << zp_code);

		uint32_t zp_size = read_uint32(data);
		DEBUG_MSG("zero padding size = " << zp_size << ", skipping");
		data += zp_size;

		uint32_t code = read_uint32(data);
		if (code != SWP_WINDOW_SECTION_CODE) {
			throw new std::runtime_error((boost::format("Expect code = %d, but got %d") % 
				SWP_WINDOW_SECTION_CODE % code).str());
		}

		uint32_t size_word = read_uint32(data);
		uint32_t windows_left = size_word >> 16;
		uint32_t num_points = size_word & 0xffff;
		DEBUG_MSG("Number of points in window = " << windows_left);
		DEBUG_MSG("Number of valid data in window = " << num_points);
		
		DEBUG_MSG("Printing time");
		for (uint32_t i = 0; i < num_points; i++) {
			double vald = read_double(data);
			DEBUG_MSG(boost::format("%.6g") % vald);
		}
		data += windowsize - 8 * num_points;
		DEBUG_MSG("Printing data1");
		for (uint32_t i = 0; i < num_points; i++) {
			double vald = read_double(data);
			DEBUG_MSG(boost::format("%.6g") % vald);
		}
	}
    
    /**
     * Read the section preamble.  Returns end position index.
     *
     * section preamble format:
     * int code = MAJOR_SECTION_CODE
     * int end_pos (end position of section).
     */
    inline uint32_t read_section_preamble(char *& data, uint32_t section_code) {
        uint32_t code = read_uint32(data);
        if (code != section_code) {
            std::string msg = (boost::format("Invalid section code %d, expected %d") % code %
                               section_code).str();
            throw new std::runtime_error(msg);
        }
    
        uint32_t end_pos = read_uint32(data);
        DEBUG_MSG("section end position = " << end_pos);

        return end_pos;
    }

    /**
     * Read the second end.  
     *
     * section end format:
     * int marker = end_marker.
     */
    inline void read_section_end(char *& data, const char * orig, uint32_t end_pos, uint32_t end_marker) {
        uint32_t marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << marker << " (should be " << end_marker << ")");
        if ((data - orig) != end_pos) {
            std::string msg = (boost::format("Section end position is not %d, something's wrong") %
                               end_pos).str();
            throw new std::runtime_error(msg);
        }
    }

    /**
     * Read the index section.
     *
     */
    inline void read_index(char *& data, bool is_trace) {
        uint32_t index_type = read_uint32(data);
        DEBUG_MSG("Type index type = " << index_type);
        uint32_t index_size = read_uint32(data);
        DEBUG_MSG("Type index size = " << index_size);
        if (is_trace) {
            // read trace information
            uint32_t id, offset, extra1, extra2;
            for(uint32_t i = 0; i < index_size; i += 4 * WORD_SIZE) {
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
            for(uint32_t i = 0; i < index_size; i += 2 * WORD_SIZE) {
                id = read_uint32(data);
                offset = read_uint32(data);
                DEBUG_MSG("index: (" << id << ", " << offset << ")");
            }
        }

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

}
