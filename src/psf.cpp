#include "psf.hpp"


namespace psf {
    
	inline uint32_t read_section_preamble(std::ifstream & data, uint32_t section_code);
	inline void read_section_end(std::ifstream & data, uint32_t end_pos, uint32_t end_marker);
	inline void read_index(std::ifstream & data, bool is_trace);

	void read_psf(std::string filename) {
		// initialize members

		// create memory map file
		std::ifstream data(filename, std::ios::binary);

		// read first word and throw away
		uint32_t first_word = read_uint32(data);
		DEBUG_MSG("First word value = " << first_word);

		DEBUG_MSG("Reading header");
		auto prop_dict = read_header(data);

		DEBUG_MSG("Reading types");
		auto type_map = read_type(data);

		DEBUG_MSG("Reading sweeps");
		auto sweep_list = read_sweep(data);

		DEBUG_MSG("Reading traces");
		auto trace_list = read_trace(data);

		// check we have at least one sweep variable.
		if (sweep_list->size() == 0) {
			throw std::runtime_error("Non-sweep PSF file is not supported yet.  Contact developers.");
		}

		// check that we have exactly one sweep variable.
		if (sweep_list->size() > 1) {
			throw std::runtime_error("Non-single sweep PSF file is not supported.  If you use ADEXL for parametric sweep this shouldn't happen.");
		}

		// check that this is a windowed sweep.
		auto prop_iter = prop_dict->find("PSF window size");
		if (prop_iter == prop_dict->end()) {
			throw std::runtime_error("Non-windowed sweep is not supported yet.  Contact developers.");
		}
		uint32_t win_size = prop_iter->second.m_ival;

		// check that sweep variable is a scalar type.
		const Variable & swp_var = sweep_list->at(0);
		const TypeDef & swp_type = type_map->at(swp_var.m_type_id);
		if (!swp_type.m_is_supported) {
			std::ostringstream builder;
			builder << "Sweep variable " << swp_var.m_name <<
				" with type \"" << swp_type.m_name << "\" (data type = " << 
				swp_type.m_type_name << " ) not supported.";
			throw std::runtime_error(builder.str());
		}

		// check that all output variables are scalar types.
		size_t num_traces = trace_list->size();
		for (auto output : *trace_list.get()) {
			const TypeDef & output_type = type_map->at(output.m_type_id);
			if (!output_type.m_is_supported) {
				std::ostringstream builder;
				builder << "Output variable " << output.m_name <<
					" with type \"" << output_type.m_name << "\" (data type = " <<
					output_type.m_type_name << " ) not supported.";
				throw std::runtime_error(builder.str());
			}
		}

		// check that number of sweep points is recorded.
		prop_iter = prop_dict->find("PSF sweep points");
		if (prop_iter == prop_dict->end()) {
			throw std::runtime_error("Cannot find property PSF \"PSF sweep points\".");
		}
		uint32_t num_points_data = prop_iter->second.m_ival;

		DEBUG_MSG("Reading valuess");
		read_values_swp_window(data, num_points_data, win_size,
			swp_var, trace_list.get(), type_map.get());

		data.close();
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
	std::unique_ptr<PropDict> read_header(std::ifstream & data) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);

		auto ans = std::unique_ptr<PropDict>(new PropDict());
		ans->read(data);

		read_section_end(data, end_pos, HEADER_END);

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
	std::unique_ptr<TypeMap> read_type(std::ifstream & data) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);
		uint32_t sub_end_pos = read_section_preamble(data, MINOR_SECTION_CODE);

		auto ans = std::unique_ptr<TypeMap>(new TypeMap());
		bool valid_type = true;
		while (valid_type && static_cast<uint32_t>(data.tellg()) < sub_end_pos) {
			TypeDef temp;
			valid_type = temp.read(data, ans.get());
		}

		read_index(data, false);
		read_section_end(data, end_pos, TYPE_END);

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
	std::unique_ptr<VarList> read_sweep(std::ifstream & data) {

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

		read_section_end(data, end_pos, SWEEP_END);

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
	std::unique_ptr<VarList> read_trace(std::ifstream & data) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);
		uint32_t sub_end_pos = read_section_preamble(data, MINOR_SECTION_CODE);

		// each trace entry is either a Variable or Group.
		// however, since we're just translating PSF to HDF5,
		// we don't really care about hierarchy, so we're just
		// going to flatten everything to Variables.
		auto ans = std::unique_ptr<VarList>(new VarList());
		bool valid_type = true;
		while (valid_type && static_cast<uint32_t>(data.tellg()) < sub_end_pos) {
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
		read_section_end(data, end_pos, TRACE_END);

		return ans;
	}

	void read_values_swp_window(std::ifstream & data, uint32_t num_points, 
		uint32_t windowsize, const Variable & swp_var,
		VarList * trace_list, TypeMap * type_map) {

		uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);

		uint32_t zp_code = read_uint32(data);
		DEBUG_MSG("zero padding code = " << zp_code);

		uint32_t zp_size = read_uint32(data);
		DEBUG_MSG("zero padding size = " << zp_size << ", skipping");
		data.seekg(zp_size, std::ios::cur);

		uint32_t code = read_uint32(data);
		if (code != SWP_WINDOW_SECTION_CODE) {
			std::ostringstream builder;
			builder << "Expect code = " << SWP_WINDOW_SECTION_CODE <<
				", but got " << code;
			throw std::runtime_error(builder.str());
		}

		uint32_t size_word = read_uint32(data);
		uint32_t size_left = size_word >> 16;
		uint32_t np_window = size_word & 0xffff;
		DEBUG_MSG("Size word left value = " << size_left);
		DEBUG_MSG("Number of valid data in window = " << np_window);
		
		/*
		// open file
		std::string fname("test.hdf5");
		hsize_t file_dim[1] = { num_points };
		H5::DataSpace file_space(1, file_dim, file_dim);
		auto file = std::unique_ptr<H5::H5File>(new H5::H5File(fname.c_str(), H5F_ACC_TRUNC));

		// create sweep dataset
		const TypeDef & swp_type = type_map->at(swp_var.m_type_id);
		auto swp_dset = std::unique_ptr<H5::DataSet>(new H5::DataSet(
			file->createDataSet(swp_var.m_name.c_str(), swp_type.m_h5_write_type, file_space)));

		// create output datasets
		auto out_dsets = std::vector<std::unique_ptr<H5::DataSet>>(trace_list->size());
		for (auto out_var : *trace_list) {
			const TypeDef & out_type = type_map->at(out_var.m_type_id);
			out_dsets.push_back(std::move(std::unique_ptr<H5::DataSet>(new H5::DataSet(
				file->createDataSet(out_var.m_name.c_str(), out_type.m_h5_write_type, file_space)))));
		}

		hsize_t mem_dim[1] = { np_window };
		hsize_t count[1] = { np_window };
		hsize_t file_offset[1] = { 0 };
		hsize_t file_stride[1] = { 1 };
		H5::DataSpace mem_space(1, mem_dim, mem_dim);

		uint32_t points_read = 0;
		auto buffer = std::unique_ptr<char[]>(new char[windowsize]);
		while (points_read < num_points) {
			data.read(buffer.get(), windowsize);
			// write sweep values
			file_offset[0] = points_read;
			file_space.selectHyperslab(H5S_SELECT_SET, count, file_offset, file_stride, file_stride);
			mem_space.selec
			time.write(vec, H5::PredType::IEEE_F64BE, mem_space, file_space);
			offset[0] = num_points;
			file_space.selectHyperslab(H5S_SELECT_SET, count, offset, stride, stride);
			time.write(vec, H5::PredType::IEEE_F64BE, mem_space, file_space);
			time.close();
			data.seekg(windowsize - 8 * np_window, std::ios::cur);
			data.read(vec, np_window * 8);
			offset[0] = 0;
			file_space.selectHyperslab(H5S_SELECT_SET, count, offset, stride, stride);
			vout.write(vec, H5::PredType::IEEE_F64BE, mem_space, file_space);
			offset[0] = num_points;
			file_space.selectHyperslab(H5S_SELECT_SET, count, offset, stride, stride);
			vout.write(vec, H5::PredType::IEEE_F64BE, mem_space, file_space);
			vout.close();
			delete vec;
			file->close();
		}
		*/
	}
    
    /**
     * Read the section preamble.  Returns end position index.
     *
     * section preamble format:
     * int code = MAJOR_SECTION_CODE
     * int end_pos (end position of section).
     */
    inline uint32_t read_section_preamble(std::ifstream & data, uint32_t section_code) {
        uint32_t code = read_uint32(data);
        if (code != section_code) {
			std::ostringstream builder;
			builder << "Invalid section code " << code << ", expected " << section_code;
            throw std::runtime_error(builder.str());
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
    inline void read_section_end(std::ifstream & data, uint32_t end_pos, uint32_t end_marker) {
        uint32_t marker = read_uint32(data);
        DEBUG_MSG("Read end marker " << marker << " (should be " << end_marker << ")");
        if (static_cast<uint32_t>(data.tellg()) != end_pos) {
			std::ostringstream builder;
			builder << "Section end position is not " << end_pos << ", something's wrong";
            throw std::runtime_error(builder.str());
        }
    }

    /**
     * Read the index section.
     *
     */
    inline void read_index(std::ifstream & data, bool is_trace) {
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
