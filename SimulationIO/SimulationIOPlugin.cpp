#include "SimulationIOPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("5457ba0f-c685-50b1-bae9-6a281cb2b25d");
} // namespace

SimulationIOPlugin::SimulationIOPlugin()
: AbstractPlugin(k_ID, "SimulationIO", "Description", "BlueQuartz Software")
{
  registerFilters();
}

SimulationIOPlugin::~SimulationIOPlugin() = default;

std::vector<complex::H5::IDataFactory*> SimulationIOPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SimulationIOPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "SimulationIO/plugin_filter_registration.h"
