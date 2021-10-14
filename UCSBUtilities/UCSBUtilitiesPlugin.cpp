#include "UCSBUtilitiesPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("91467a29-597c-5b71-afd6-88d1902c3b5e");
} // namespace

UCSBUtilitiesPlugin::UCSBUtilitiesPlugin()
: AbstractPlugin(k_ID, "UCSBUtilities", "Description", "BlueQuartz Software")
{
  registerFilters();
}

UCSBUtilitiesPlugin::~UCSBUtilitiesPlugin() = default;

std::vector<complex::H5::IDataFactory*> UCSBUtilitiesPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(UCSBUtilitiesPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "UCSBUtilities/plugin_filter_registration.h"
