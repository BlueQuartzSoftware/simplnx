#include "ReconstructionPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("640adfc0-800a-528e-9472-8b201349a205");
} // namespace

ReconstructionPlugin::ReconstructionPlugin()
: AbstractPlugin(k_ID, "Reconstruction", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ReconstructionPlugin::~ReconstructionPlugin() = default;

std::vector<complex::H5::IDataFactory*> ReconstructionPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ReconstructionPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Reconstruction/plugin_filter_registration.h"
