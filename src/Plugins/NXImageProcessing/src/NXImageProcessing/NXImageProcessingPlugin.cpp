#include "NXImageProcessingPlugin.hpp"
#include "NXImageProcessing/NXImageProcessingLegacyUUIDMapping.hpp"
#include "NXImageProcessing/NXImageProcessing_filter_registration.hpp"

using namespace nx::core;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("6f2d9b14-8e3a-4c7f-a1d2-5b90e6c4732a");
} // namespace

NXImageProcessingPlugin::NXImageProcessingPlugin()
: AbstractPlugin(k_ID, "NXImageProcessing", "ITK-free image processing filters", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

NXImageProcessingPlugin::~NXImageProcessingPlugin() = default;

AbstractPlugin::SIMPLMapType NXImageProcessingPlugin::getSimplToSimplnxMap() const
{
  return nx::core::k_SIMPL_to_NXImageProcessing;
}

AbstractPlugin::FilterReplacementMapType NXImageProcessingPlugin::getFilterReplacementMap() const
{
  return nx::core::k_ITK_to_NXImageProcessing_Replacements;
}

SIMPLNX_DEF_PLUGIN(NXImageProcessingPlugin)
