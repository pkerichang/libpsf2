#ifndef _PSF
#define _PSF

#include <vector>
#include <string>
#include <complex>
#include <unordered_map>
#include <memory>
#include <boost/variant.hpp>


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

#endif
