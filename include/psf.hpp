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
#include <boost/format.hpp>

#define DEBUG 1

#ifdef DEBUG
#include <iostream>
#define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do { } while ( false )
#endif

namespace psf {

    static const uint32_t MAJOR_SECTION_CODE = 21;
    static const uint32_t MINOR_SECTION_CODE = 22;
    static const uint32_t WORD_SIZE = sizeof(uint32_t);
    static const uint32_t HEADER_END = 1;
    static const uint32_t TYPE_END = 2;
    static const uint32_t SWEEP_END = 3;
    
    typedef boost::variant<int8_t, int32_t, double, std::complex<double>, std::string> PSFScalar;
    typedef std::vector<PSFScalar> PSFVector;
    typedef std::vector<std::string> StrVector;

    typedef std::pair<std::string, PSFScalar> PropEntry;
    typedef std::unordered_map<std::string, PSFScalar> PropDict;
    typedef std::unordered_map<std::string, std::unique_ptr<PropDict>> NestPropDict;
    typedef std::unordered_map<std::string, std::unique_ptr<PSFVector>> VecDict;

    // a class representing a type definition.
    class TypeDef {
    public:
        static const uint32_t code = 16;
        static const uint32_t struct_code = 16;
        static const uint32_t tuple_code = 18;

        TypeDef();
        TypeDef(uint32_t id, std::string name, uint32_t array_type, uint32_t data_type,
                std::vector<TypeDef> typedef_tuple, PropDict prop_dict);
        
        ~TypeDef();

    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_array_type;
        uint32_t m_data_type;
        std::vector<TypeDef> m_typedef_tuple;        
        PropDict m_prop_dict;
    };

    typedef std::vector<TypeDef> TypeList;

    // a class referencing a defined type
    class TypePointer {
    public:
        static const uint32_t code = 16;
        
        TypePointer();
        TypePointer(uint32_t id, std::string name, uint32_t type_id, PropDict prop_dict);
        
        ~TypePointer();

    private:
        uint32_t m_id;
        std::string m_name;
        uint32_t m_type_id;
        PropDict m_prop_dict;
    };

    typedef std::vector<TypePointer> TypePtrList;
    
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
        std::unique_ptr<TypeList> m_type_list;
        std::unique_ptr<TypePtrList> m_sweep_list;
        
        int m_num_sweeps;
        int m_num_points;
        bool m_swept;
        bool m_invert_struct;
    };
    

    inline uint32_t read_uint32(char *& data) {
        uint32_t ans = ntohl(*(reinterpret_cast<uint32_t*>(data)));
        data += WORD_SIZE;
        return ans;
    }
    
    inline uint32_t peek_uint32(const char * data, std::size_t& num_read) {
        num_read += WORD_SIZE;
        return ntohl(*(reinterpret_cast<const uint32_t*>(data)));
    }

    inline int32_t read_int32(char *& data) {
        return static_cast<int32_t>(read_uint32(data));
    }

    inline int8_t read_int8(char *& data) {
        return static_cast<int8_t>(read_uint32(data));
    }
    
    inline double read_double(char *& data) {
        uint64_t ans = be64toh(*(reinterpret_cast<uint64_t*>(data)));
        data += sizeof(uint64_t);
        return *reinterpret_cast<double*>(&ans);
    }
    
    inline std::string read_str(char *& data) {
        uint32_t len = read_int32(data);
        std::string ans(data, len);
        // align to word boundary (4 bytes)
        data += (len + 3) & ~0x03;
        return ans;
    }

    PropEntry read_entry(char *& data, bool& valid);
    
    TypeDef read_type(char *& data, bool& valid);
    
    TypeList read_type_list(char *& data);

    TypePointer read_type_pointer(char *& data, bool& valid);
    
    std::unique_ptr<PropDict> read_header(char *& data, const char * orig);

    std::unique_ptr<TypeList> read_type_section(char *& data, const char * orig, bool is_trace);

    std::unique_ptr<TypePtrList> read_sweep(char *& data, const char * orig);


}

    
#endif
