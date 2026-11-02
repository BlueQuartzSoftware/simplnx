#include "ImageProcessingPlugin.hpp"
#include "ImageProcessing/ImageProcessingLegacyUUIDMapping.hpp"
#include "ImageProcessing/ImageProcessing_filter_registration.hpp"

using namespace nx::core;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("6f2d9b14-8e3a-4c7f-a1d2-5b90e6c4732a");
} // namespace

ImageProcessingPlugin::ImageProcessingPlugin()
: AbstractPlugin(k_ID, "ImageProcessing", "ITK-free image processing filters", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

ImageProcessingPlugin::~ImageProcessingPlugin() = default;

AbstractPlugin::SIMPLMapType ImageProcessingPlugin::getSimplToSimplnxMap() const
{
  return nx::core::k_SIMPL_to_ImageProcessing;
}

AbstractPlugin::FilterReplacementMapType ImageProcessingPlugin::getFilterReplacementMap() const
{
  return nx::core::k_ITK_to_ImageProcessing_Replacements;
}

SIMPLNX_DEF_PLUGIN(ImageProcessingPlugin)
