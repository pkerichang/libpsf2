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
    static constexpr uint32_t HEADER_END = 1;
    static constexpr uint32_t TYPE_END = 2;
    static constexpr uint32_t SWEEP_END = 3;
    static constexpr uint32_t TRACE_END = 4;
    
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
    
	void read_psf(std::string fname);

    uint32_t read_section_preamble(std::ifstream & data);
    
    std::unique_ptr<PropDict> read_header(std::ifstream & data);

    std::unique_ptr<TypeMap> read_type(std::ifstream & data);

    // really the same as read_header
    std::unique_ptr<VarList> read_sweep(std::ifstream & data);

    // really the same as read_type_section
    std::unique_ptr<VarList> read_trace(std::ifstream & data);

    void read_values_swp_window(std::ifstream & data, uint32_t num_points,
		uint32_t windowsize, const Variable & swp_var, 
		VarList * trace_list, TypeMap * type_map);
    
}

    
#endif
