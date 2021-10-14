#include "ProcessingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("4d78a5c9-3c3a-5060-a0af-e992e0fbe921");
} // namespace

ProcessingPlugin::ProcessingPlugin()
: AbstractPlugin(k_ID, "Processing", "Description", "BlueQuartz Software")
{
  registerFilters();
}

ProcessingPlugin::~ProcessingPlugin() = default;

std::vector<complex::H5::IDataFactory*> ProcessingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ProcessingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Processing/plugin_filter_registration.h"
