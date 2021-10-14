#include "ProgWorkshopPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("c1ad52bd-0c68-5aeb-8d1a-527d5f2f9a59");
} // namespace

ProgWorkshopPlugin::ProgWorkshopPlugin()
: AbstractPlugin(k_ID, "ProgWorkshop", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ProgWorkshopPlugin::~ProgWorkshopPlugin() = default;

std::vector<complex::H5::IDataFactory*> ProgWorkshopPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ProgWorkshopPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ProgWorkshop/plugin_filter_registration.h"
