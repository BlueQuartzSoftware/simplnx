#include <pybind11/pybind11.h>

#include <complex/Filter/IFilter.hpp>

using namespace complex;
namespace py = pybind11;

PYBIND11_MODULE(complex, mod)
{
  py::class_<IFilter> filter(mod, "IFilter");
  filter.def("name", &IFilter::name);
  filter.def("uuid", &IFilter::uuid);
  filter.def("humanName", &IFilter::humanName);
}
