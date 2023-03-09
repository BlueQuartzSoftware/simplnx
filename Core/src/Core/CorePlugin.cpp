#include "CorePlugin.hpp"
#include "CoreLegacyUUIDMapping.hpp"

#include "Core/Core_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_CoreToComplexFilterMapping = {};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("65a0a3fc-8c93-5405-8ac6-182e7f313a69");
} // namespace

CorePlugin::CorePlugin()
: AbstractPlugin(k_ID, "Core", "<<--Description was not read-->>", "OpenSource")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

CorePlugin::~CorePlugin() = default;

std::map<complex::Uuid, complex::Uuid> CorePlugin::getSimplToComplexMap() const
{
  return complex::k_SIMPL_to_Core;
}

COMPLEX_DEF_PLUGIN(CorePlugin)
