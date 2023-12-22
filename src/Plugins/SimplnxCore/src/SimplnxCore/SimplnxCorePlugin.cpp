#include "SimplnxCorePlugin.hpp"
#include "SimplnxCoreLegacyUUIDMapping.hpp"

#include "SimplnxCore/SimplnxCore_filter_registration.hpp"

using namespace nx::core;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

SimplnxCorePlugin::SimplnxCorePlugin()
: AbstractPlugin(k_ID, "SimplnxCore", "Description", "BlueQuartz Software")
{
  std::vector<nx::core::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

SimplnxCorePlugin::~SimplnxCorePlugin() = default;

AbstractPlugin::SIMPLMapType SimplnxCorePlugin::getSimplToSimplnxMap() const
{
  return nx::core::k_SIMPL_to_SimplnxCore;
}

SIMPLNX_DEF_PLUGIN(SimplnxCorePlugin)
