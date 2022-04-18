#include "SyntheticBuildingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{ac99b706-d1e0-5f78-9246-fbbe1efd93d2}").value(), Uuid::FromString("ac99b706-d1e0-5f78-9246-fbbe1efd93d2").value()}, /* AddBadData */
    {Uuid::FromString("{9f392fba-1520-5f8f-988b-0032d7c51811}").value(), Uuid::FromString("9f392fba-1520-5f8f-988b-0032d7c51811").value()}, /* AddOrientationNoise */
    {Uuid::FromString("{28910d1c-4309-500a-9508-e3ef1612e1f8}").value(), Uuid::FromString("28910d1c-4309-500a-9508-e3ef1612e1f8").value()}, /* EstablishMatrixPhase */
    {Uuid::FromString("{4edbbd35-a96b-5ff1-984a-153d733e2abb}").value(), Uuid::FromString("4edbbd35-a96b-5ff1-984a-153d733e2abb").value()}, /* EstablishShapeTypes */
    {Uuid::FromString("{16659766-5c53-5ada-a7b7-8a95c29ea674}").value(), Uuid::FromString("16659766-5c53-5ada-a7b7-8a95c29ea674").value()}, /* GeneratePrecipitateStatsData */
    {Uuid::FromString("{383f0e2a-c82e-5f7e-a904-156828b42314}").value(), Uuid::FromString("383f0e2a-c82e-5f7e-a904-156828b42314").value()}, /* GeneratePrimaryStatsData */
    {Uuid::FromString("{c2ae366b-251f-5dbd-9d70-d790376c0c0d}").value(), Uuid::FromString("c2ae366b-251f-5dbd-9d70-d790376c0c0d").value()}, /* InitializeSyntheticVolume */
    {Uuid::FromString("{4ee65edd-8d7f-5b0b-a7dd-c4b96e272a87}").value(), Uuid::FromString("4ee65edd-8d7f-5b0b-a7dd-c4b96e272a87").value()}, /* InsertAtoms */
    {Uuid::FromString("{1e552e0c-53bb-5ae1-bd1c-c7a6590f9328}").value(), Uuid::FromString("1e552e0c-53bb-5ae1-bd1c-c7a6590f9328").value()}, /* InsertPrecipitatePhases */
    {Uuid::FromString("{b7301dbf-27d5-5335-b86e-563d573f002b}").value(), Uuid::FromString("b7301dbf-27d5-5335-b86e-563d573f002b").value()}, /* JumbleOrientations */
    {Uuid::FromString("{7bfb6e4a-6075-56da-8006-b262d99dff30}").value(), Uuid::FromString("7bfb6e4a-6075-56da-8006-b262d99dff30").value()}, /* MatchCrystallography */
    {Uuid::FromString("{84305312-0d10-50ca-b89a-fda17a353cc9}").value(), Uuid::FromString("84305312-0d10-50ca-b89a-fda17a353cc9").value()}, /* PackPrimaryPhases */
    {Uuid::FromString("{f642e217-4722-5dd8-9df9-cee71e7b26ba}").value(), Uuid::FromString("f642e217-4722-5dd8-9df9-cee71e7b26ba").value()}, /* StatsGeneratorFilter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("290cb19c-d8ba-5c4c-827f-50a4a4fa003f");
} // namespace

SyntheticBuildingPlugin::SyntheticBuildingPlugin()
: AbstractPlugin(k_ID, "SyntheticBuilding", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerPublicFilters();
}

SyntheticBuildingPlugin::~SyntheticBuildingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SyntheticBuildingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SyntheticBuildingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "SyntheticBuilding/SyntheticBuilding_filter_registration.hpp"
