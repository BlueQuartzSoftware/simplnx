#include "SyntheticBuildingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("7d2e4cc0-c5b5-5f40-81c2-aa2c4f6141f0");
} // namespace

SyntheticBuildingPlugin::SyntheticBuildingPlugin()
: AbstractPlugin(k_ID, "SyntheticBuilding", "Description", "BlueQuartz Software")
{
  registerFilters();
}

SyntheticBuildingPlugin::~SyntheticBuildingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SyntheticBuildingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SyntheticBuildingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "SyntheticBuilding/plugin_filter_registration.h"
