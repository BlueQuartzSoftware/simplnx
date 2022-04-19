#include "OSUToolboxPlugin.hpp"
#include "OSUToolbox/OSUToolbox_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{37fb4025-f40d-5ef7-bff4-8bec274316e3}").value(), Uuid::FromString("37fb4025-f40d-5ef7-bff4-8bec274316e3").value()}, /* OSUToolboxFilter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("6d1011be-24af-5887-8ba1-eaf8e0783ec3");
} // namespace

OSUToolboxPlugin::OSUToolboxPlugin()
: AbstractPlugin(k_ID, "OSUToolbox", "<<--Description was not read-->>", "Vendor Name")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

OSUToolboxPlugin::~OSUToolboxPlugin() = default;

std::vector<complex::H5::IDataFactory*> OSUToolboxPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(OSUToolboxPlugin)
