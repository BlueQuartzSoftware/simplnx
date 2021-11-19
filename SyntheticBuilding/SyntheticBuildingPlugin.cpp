#include "SyntheticBuildingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{ac99b706-d1e0-5f78-9246-fbbe1efd93d2}").value(), Uuid::FromString("c91b5df9-8e77-5308-b958-f0d636ac15c5").value()}, /* AddBadData */
    {Uuid::FromString("{9f392fba-1520-5f8f-988b-0032d7c51811}").value(), Uuid::FromString("20d2b746-72ac-52db-861e-219d62e64275").value()}, /* AddOrientationNoise */
    {Uuid::FromString("{28910d1c-4309-500a-9508-e3ef1612e1f8}").value(), Uuid::FromString("dece3e11-41ba-5371-b554-4582bac37ff1").value()}, /* EstablishMatrixPhase */
    {Uuid::FromString("{4edbbd35-a96b-5ff1-984a-153d733e2abb}").value(), Uuid::FromString("c9ef2d38-2cc2-52f4-a763-0969609a4d6f").value()}, /* EstablishShapeTypes */
    {Uuid::FromString("{16659766-5c53-5ada-a7b7-8a95c29ea674}").value(), Uuid::FromString("bf3a46e2-74fb-5e6f-a242-eb3da3ad5129").value()}, /* GeneratePrecipitateStatsData */
    {Uuid::FromString("{383f0e2a-c82e-5f7e-a904-156828b42314}").value(), Uuid::FromString("b28ca241-0fe6-5353-8a2a-4f8ce468b82a").value()}, /* GeneratePrimaryStatsData */
    {Uuid::FromString("{c2ae366b-251f-5dbd-9d70-d790376c0c0d}").value(), Uuid::FromString("08aa5ff6-9b7c-5e98-9a63-711e853c822e").value()}, /* InitializeSyntheticVolume */
    {Uuid::FromString("{4ee65edd-8d7f-5b0b-a7dd-c4b96e272a87}").value(), Uuid::FromString("08bcc8bc-d5d5-511d-aa46-28ff88512a15").value()}, /* InsertAtoms */
    {Uuid::FromString("{1e552e0c-53bb-5ae1-bd1c-c7a6590f9328}").value(), Uuid::FromString("cd77503d-ed3d-5391-aa52-81d1f484039e").value()}, /* InsertPrecipitatePhases */
    {Uuid::FromString("{b7301dbf-27d5-5335-b86e-563d573f002b}").value(), Uuid::FromString("5b0ad850-8e35-58d7-8d9b-1926852f8632").value()}, /* JumbleOrientations */
    {Uuid::FromString("{7bfb6e4a-6075-56da-8006-b262d99dff30}").value(), Uuid::FromString("ffeb3492-3af0-59ec-a426-1dc276abd427").value()}, /* MatchCrystallography */
    {Uuid::FromString("{84305312-0d10-50ca-b89a-fda17a353cc9}").value(), Uuid::FromString("7604a61e-f0c0-509b-a2b7-d9b5d30345ea").value()}, /* PackPrimaryPhases */
    {Uuid::FromString("{f642e217-4722-5dd8-9df9-cee71e7b26ba}").value(), Uuid::FromString("2452875a-335e-5d26-b391-e1909bf9e764").value()}, /* StatsGeneratorFilter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("290cb19c-d8ba-5c4c-827f-50a4a4fa003f");
} // namespace

SyntheticBuildingPlugin::SyntheticBuildingPlugin()
: AbstractPlugin(k_ID, "SyntheticBuilding", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
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
#include "SyntheticBuilding/plugin_filter_registration.h"
