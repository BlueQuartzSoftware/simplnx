#include "StatsToolboxPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("46b6b619-9852-55ab-8c68-2aca73f3f3f6");
} // namespace

StatsToolboxPlugin::StatsToolboxPlugin()
: AbstractPlugin(k_ID, "StatsToolbox", "Description", "BlueQuartz Software")
{
  registerFilters();
}

StatsToolboxPlugin::~StatsToolboxPlugin() = default;

std::vector<complex::H5::IDataFactory*> StatsToolboxPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(StatsToolboxPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "StatsToolbox/plugin_filter_registration.h"
