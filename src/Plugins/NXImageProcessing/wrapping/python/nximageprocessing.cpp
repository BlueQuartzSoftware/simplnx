#include <NxPybind/NxPybind.hpp>

#include "ImageProcessing/ImageProcessingFilterBinding.hpp"
#include "ImageProcessing/ImageProcessingPlugin.hpp"

using namespace nx::core;
using namespace nx::core::NxPybind;
namespace py = pybind11;

PYBIND11_MODULE(imageprocessing, mod)
{
  py::module_::import("simplnx");

  auto& internals = Internals::Instance();
  internals.addPlugin<ImageProcessingPlugin>();

  ImageProcessing::BindFilters(mod, internals);
}
