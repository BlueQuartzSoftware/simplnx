#include "SimulationIOPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{d702beff-eb02-5ee1-a76a-79d5b56ec730}").value(), Uuid::FromString("d702beff-eb02-5ee1-a76a-79d5b56ec730").value()}, /* CreateAbaqusFile */
    {Uuid::FromString("{542c2417-1685-54e5-9322-f60792e9176c}").value(), Uuid::FromString("542c2417-1685-54e5-9322-f60792e9176c").value()}, /* CreateBsamFile */
    {Uuid::FromString("{fcff3b03-bff6-5511-bc65-5e558d12f0a6}").value(), Uuid::FromString("fcff3b03-bff6-5511-bc65-5e558d12f0a6").value()}, /* Export3dSolidMesh */
    {Uuid::FromString("{7c58e612-d7d6-5ec7-806b-cce0c1c211a3}").value(), Uuid::FromString("7c58e612-d7d6-5ec7-806b-cce0c1c211a3").value()}, /* ExportDAMASKFiles */
    {Uuid::FromString("{33c10889-4cdc-5992-ae00-1795e9bee022}").value(), Uuid::FromString("33c10889-4cdc-5992-ae00-1795e9bee022").value()}, /* ExportLAMMPSFile */
    {Uuid::FromString("{d5873836-f150-5fc9-9bc8-0bc837bd8dd4}").value(), Uuid::FromString("d5873836-f150-5fc9-9bc8-0bc837bd8dd4").value()}, /* ExportMultiOnScaleTableFile */
    {Uuid::FromString("{8efc447d-1c92-5ec5-885c-60b4a597835c}").value(), Uuid::FromString("8efc447d-1c92-5ec5-885c-60b4a597835c").value()}, /* ExportOnScaleTableFile */
    {Uuid::FromString("{f3def19a-b932-5ce7-9796-9e9476a29e1a}").value(), Uuid::FromString("f3def19a-b932-5ce7-9796-9e9476a29e1a").value()}, /* ImportDelamData */
    {Uuid::FromString("{248fdae8-a623-511b-8d09-5368c793c04d}").value(), Uuid::FromString("248fdae8-a623-511b-8d09-5368c793c04d").value()}, /* ImportFEAData */
    {Uuid::FromString("{06dd6e66-84fb-5170-a923-d925dc39bb94}").value(), Uuid::FromString("06dd6e66-84fb-5170-a923-d925dc39bb94").value()}, /* ImportOnScaleTableFile */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("40bd9d86-8fae-5451-a369-75fbd57def23");
} // namespace

SimulationIOPlugin::SimulationIOPlugin()
: AbstractPlugin(k_ID, "SimulationIO", "<<--Description was not read-->>", "BlueQuartz Software")
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
