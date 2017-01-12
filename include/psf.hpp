#ifndef LIBPSF_PSF_H_
#define LIBPSF_PSF_H_

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
#include <boost/asio.hpp>

#ifdef DEBUG
#include <iostream>
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

#define WORD_SIZE sizeof(uint32_t)
#define GET_I32(buf) ntohl(*(reinterpret_cast<const uint32_t*>(buf)))
#define GET_I64(buf) be64toh(*(reinterpret_cast<const uint64_t*>(buf)))



namespace psf {

    typedef boost::variant<char, int, double, std::complex<double>, std::string> PSFScalar;
    typedef std::vector<PSFScalar> PSFVector;
    typedef std::vector<std::string> StrVector;
    
    typedef std::unordered_map<std::string, PSFScalar> PropDict;
    typedef std::unordered_map<std::string, std::unique_ptr<PropDict>> NestPropDict;
    typedef std::unordered_map<std::string, std::unique_ptr<PSFVector>> VecDict;


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
        
        int m_num_sweeps;
        int m_num_points;
        bool m_swept;
        bool m_invert_struct;
    };

    inline std::string deserialize_str(const char* start, std::size_t& num_read) {
        uint32_t len = GET_I32(start);
        // align to word boundary (4 bytes)
        num_read += WORD_SIZE + len + ((WORD_SIZE - len) & 3);
        return std::string(start + WORD_SIZE, len);
    }
    
    inline uint32_t deserialize_int(const char* start, std::size_t& num_read) {
        num_read += WORD_SIZE;
        return GET_I32(start);
    }
    
    inline uint64_t deserialize_double(const char* start, std::size_t& num_read) {
        num_read += sizeof(uint64_t);
        return GET_I64(start);
    }

    // Decodes a single name-value entry.
    std::size_t deserialize_entry(const char* start, uint32_t end_marker, PropDict* pdict);
    std::size_t deserialize_prop_section(const char* start, std::size_t start_idx, uint32_t ps_type,
                                         uint32_t end_marker, PropDict* pdict);
}

    
#endif
