#include "GenericPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("646640db-a416-56bb-9a4f-1777d3473ac9");
} // namespace

GenericPlugin::GenericPlugin()
: AbstractPlugin(k_ID, "Generic", "Description", "BlueQuartz Software")
{
  registerFilters();
}

GenericPlugin::~GenericPlugin() = default;

std::vector<complex::H5::IDataFactory*> GenericPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(GenericPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Generic/plugin_filter_registration.h"
