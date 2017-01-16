#include "psf.hpp"

INITIALIZE_EASYLOGGINGPP

namespace psf {

    std::unique_ptr<PropDict> read_header(std::ifstream& data);
    std::unique_ptr<TypeMap> read_type(std::ifstream& data);
    std::unique_ptr<VarList> read_sweep(std::ifstream& data);
    std::unique_ptr<VarList> read_trace(std::ifstream& data);
    void read_values_no_swp(std::ifstream & data, H5::H5File * file, TypeMap * type_map);
    void read_values_swp_window(std::ifstream & data, H5::H5File * file, std::list<std::unique_ptr<H5::DataSet>> * dsets,
        uint32_t num_points, uint32_t windowsize, std::list<TypeDef> * type_list, TypeMap * type_map);
    void read_values_swp_simple(std::ifstream & data, H5::H5File * file, std::list<std::unique_ptr<H5::DataSet>> * dsets,
        uint32_t num_points, uint32_t max_data_size, std::list<TypeDef> * type_list, TypeMap * type_map);
    inline uint32_t read_section_preamble(std::ifstream & data, uint32_t section_code);
    inline void check_section_end(std::ifstream & data, uint32_t end_pos);
    inline void read_index(std::ifstream & data, bool is_trace);
    void write_properties(const PropDict & prop_dict, H5::H5Location * dset);

    void read_psf(const std::string& psf_filename, const std::string& hdf5_filename, bool print_msg) {
        read_psf(psf_filename, hdf5_filename, "", print_msg);
    }

    void read_psf(const std::string& psf_filename, const std::string& hdf5_filename,
        const std::string& log_filename, bool print_msg) {

        // set logging message format
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%level: %msg");
        // set to print to stdout
        std::string print_to_stdout = (print_msg) ? "true" : "false";
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, print_to_stdout);
        // set log file.
        if (log_filename == "") {
            if (!print_msg) {
                // just disable all logging
                el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Enabled, "false");
            }
            el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "false");
        }
        else {
            el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "true");
            el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename, log_filename);
        }

        // open PSF file
        std::ifstream data(psf_filename, std::ios::binary);
        if (!data.good()) {
            throw std::runtime_error("Error opening file.");
        }

        // open HDF5 file
        auto h5_file = std::unique_ptr<H5::H5File>(new H5::H5File(hdf5_filename.c_str(), H5F_ACC_TRUNC));

        // read first word and throw away
        uint32_t section_marker = read_uint32(data);
        LOG(TRACE) << "section marker = " << section_marker;
        LOG(TRACE) << "Reading header";
        auto prop_dict = read_header(data);

        // write header properties to file
        LOG(TRACE) << "Writing header to file";
        write_properties(*(prop_dict.get()), h5_file.get());

        section_marker = read_uint32(data);
        LOG(TRACE) << "section marker = " << section_marker;

        std::unique_ptr<TypeMap> type_map;
        if (section_marker == TYPE_START) {
            // read section.
            LOG(TRACE) << "Reading types";
            type_map = read_type(data);

            // read next section marker.
            section_marker = read_uint32(data);
            LOG(TRACE) << "section marker = " << section_marker;
        }

        std::unique_ptr<VarList> sweep_list;
        if (section_marker == SWEEP_START) {
            // read section.
            LOG(TRACE) << "Reading sweeps";
            sweep_list = read_sweep(data);

            // read next section marker.
            section_marker = read_uint32(data);
            LOG(TRACE) << "section marker = " << section_marker;
        }

        std::unique_ptr<VarList> trace_list;
        if (section_marker == TRACE_START) {
            // read section.
            LOG(TRACE) << "Reading traces";
            trace_list = read_trace(data);

            // read next section marker.
            section_marker = read_uint32(data);
            LOG(TRACE) << "section marker = " << section_marker;
        }

        // make sure that we are reading value section next
        if (section_marker != VALUE_START) {
            std::ostringstream builder;
            builder << "Error: section marker is not equal to value section ID = " << VALUE_START;
            throw std::runtime_error(builder.str());
        }

        // check we have at least one sweep variable.
        if (sweep_list == nullptr || sweep_list->size() == 0) {
            LOG(TRACE) << "Reading values (No sweep)";
            read_values_no_swp(data, h5_file.get(), type_map.get());
        }
        else {

            // check that we have exactly one sweep variable.
            if (sweep_list->size() > 1) {
                throw std::runtime_error("Non-single sweep PSF file is not supported.  If you use ADEXL for parametric sweep this shouldn't happen.");
            }

            // get window size
            uint32_t win_size;
            auto prop_iter = prop_dict->find("PSF window size");
            if (prop_iter == prop_dict->end()) {
                win_size = 0;
            }
            else {
                win_size = prop_iter->second.m_ival;
            }

            // check that number of sweep points is recorded.
            prop_iter = prop_dict->find("PSF sweep points");
            if (prop_iter == prop_dict->end()) {
                throw std::runtime_error("Cannot find PSF property \"PSF sweep points\".");
            }
            uint32_t num_points_data = prop_iter->second.m_ival;

            // check that sweep variable type is supported
            const Variable & swp_var = sweep_list->front();
            const TypeDef & swp_type = type_map->at(swp_var.m_type_id);
            if (!swp_type.m_is_supported) {
                std::ostringstream builder;
                builder << "Sweep variable " << swp_var.m_name <<
                    " with type \"" << swp_type.m_name << "\" (data type = " <<
                    swp_type.m_type_name << " ) is not supported.";
                throw std::runtime_error(builder.str());
            }

            // check that all output variable types are supported and legal.
            for (auto output : *trace_list.get()) {
                const TypeDef & output_type = type_map->at(output.m_type_id);
                if (!output_type.m_is_supported) {
                    std::ostringstream builder;
                    builder << "Output variable " << output.m_name <<
                        " with type \"" << output_type.m_name << "\" (data type = " <<
                        output_type.m_type_name << " ) is not supported.";
                    throw std::runtime_error(builder.str());
                }
                if (win_size > 0 &&
                    (swp_type.m_h5_read_type.getSize() != output_type.m_h5_read_type.getSize())) {
                    // for windowed sweep, make sure sweep and all output variables
                    // have the same data size.
                    std::ostringstream builder;
                    builder << "Output variable " << output.m_name <<
                        " with type \"" << output_type.m_name << "\" (data type = " <<
                        output_type.m_type_name << " ) has a data size different than" <<
                        "sweep variable " << swp_var.m_name <<
                        " with type \"" << swp_type.m_name << "\" (data type = " <<
                        swp_type.m_type_name << " ).  This is not expected.  " <<
                        "Please send your PSF File to developers for debugging.";
                    throw std::runtime_error(builder.str());
                }
            }

            // append the only sweep variable to front of trace list
            trace_list->push_front(sweep_list->front());

            // create output datasets
            hsize_t file_dim[1] = { num_points_data };
            size_t max_data_size = 0;
            H5::DataSpace file_space(1, file_dim, file_dim);
            auto out_types = std::unique_ptr<std::list<TypeDef>>(new std::list<TypeDef>());
            auto out_dsets = std::unique_ptr<std::list<std::unique_ptr<H5::DataSet>>>(
                new std::list<std::unique_ptr<H5::DataSet>>());
            for (auto var : *trace_list) {
                LOG(TRACE) << "Create " << var.m_name << " dataset";
                const TypeDef & out_type = type_map->at(var.m_type_id);
                max_data_size = std::max(max_data_size, out_type.m_h5_read_type.getSize());
                auto out_dset = std::unique_ptr<H5::DataSet>(new H5::DataSet(h5_file->createDataSet(var.m_name.c_str(),
                    out_type.m_h5_write_type, file_space)));
                // write output properties to file.
                LOG(TRACE) << "Write " << var.m_name << " properties";
                write_properties(var.m_prop_dict, out_dset.get());
                out_dsets->push_back(std::move(out_dset));
                out_types->push_back(out_type);
            }

            if (win_size == 0) {
                LOG(TRACE) << "Reading values (sweep simple)";
                read_values_swp_simple(data, h5_file.get(), out_dsets.get(), num_points_data,
                    max_data_size, out_types.get(), type_map.get());
            }
            else {
                LOG(TRACE) << "Reading values (sweep windowed)";
                read_values_swp_window(data, h5_file.get(), out_dsets.get(), num_points_data,
                    win_size, out_types.get(), type_map.get());
            }

            // close all datasets
            for (auto itd = out_dsets->begin(); itd != out_dsets->end(); ++itd) {
                (*itd)->close();
            }
        }

        LOG(TRACE) << "Finished reading PSF file.";
        h5_file->close();
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

        check_section_end(data, end_pos);

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
        check_section_end(data, end_pos);

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

        LOG(TRACE) << "Reading sweep types";
        auto ans = std::unique_ptr<VarList>(new VarList());
        bool valid_type = true;
        while (valid_type) {
            Variable temp;
            valid_type = temp.read(data);
            if (valid_type) {
                ans->push_back(temp);
            }
        }

        check_section_end(data, end_pos);

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
            }
            else {
                // Group failed, try reading as Variable.
                Variable var;
                valid_type = var.read(data);
                if (valid_type) {
                    ans->push_back(var);
                }
            }
        }

        read_index(data, true);
        check_section_end(data, end_pos);

        return ans;
    }

    /**
    * This functions reads the value section when no sweep is defined, and save
    * results to HDF5 file.
    *
    * subsection{
    * NonsweepValue val1
    * NonsweepValue val2
    * ...
    * }
    * int index_type
    * int index_size
    * int index_id1
    * int index_offset1
    * int index_id2
    * int index_offset2
    * ...
    */
    void read_values_no_swp(std::ifstream & data, H5::H5File * file, TypeMap * type_map) {
        uint32_t end_pos = read_section_preamble(data, MAJOR_SECTION_CODE);
        uint32_t sub_end_pos = read_section_preamble(data, MINOR_SECTION_CODE);

        hsize_t file_dim[1] = { 1 };
        H5::DataSpace file_space(1, file_dim, file_dim);
        H5::DataSpace buf_space(1, file_dim, file_dim);

        bool valid = true;
        while (valid && static_cast<uint32_t>(data.tellg()) < sub_end_pos) {
            uint32_t code = read_uint32(data);
            LOG(TRACE) << "value code = " << code;
            valid = (NONSWP_VAL_SECTION_CODE == code);
            if (valid) {
                uint32_t var_id = read_uint32(data);
                LOG(TRACE) << "Var id = " << var_id;
                std::string var_name = read_str(data);
                LOG(TRACE) << "Var name = " << var_name;
                uint32_t type_id = read_uint32(data);
                LOG(TRACE) << "Var type id = " << type_id;
                const TypeDef & var_type = type_map->at(type_id);
                LOG(TRACE) << "Var type = " << var_type.m_name << ", " << var_type.m_type_name;

                // check var_type is supported.
                if (!var_type.m_is_supported) {
                    std::ostringstream builder;
                    builder << "Output variable " << var_name <<
                        " with type \"" << var_type.m_name << "\" (data type = " <<
                        var_type.m_type_name << " ) is not supported.";
                    throw std::runtime_error(builder.str());
                }

                // create output dataset
                auto dset = std::unique_ptr<H5::DataSet>(new H5::DataSet(
                    file->createDataSet(var_name.c_str(), var_type.m_h5_write_type, file_space)));

                // read data into buffer
                size_t data_size = var_type.m_h5_read_type.getSize();
                auto buf = std::unique_ptr<char[]>(new char[data_size]);
                data.read(buf.get(), data_size);

                // write to dataset
                dset->write(buf.get(), var_type.m_h5_read_type, buf_space, file_space);

                // read properties
                PropDict prop_dict;
                prop_dict.read(data);

                // write properties to file.
                write_properties(prop_dict, dset.get());

                // close dataset
                dset->close();
            }
        }

        // read rest of the variable section.
        read_index(data, false);
        check_section_end(data, end_pos);
    }

    void read_values_swp_window(std::ifstream & data, H5::H5File * file, std::list<std::unique_ptr<H5::DataSet>> * dsets,
        uint32_t num_points, uint32_t windowsize, std::list<TypeDef> * type_list, TypeMap * type_map) {

        read_section_preamble(data, MAJOR_SECTION_CODE);

        // skip zero paddings
        uint32_t zp_code = read_uint32(data);
        LOG(TRACE) << "zero padding code = " << zp_code;
        uint32_t zp_size = read_uint32(data);
        LOG(TRACE) << "zero padding size = " << zp_size << ", skipping";
        data.seekg(zp_size, std::ios::cur);

        // read window info
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
        LOG(TRACE) << "Size word left value = " << size_left;
        LOG(TRACE) << "Number of valid data in window = " << np_window;

        // setup HDF5 needed info
        hsize_t file_dim[1] = { num_points };
        H5::DataSpace file_space(1, file_dim, file_dim);

        hsize_t mem_dim[1] = { np_window };
        hsize_t file_count[1] = { np_window };
        hsize_t mem_count[1] = { np_window };
        hsize_t file_offset[1] = { 0 };
        hsize_t zero_offset[1] = { 0 };
        hsize_t unit_step[1] = { 1 };
        H5::DataSpace mem_space(1, mem_dim, mem_dim);

        // start data transfer
        LOG(TRACE) << "Transferring data";
        uint32_t points_read = 0;
        auto buffer = std::unique_ptr<char[]>(new char[windowsize]);
        while (points_read < num_points) {
            // update HDF5 file offset
            file_offset[0] = points_read;
            file_count[0] = std::min(np_window, num_points - points_read);
            file_space.selectHyperslab(H5S_SELECT_SET, file_count, file_offset, unit_step, unit_step);
            // update data count
            mem_count[0] = file_count[0];
            mem_space.selectHyperslab(H5S_SELECT_SET, mem_count, zero_offset, unit_step, unit_step);

            // write output values
            auto itv = type_list->begin();
            auto itd = dsets->begin();
            for (; itv != type_list->end(); ++itv, ++itd) {
                data.read(buffer.get(), windowsize);
                (*itd)->write(buffer.get(), (*itv).m_h5_read_type, mem_space, file_space);
            }
            // update number of points read
            points_read += mem_count[0];
        }
    }

    void read_values_swp_simple(std::ifstream & data, H5::H5File * file, std::list<std::unique_ptr<H5::DataSet>> * dsets,
        uint32_t num_points, uint32_t max_data_size, std::list<TypeDef> * type_list, TypeMap * type_map) {

        read_section_preamble(data, MAJOR_SECTION_CODE);

        // setup HDF5 needed info
        hsize_t file_dim[1] = { num_points };
        H5::DataSpace file_space(1, file_dim, file_dim);

        hsize_t mem_dim[1] = { 1 };
        hsize_t count[1] = { 1 };
        hsize_t file_offset[1] = { 0 };
        hsize_t unit_step[1] = { 1 };
        H5::DataSpace mem_space(1, mem_dim, mem_dim);

        // start data transfer
        LOG(TRACE) << "Transferring data";
        uint32_t points_read = 0;
        auto buffer = std::unique_ptr<char[]>(new char[max_data_size]);
        while (points_read < num_points) {
            // update HDF5 file offset
            file_offset[0] = points_read;
            file_space.selectHyperslab(H5S_SELECT_SET, count, file_offset, unit_step, unit_step);

            // write output values
            auto itv = type_list->begin();
            auto itd = dsets->begin();
            for (; itv != type_list->end(); ++itv, ++itd) {
                uint32_t code = read_uint32(data);
                uint32_t var_id = read_uint32(data);
                uint32_t cur_size = (*itv).m_h5_read_type.getSize();
                data.read(buffer.get(), cur_size);
                (*itd)->write(buffer.get(), (*itv).m_h5_read_type, mem_space, file_space);
            }
            // update number of points read
            points_read += 1;
        }
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
        LOG(TRACE) << "section end position = " << end_pos <<
            ", current position = " << data.tellg();

        return end_pos;
    }

    /**
     * Read the second end.
     *
     * section end format:
     * int marker = end_marker.
     */
    inline void check_section_end(std::ifstream & data, uint32_t end_pos) {
        uint32_t cur_pos = static_cast<uint32_t>(data.tellg()) + sizeof(uint32_t);
        if (cur_pos != end_pos) {
            std::ostringstream builder;
            builder << "Section end position = " << cur_pos <<
                " is not " << end_pos << ", something's wrong";
            throw std::runtime_error(builder.str());
        }
    }

    /**
     * Read the index section.
     *
     */
    inline void read_index(std::ifstream & data, bool is_trace) {
        uint32_t index_type = read_uint32(data);
        LOG(TRACE) << "Type index type = " << index_type;
        uint32_t index_size = read_uint32(data);
        LOG(TRACE) << "Type index size = " << index_size;
        if (is_trace) {
            // read trace information
            int32_t id, offset, extra1, extra2;
            for (uint32_t i = 0; i < index_size; i += 4 * WORD_SIZE) {
                id = read_int32(data);
                offset = read_int32(data);
                extra1 = read_int32(data);
                extra2 = read_int32(data);
                LOG(TRACE) << "trace index: (0x" << std::hex << std::setfill('0') << id << std::dec <<
                    ", " << offset << ", " << extra1 << ", " << extra2 << ")";
            }
        }
        else {
            // read index information
            int32_t id, offset;
            for (uint32_t i = 0; i < index_size; i += 2 * WORD_SIZE) {
                id = read_int32(data);
                offset = read_int32(data);
                LOG(TRACE) << "index: (" << id << ", " << offset << ")";
            }
        }

    }

    void write_properties(const PropDict & prop_dict, H5::H5Location * dset) {
        // write properties as attributes to dataset.
        for (auto entry : prop_dict) {
            H5::DataSpace attr_space = H5::DataSpace(H5S_SCALAR);
            H5::Attribute attr;
            std::ostringstream builder;
            H5::StrType stype;
            size_t len;
            switch (entry.second.m_type) {
            case Property::type::INT:
                attr = dset->createAttribute(entry.second.m_name,
                    H5::PredType::STD_I32LE, attr_space);
                attr.write(H5::PredType::STD_I32LE, &entry.second.m_ival);
                break;
            case Property::type::DOUBLE:
                attr = dset->createAttribute(entry.second.m_name,
                    H5::PredType::IEEE_F64LE, attr_space);
                attr.write(H5::PredType::IEEE_F64LE, &entry.second.m_dval);
                break;
            case Property::type::STRING:
                len = entry.second.m_sval.length();
                stype = H5::StrType(H5::PredType::C_S1, len);
                attr = dset->createAttribute(entry.second.m_name, stype, attr_space);
                attr.write(stype, entry.second.m_sval.c_str());
                break;
            default:
                builder << "Unknown property type ID: " << entry.second.m_type;
                throw new std::runtime_error(builder.str());
            }

            // close attribute
            attr.close();
        }
    }

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
