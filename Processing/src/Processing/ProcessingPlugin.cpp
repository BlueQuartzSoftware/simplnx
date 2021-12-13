#include "ProcessingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{14a39c35-8558-58c1-9d00-952396e6632b}").value(), Uuid::FromString("14a39c35-8558-58c1-9d00-952396e6632b").value()}, /* DetectEllipsoids */
    {Uuid::FromString("{3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254}").value(), Uuid::FromString("3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254").value()}, /* ErodeDilateBadData */
    {Uuid::FromString("{d26e85ff-7e52-53ae-b095-b1d969c9e73c}").value(), Uuid::FromString("d26e85ff-7e52-53ae-b095-b1d969c9e73c").value()}, /* ErodeDilateCoordinationNumber */
    {Uuid::FromString("{4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba}").value(), Uuid::FromString("4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba").value()}, /* ErodeDilateMask */
    {Uuid::FromString("{30ae0a1e-3d94-5dab-b279-c5727ab5d7ff}").value(), Uuid::FromString("30ae0a1e-3d94-5dab-b279-c5727ab5d7ff").value()}, /* FillBadData */
    {Uuid::FromString("{577dfdf6-02f8-5284-b45b-e31f5392a191}").value(), Uuid::FromString("577dfdf6-02f8-5284-b45b-e31f5392a191").value()}, /* FindProjectedImageStatistics */
    {Uuid::FromString("{801008ce-1dcb-5604-8f16-a86526e28cf9}").value(), Uuid::FromString("801008ce-1dcb-5604-8f16-a86526e28cf9").value()}, /* FindRelativeMotionBetweenSlices */
    {Uuid::FromString("{0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a}").value(), Uuid::FromString("0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a").value()}, /* IdentifySample */
    {Uuid::FromString("{dab5de3c-5f81-5bb5-8490-73521e1183ea}").value(), Uuid::FromString("dab5de3c-5f81-5bb5-8490-73521e1183ea").value()}, /* MinNeighbors */
    {Uuid::FromString("{53ac1638-8934-57b8-b8e5-4b91cdda23ec}").value(), Uuid::FromString("53ac1638-8934-57b8-b8e5-4b91cdda23ec").value()}, /* MinSize */
    {Uuid::FromString("{a8463056-3fa7-530b-847f-7f4cb78b8602}").value(), Uuid::FromString("a8463056-3fa7-530b-847f-7f4cb78b8602").value()}, /* RemoveFlaggedFeatures */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("660e085c-7718-5572-a440-cc946423c228");
} // namespace

ProcessingPlugin::ProcessingPlugin()
: AbstractPlugin(k_ID, "Processing", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
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
