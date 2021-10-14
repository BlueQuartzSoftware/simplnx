#include "ImageProcessingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("bc34d68e-861d-580d-ae07-30a41afc0f09");
} // namespace

ImageProcessingPlugin::ImageProcessingPlugin()
: AbstractPlugin(k_ID, "ImageProcessing", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ImageProcessingPlugin::~ImageProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImageProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImageProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ImageProcessing/plugin_filter_registration.h"
