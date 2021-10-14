#include "SurfaceMeshingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("02560853-50e7-5788-bac8-c840bc4fff89");
} // namespace

SurfaceMeshingPlugin::SurfaceMeshingPlugin()
: AbstractPlugin(k_ID, "SurfaceMeshing", "Description", "BlueQuartz Software")
{
  registerFilters();
}

SurfaceMeshingPlugin::~SurfaceMeshingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SurfaceMeshingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SurfaceMeshingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "SurfaceMeshing/plugin_filter_registration.h"
