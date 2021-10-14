#include "ImportExportPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("ac2bf919-d16c-57e5-9901-b3ecd14dc6f8");
} // namespace

ImportExportPlugin::ImportExportPlugin()
: AbstractPlugin(k_ID, "ImportExport", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ImportExportPlugin::~ImportExportPlugin() = default;

std::vector<complex::H5::IDataFactory*> ImportExportPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ImportExportPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "ImportExport/plugin_filter_registration.h"
