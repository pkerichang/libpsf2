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
#include <fstream>
#include <stdexcept>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/variant.hpp>
#include <boost/format.hpp>

#include "psfproperty.hpp"
#include "psftypes.hpp"

namespace psf {

    static const uint32_t MAJOR_SECTION_CODE = 21;
    static const uint32_t MINOR_SECTION_CODE = 22;
	static const uint32_t SWP_WINDOW_SECTION_CODE = 16;
    static const uint32_t HEADER_END = 1;
    static const uint32_t TYPE_END = 2;
    static const uint32_t SWEEP_END = 3;
    static const uint32_t TRACE_END = 4;

    typedef boost::variant<int8_t, int32_t, double, std::complex<double>, std::string> PSFScalar;
    typedef std::vector<PSFScalar> PSFVector;
    typedef std::unordered_map<std::string, std::unique_ptr<PSFVector>> VecDict;
    
    // a value in a non-sweep simulation result.
    class NonSweepValue {
    public:
        static const uint32_t code = 16;

        NonSweepValue();
        NonSweepValue(uint32_t id, std::string name, uint32_t type_id,
                      PSFScalar value, PropDict prop_dict);
        
        ~NonSweepValue();

    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_type_id;
        PSFScalar m_value;
        PropDict m_prop_dict;
    };  
    
	void read_psf(std::string fname);

    uint32_t read_section_preamble(char *&data);
    
    std::unique_ptr<PropDict> read_header(char *& data, const char * orig);

    std::unique_ptr<TypeMap> read_type(char *& data, const char * orig);

    // really the same as read_header
    std::unique_ptr<VarList> read_sweep(char *& data, const char * orig);

    // really the same as read_type_section
    std::unique_ptr<VarList> read_trace(char *& data, const char * orig);

    void read_values_swp_window(char *& data, const char * orig, uint32_t num_points, uint32_t windowsize);
    
}

    
#endif
