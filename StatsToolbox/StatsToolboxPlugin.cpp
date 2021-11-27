#include "StatsToolboxPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{289f0d8c-29ab-5fbc-91bd-08aac01e37c5}").value(), Uuid::FromString("dd9651b3-8933-5358-bb00-2825d6b3d88b").value()}, /* CalculateArrayHistogram */
    {Uuid::FromString("{27a132b2-a592-519a-8cb7-38599a7f28ec}").value(), Uuid::FromString("d6fd0aca-30d9-53a3-a661-0a38690a4a45").value()}, /* ComputeMomentInvariants2D */
    {Uuid::FromString("{cd91b8fd-0383-5803-ad26-9a47d6c309b0}").value(), Uuid::FromString("918ad532-c16b-5716-a0e2-fb33e888d670").value()}, /* FindAvgScalarValueForFeatures */
    {Uuid::FromString("{6357243e-41a6-52c4-be2d-2f6894c39fcc}").value(), Uuid::FromString("3424e842-ccc8-58f6-9dba-d38fc17cf2c1").value()}, /* FindBoundaryElementFractions */
    {Uuid::FromString("{29086169-20ce-52dc-b13e-824694d759aa}").value(), Uuid::FromString("4ef264f2-b97c-5d9a-b668-42f5416d22c4").value()}, /* FindDifferenceMap */
    {Uuid::FromString("{933e4b2d-dd61-51c3-98be-00548ba783a3}").value(), Uuid::FromString("98507d64-483d-5e47-81df-c0b4455e94ac").value()}, /* FindEuclideanDistMap */
    {Uuid::FromString("{a1e9cf6d-2d1b-573e-98b8-0314c993d2b6}").value(), Uuid::FromString("ee0a8e2c-2556-58b5-9ee9-c988d5068798").value()}, /* FindFeatureClustering */
    {Uuid::FromString("{9f77b4a9-6416-5220-a688-115f4e14c90d}").value(), Uuid::FromString("b881e549-8c73-5f08-a0dc-ad2b2b7d0e5d").value()}, /* FindLargestCrossSections */
    {Uuid::FromString("{697ed3de-db33-5dd1-a64b-04fb71e7d63e}").value(), Uuid::FromString("b8cd50e0-15c7-56dc-ac85-cf5b09a14207").value()}, /* FindNeighborhoods */
    {Uuid::FromString("{97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac}").value(), Uuid::FromString("0307a3a9-8633-5878-a208-f799308162f3").value()}, /* FindNeighbors */
    {Uuid::FromString("{529743cf-d5d5-5d5a-a79f-95c84a5ddbb5}").value(), Uuid::FromString("003fd97b-0645-54c3-841c-c6ed62cb6f36").value()}, /* FindNumFeatures */
    {Uuid::FromString("{3b0ababf-9c8d-538d-96af-e40775c4f0ab}").value(), Uuid::FromString("759c49f6-f5d4-5981-8967-dc9790c6d292").value()}, /* FindShapes */
    {Uuid::FromString("{5d586366-6b59-566e-8de1-57aa9ae8a91c}").value(), Uuid::FromString("16fff570-cb6f-59ac-ba75-3eab70607590").value()}, /* FindSurfaceAreaToVolume */
    {Uuid::FromString("{68246a67-7f32-5c80-815a-bec82008d7bc}").value(), Uuid::FromString("d0ef7827-cbf1-5a3f-938a-2d184604b49f").value()}, /* FindVolFractions */
    {Uuid::FromString("{6c255fc4-1692-57cf-be55-71dc4e05ec83}").value(), Uuid::FromString("ac39a101-3f9a-5604-b0f6-b0bda6989d40").value()}, /* FitFeatureData */
    {Uuid::FromString("{19a1cb76-6b46-528d-b629-1af5f1d6344c}").value(), Uuid::FromString("f11f8b23-b09d-5d78-abb0-8e9d8ab35bfb").value()}, /* GenerateEnsembleStatistics */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("b2e819c6-314c-56da-ab25-0684cd22cb2c");
} // namespace

StatsToolboxPlugin::StatsToolboxPlugin()
: AbstractPlugin(k_ID, "StatsToolbox", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

StatsToolboxPlugin::~StatsToolboxPlugin() = default;

std::vector<complex::H5::IDataFactory*> StatsToolboxPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(StatsToolboxPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "StatsToolbox/plugin_filter_registration.h"
