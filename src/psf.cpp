#include <fstream>
#include <stdexcept>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "psf.hpp"
#include "bindata.hpp"

namespace bip = boost::interprocess;


PSFDataSet::PSFDataSet(std::string filename) : m_invert_struct(false) {
    open(filename);
}

PSFDataSet::PSFDataSet(std::string filename, bool invert_struct) : m_invert_struct(invert_struct) {
    open(filename);
}

PSFDataSet::~PSFDataSet() {}


void PSFDataSet::open(std::string filename) {
    m_prop_dict = std::unique_ptr<PropDict>(new PropDict());
    m_scalar_prop_dict = std::unique_ptr<NestPropDict>(new NestPropDict());
    m_scalar_dict = std::unique_ptr<PropDict>(new PropDict());
    m_vector_dict = std::unique_ptr<VecDict>(new VecDict());
    m_swp_vars = std::unique_ptr<StrVector>(new StrVector());
    m_swp_vals = std::unique_ptr<PSFVector>(new PSFVector());

    m_num_sweeps = 1;
    m_num_points = 1;
    m_swept = false;

    bip::file_mapping mapping(filename.c_str(), bip::read_only);
    bip::mapped_region mapped_rgn(mapping, bip::read_only);
    const char* mmap_data = static_cast<char*>(mapped_rgn.get_address());
    std::size_t const mmap_size = mapped_rgn.get_size();
    
    PropSection header(21, 1);
    header.deserialize(mmap_data + 4, 4, m_prop_dict.get());
}

PropDict* PSFDataSet::get_prop_dict() const {
    return m_prop_dict.get();
}

NestPropDict* PSFDataSet::get_scalar_prop_dict() const {
    return m_scalar_prop_dict.get();
}

StrVector* PSFDataSet::get_sweep_names() const {
    return m_swp_vars.get();
}

PSFVector* PSFDataSet::get_sweep_values() const {
    return m_swp_vals.get();
}

int PSFDataSet::get_num_sweeps() const {
    return m_num_sweeps;
}

int PSFDataSet::get_num_points() const {
    return m_num_points;
}

bool PSFDataSet::is_swept() const {
    return m_swept;
}

bool PSFDataSet::is_struct_inverted() const {
    return m_invert_struct;
}

PropDict* PSFDataSet::get_scalar_dict() const {
    return m_scalar_dict.get();
}

VecDict* PSFDataSet::get_vector_dict() const {
    return m_vector_dict.get();
}
