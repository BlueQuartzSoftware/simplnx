#include "EMMPMPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{4cd8f98b-75d0-5293-bf8e-d0f9f6211f58}").value(), Uuid::FromString("4cd8f98b-75d0-5293-bf8e-d0f9f6211f58").value()}, /* EMMPMFilter */
    {Uuid::FromString("{b2847755-dd39-5989-b624-98b1fdc9db5b}").value(), Uuid::FromString("b2847755-dd39-5989-b624-98b1fdc9db5b").value()}, /* MultiEmmpmFilter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("cd34edc4-d9d0-555c-855b-e846bb81085c");
} // namespace

EMMPMPlugin::EMMPMPlugin()
: AbstractPlugin(k_ID, "EMMPM", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerPublicFilters();
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
#include "EMMPM/EMMPM_filter_registration.hpp"
