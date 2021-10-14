#include "EMMPMPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("9bebf190-632e-52f6-83a2-094e60e22478");
} // namespace

EMMPMPlugin::EMMPMPlugin()
: AbstractPlugin(k_ID, "EMMPM", "Description", "BlueQuartz Software")
{
  registerFilters();
}

EMMPMPlugin::~EMMPMPlugin() = default;

std::vector<complex::H5::IDataFactory*> EMMPMPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(EMMPMPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "EMMPM/plugin_filter_registration.h"
