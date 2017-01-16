#ifndef LIBPSF_PSF_H_
#define LIBPSF_PSF_H_

/**
 *  Main header file for psf.
 */


#include <vector>
#include <string>
#include <complex>
#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "H5Cpp.h"

#include "psfproperty.hpp"
#include "psftypes.hpp"

namespace psf {

    static constexpr uint32_t MAJOR_SECTION_CODE = 21;
    static constexpr uint32_t MINOR_SECTION_CODE = 22;
    static constexpr uint32_t SWP_WINDOW_SECTION_CODE = 16;
    static constexpr uint32_t NONSWP_VAL_SECTION_CODE = 16;
    static constexpr uint32_t TYPE_START = 1;
    static constexpr uint32_t SWEEP_START = 2;
    static constexpr uint32_t TRACE_START = 3;
    static constexpr uint32_t VALUE_START = 4;

    // a value in a non-sweep simulation result.
    class NonSweepValue {
    public:
        static constexpr uint32_t code = 16;

        NonSweepValue() {}
        ~NonSweepValue() {}

    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_type_id;
        int8_t m_cval;
        int32_t m_ival;
        double m_dval;
        std::string m_sval;
        PropDict m_prop_dict;
    };

    void read_psf(const std::string& psf_filename, const std::string& hdf5_filename,
        bool print_msg = false);

    void read_psf(const std::string& psf_filename, const std::string& hdf5_filename,
        const std::string& log_filename, bool print_msg = false);

    uint32_t read_section_preamble(std::ifstream& data);

    std::unique_ptr<PropDict> read_header(std::ifstream& data);

    std::unique_ptr<TypeMap> read_type(std::ifstream& data);

    std::unique_ptr<VarList> read_sweep(std::ifstream& data);

    std::unique_ptr<VarList> read_trace(std::ifstream& data);

    void read_values_no_swp(std::ifstream & data, H5::H5File * file, TypeMap * type_map);

    void read_values_swp_window(std::ifstream & data, H5::H5File * file, uint32_t num_points,
        uint32_t windowsize, const Variable & swp_var, VarList * trace_list, TypeMap * type_map);

}


#endif
