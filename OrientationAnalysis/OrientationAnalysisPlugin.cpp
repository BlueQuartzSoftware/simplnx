#include "OrientationAnalysisPlugin.hpp"

using namespace complex;

namespace
{
// List Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("790216ef-7042-5645-bccf-fd55d1569cbf");
} // namespace

OrientationAnalysisPlugin::OrientationAnalysisPlugin()
: AbstractPlugin(k_ID, "OrientationAnalysis", "Description", "BlueQuartz Software")
{
  registerFilters();
}

OrientationAnalysisPlugin::~OrientationAnalysisPlugin() = default;

std::vector<complex::H5::IDataFactory*> OrientationAnalysisPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(OrientationAnalysisPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "OrientationAnalysis/plugin_filter_registration.h"
