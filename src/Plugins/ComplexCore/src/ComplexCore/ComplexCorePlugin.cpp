#include "ComplexCorePlugin.hpp"
#include "ComplexCoreLegacyUUIDMapping.hpp"

#include "ComplexCore/ComplexCore_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{47cafe63-83cc-5826-9521-4fb5bea684ef}").value(), Uuid::FromString("bad9b7bd-1dc9-5f21-a889-6520e7a41881").value()}, /* ConditionalSetValue */
    {Uuid::FromString("{f2132744-3abb-5d66-9cd9-c9a233b5c4aa}").value(), Uuid::FromString("c4320659-1a84-561d-939e-c7c10229a504").value()}, /* CreateImageGeometry */
};

constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

ComplexCorePlugin::ComplexCorePlugin()
: AbstractPlugin(k_ID, "ComplexCore", "Description", "BlueQuartz Software")
{
  std::vector<complex::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

ComplexCorePlugin::~ComplexCorePlugin() = default;

std::map<complex::Uuid, complex::Uuid> ComplexCorePlugin::getSimplToComplexMap() const
{
  return complex::k_SIMPL_to_ComplexCore;
}

COMPLEX_DEF_PLUGIN(ComplexCorePlugin)
