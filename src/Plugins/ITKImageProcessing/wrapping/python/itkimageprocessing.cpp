#include <CxPybind/CxPybind.hpp>

#include "ITKImageProcessing/ITKImageProcessingFilterBinding.hpp"
#include "ITKImageProcessing/ITKImageProcessingPlugin.hpp"

using namespace nx::core;
using namespace nx::core::CxPybind;
namespace py = pybind11;

PYBIND11_MODULE(itkimageprocessing, mod)
{
  py::module_::import("simplnx");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<ITKImageProcessingPlugin>();

  ITKImageProcessing::BindFilters(mod, internals);

  internals.registerPluginPyFilters(*plugin);
}
