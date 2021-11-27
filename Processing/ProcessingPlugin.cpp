#include "ProcessingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{14a39c35-8558-58c1-9d00-952396e6632b}").value(), Uuid::FromString("56f5483c-bbef-5fed-9cd3-a96df98b1971").value()}, /* DetectEllipsoids */
    {Uuid::FromString("{3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254}").value(), Uuid::FromString("7444ae5c-33a8-5076-886b-88f46b5d7f70").value()}, /* ErodeDilateBadData */
    {Uuid::FromString("{d26e85ff-7e52-53ae-b095-b1d969c9e73c}").value(), Uuid::FromString("084de759-cf5e-56fc-84e7-c472e7c9b441").value()}, /* ErodeDilateCoordinationNumber */
    {Uuid::FromString("{4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba}").value(), Uuid::FromString("880fd85d-2610-5e58-97c5-333ab52ff260").value()}, /* ErodeDilateMask */
    {Uuid::FromString("{30ae0a1e-3d94-5dab-b279-c5727ab5d7ff}").value(), Uuid::FromString("83f409ac-ed90-5640-a378-b99ac72d405a").value()}, /* FillBadData */
    {Uuid::FromString("{577dfdf6-02f8-5284-b45b-e31f5392a191}").value(), Uuid::FromString("d76b27b4-287e-5033-ae7a-740320f595eb").value()}, /* FindProjectedImageStatistics */
    {Uuid::FromString("{801008ce-1dcb-5604-8f16-a86526e28cf9}").value(), Uuid::FromString("b81d0944-9f3e-5a7f-8c2d-9cc8d3d41bb3").value()}, /* FindRelativeMotionBetweenSlices */
    {Uuid::FromString("{0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a}").value(), Uuid::FromString("3c118462-ae4c-52b4-a54f-d690241a427a").value()}, /* IdentifySample */
    {Uuid::FromString("{dab5de3c-5f81-5bb5-8490-73521e1183ea}").value(), Uuid::FromString("7e4e67b7-4b6c-5073-a7ec-98d0654c7315").value()}, /* MinNeighbors */
    {Uuid::FromString("{a8463056-3fa7-530b-847f-7f4cb78b8602}").value(), Uuid::FromString("5a3d45d6-5d91-5a28-9589-c6c2acecd69d").value()}, /* RemoveFlaggedFeatures */
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
