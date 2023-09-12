#include <CxPybind/CxPybind.hpp>

#include <ITKImageProcessing/ITKImageProcessingPlugin.hpp>

#include "ITKImageProcessing/ITKImageProcessingFilterBinding.hpp"

using namespace complex;
using namespace complex::CxPybind;
namespace py = pybind11;

PYBIND11_MODULE(itkimageprocessing, mod)
{
  py::module_::import("complex");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<ITKImageProcessingPlugin>();

  ITKImageProcessing::BindFilters(mod, internals);

  internals.registerPluginPyFilters(*plugin);
}
