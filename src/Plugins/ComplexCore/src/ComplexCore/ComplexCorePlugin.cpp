#include "ComplexCorePlugin.hpp"
#include "ComplexCoreLegacyUUIDMapping.hpp"

#include "ComplexCore/ComplexCore_filter_registration.hpp"

using namespace complex;

namespace
{
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

AbstractPlugin::SIMPLMapType ComplexCorePlugin::getSimplToComplexMap() const
{
  return complex::k_SIMPL_to_ComplexCore;
}

COMPLEX_DEF_PLUGIN(ComplexCorePlugin)
