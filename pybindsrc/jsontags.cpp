/**
 * @file jsontags.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2022.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include "opmonlib/JSONTags.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


namespace py = pybind11;

namespace dunedaq::opmonlib {
namespace python {

void register_jsontags(py::module& m)
{

  py::class_<opmonlib::JSONTags>(m,"JSONTags")   
    .def("tags", &JSONTags::tags) 
    .def("parent", &JSONTags::parent) 
    .def("time", &JSONTags::time) 
    .def("data", &JSONTags::data)
    .def("children", &JSONTags::children) 
    .def("properties", &JSONTags::properties); 

} 


} // namespace python
} // namespace dunedaq::opmonlib