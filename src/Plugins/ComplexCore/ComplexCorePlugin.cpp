#include "ComplexCorePlugin.hpp"

using namespace complex;

namespace
{
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
} // namespace

ComplexCorePlugin::ComplexCorePlugin()
: AbstractPlugin(k_ID, "ComplexCore", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ComplexCorePlugin::~ComplexCorePlugin() = default;

std::vector<complex::H5::IDataFactory*> ComplexCorePlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ComplexCorePlugin)

#include "ComplexCore/plugin_filter_registration.h"
