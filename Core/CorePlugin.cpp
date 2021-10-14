#include "CorePlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("a562a6a7-94d9-5803-bea3-eabb0bb2e5b2");
} // namespace

CorePlugin::CorePlugin()
: AbstractPlugin(k_ID, "Core", "Description", "BlueQuartz Software")
{
  registerFilters();
}

CorePlugin::~CorePlugin() = default;

std::vector<complex::H5::IDataFactory*> CorePlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(CorePlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Core/plugin_filter_registration.h"
