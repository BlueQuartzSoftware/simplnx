#include "VolumeMeshingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("eff5697e-e79d-5eea-9743-47de482c6cc5");
} // namespace

VolumeMeshingPlugin::VolumeMeshingPlugin()
: AbstractPlugin(k_ID, "VolumeMeshing", "Description", "BlueQuartz Software")
{
  registerFilters();
}

VolumeMeshingPlugin::~VolumeMeshingPlugin() = default;

std::vector<complex::H5::IDataFactory*> VolumeMeshingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(VolumeMeshingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "VolumeMeshing/plugin_filter_registration.h"
