#include "SamplingPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("4d95c46f-33c2-5a54-8978-e8a725306674");
} // namespace

SamplingPlugin::SamplingPlugin()
: AbstractPlugin(k_ID, "Sampling", "Description", "BlueQuartz Software")
{
  registerFilters();
}

SamplingPlugin::~SamplingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SamplingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SamplingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Sampling/plugin_filter_registration.h"
