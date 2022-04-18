#include "ReconstructionPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{2bb76fa9-934a-51df-bff1-b0c866971706}").value(), Uuid::FromString("2bb76fa9-934a-51df-bff1-b0c866971706").value()}, /* AlignSectionsFeature */
    {Uuid::FromString("{886f8b46-51b6-5682-a289-6febd10b7ef0}").value(), Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0").value()}, /* AlignSectionsFeatureCentroid */
    {Uuid::FromString("{accf8f6c-0551-5da3-9a3d-e4be41c3985c}").value(), Uuid::FromString("accf8f6c-0551-5da3-9a3d-e4be41c3985c").value()}, /* AlignSectionsList */
    {Uuid::FromString("{4fb2b9de-3124-534b-b914-dbbbdbc14604}").value(), Uuid::FromString("4fb2b9de-3124-534b-b914-dbbbdbc14604").value()}, /* AlignSectionsMisorientation */
    {Uuid::FromString("{61c5519b-5561-58b8-a522-2ce1324e244d}").value(), Uuid::FromString("61c5519b-5561-58b8-a522-2ce1324e244d").value()}, /* AlignSectionsMutualInformation */
    {Uuid::FromString("{bff6be19-1219-5876-8838-1574ad29d965}").value(), Uuid::FromString("bff6be19-1219-5876-8838-1574ad29d965").value()}, /* CAxisSegmentFeatures */
    {Uuid::FromString("{6eda8dbf-dbd8-562a-ae1a-f2904157c189}").value(), Uuid::FromString("6eda8dbf-dbd8-562a-ae1a-f2904157c189").value()}, /* ComputeFeatureRect */
    {Uuid::FromString("{7861c691-b821-537b-bd25-dc195578e0ea}").value(), Uuid::FromString("7861c691-b821-537b-bd25-dc195578e0ea").value()}, /* EBSDSegmentFeatures */
    {Uuid::FromString("{2c4a6d83-6a1b-56d8-9f65-9453b28845b9}").value(), Uuid::FromString("2c4a6d83-6a1b-56d8-9f65-9453b28845b9").value()}, /* MergeColonies */
    {Uuid::FromString("{c9af506e-9ea1-5ff5-a882-fa561def5f52}").value(), Uuid::FromString("c9af506e-9ea1-5ff5-a882-fa561def5f52").value()}, /* MergeTwins */
    {Uuid::FromString("{2c5edebf-95d8-511f-b787-90ee2adf485c}").value(), Uuid::FromString("2c5edebf-95d8-511f-b787-90ee2adf485c").value()}, /* ScalarSegmentFeatures */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("a506e94f-2a20-5eb4-b191-574cd89cf2f0");
} // namespace

ReconstructionPlugin::ReconstructionPlugin()
: AbstractPlugin(k_ID, "Reconstruction", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerPublicFilters();
}

ReconstructionPlugin::~ReconstructionPlugin() = default;

std::vector<complex::H5::IDataFactory*> ReconstructionPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(ReconstructionPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Reconstruction/Reconstruction_filter_registration.hpp"
