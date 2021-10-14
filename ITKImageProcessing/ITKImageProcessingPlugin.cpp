#include "ITKImageProcessingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("67ab1c35-e639-5852-a963-ea317e1f63fb");
} // namespace

ITKImageProcessingPlugin::ITKImageProcessingPlugin()
: AbstractPlugin(k_ID, "ITKImageProcessing", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ITKImageProcessingPlugin::~ITKImageProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ITKImageProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ITKImageProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ITKImageProcessing/plugin_filter_registration.h"
