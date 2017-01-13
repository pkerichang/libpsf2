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
    static const uint32_t HEADER_END = 1;
    static const uint32_t TYPE_END = 2;
    static const uint32_t SWEEP_END = 3;
    static const uint32_t TRACE_END = 4;

    typedef std::vector<std::string> StrVector;

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
    
    // a class representing the PSF file.
    class PSFDataSet {
    public:
        PSFDataSet(std::string filename);
        PSFDataSet(std::string filename, bool invert_struct);
        ~PSFDataSet();
        
        PropDict* get_prop_dict() const;
        NestPropDict* get_scalar_prop_dict() const;
        
        StrVector* get_sweep_names() const;
        PSFVector* get_sweep_values() const;
        
        int get_num_sweeps() const;
        int get_num_points() const;
        bool is_swept() const;
        bool is_struct_inverted() const;
        
        PropDict* get_scalar_dict() const;
        VecDict* get_vector_dict() const;
        
    private:
        void open(std::string filename);
        
        std::unique_ptr<PropDict> m_prop_dict;
        std::unique_ptr<NestPropDict> m_scalar_prop_dict;
        std::unique_ptr<PropDict> m_scalar_dict;
        std::unique_ptr<VecDict> m_vector_dict;
        std::unique_ptr<StrVector> m_swp_vars;
        std::unique_ptr<PSFVector> m_swp_vals;
        std::unique_ptr<TypeMap> m_type_map;
        std::unique_ptr<VarList> m_sweep_list;
        std::unique_ptr<TraceList> m_trace_list;
        
        int m_num_sweeps;
        int m_num_points;
        bool m_swept;
        bool m_invert_struct;
    };

    uint32_t read_section_preamble(char *&data);
    
    std::unique_ptr<PropDict> read_header(char *& data, const char * orig);

    std::unique_ptr<TypeMap> read_type(char *& data, const char * orig);

    // really the same as read_header
    std::unique_ptr<VarList> read_sweep(char *& data, const char * orig);

    // really the same as read_type_section
    std::unique_ptr<TraceList> read_trace(char *& data, const char * orig);

    void read_sweep_values_test(char *& data, const char * orig);
    
}

    
#endif
