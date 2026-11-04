#include <NxPybind/NxPybind.hpp>

#include "NXImageProcessing/NXImageProcessingFilterBinding.hpp"
#include "NXImageProcessing/NXImageProcessingPlugin.hpp"

using namespace nx::core;
using namespace nx::core::NxPybind;
namespace py = pybind11;

PYBIND11_MODULE(nximageprocessing, mod)
{
  py::module_::import("simplnx");

  auto& internals = Internals::Instance();
  internals.addPlugin<NXImageProcessingPlugin>();

  NXImageProcessing::BindFilters(mod, internals);
}
