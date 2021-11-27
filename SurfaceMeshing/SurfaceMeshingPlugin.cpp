#include "SurfaceMeshingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{a69d8d43-5be9-59a0-b997-81773a635673}").value(), Uuid::FromString("69a75d68-16e7-53b1-9f6f-d49b6524bba0").value()}, /* FeatureFaceCurvatureFilter */
    {Uuid::FromString("{a5cff82b-9fe4-5a8c-90c9-6db74b6dcd50}").value(), Uuid::FromString("6776a912-764b-52fa-90f4-3a17bb866820").value()}, /* FindTriangleGeomCentroids */
    {Uuid::FromString("{749dc8ae-a402-5ee7-bbca-28d5734c60df}").value(), Uuid::FromString("c1a2cc33-4db8-5d9b-ad80-153062a17976").value()}, /* FindTriangleGeomNeighbors */
    {Uuid::FromString("{26765457-89fb-5686-87f6-878ca549f0df}").value(), Uuid::FromString("42bcb963-047c-58ab-a6a6-034c9c1f840d").value()}, /* FindTriangleGeomShapes */
    {Uuid::FromString("{9157ef1c-7cbc-5840-b6e7-26089c0b0f88}").value(), Uuid::FromString("ada04e57-bafd-5edb-8bd1-078f4a5a2da4").value()}, /* FindTriangleGeomSizes */
    {Uuid::FromString("{ea565056-784e-52c5-b705-92f6799714c7}").value(), Uuid::FromString("e4c2ad48-b6f8-5b59-9592-dfa9ca23140d").value()}, /* GenerateGeometryConnectivity */
    {Uuid::FromString("{07b49e30-3900-5c34-862a-f1fb48bad568}").value(), Uuid::FromString("1a36c497-2e93-580a-9fa6-b8a217a32492").value()}, /* QuickSurfaceMesh */
    {Uuid::FromString("{9b9fb9e1-074d-54b6-a6ce-0be21ab4496d}").value(), Uuid::FromString("9844a992-a520-5611-aa20-aab712caa337").value()}, /* ReverseTriangleWinding */
    {Uuid::FromString("{15c743db-5936-53a2-92cf-edf00526486d}").value(), Uuid::FromString("27e919af-b100-58fe-b844-3c1bfb3a4bdf").value()}, /* SharedFeatureFaceFilter */
    {Uuid::FromString("{7aa33007-4186-5d7f-ba9d-d0a561b3351d}").value(), Uuid::FromString("6eb7f1f4-c26a-5f93-88a6-ed0fbd689ede").value()}, /* TriangleCentroidFilter */
    {Uuid::FromString("{0541c5eb-1976-5797-9468-be50a93d44e2}").value(), Uuid::FromString("4658b821-26de-5930-80a9-d3bb2e94d4ea").value()}, /* TriangleDihedralAngleFilter */
    {Uuid::FromString("{928154f6-e4bc-5a10-a9dd-1abb6a6c0f6b}").value(), Uuid::FromString("a09e69b1-5f02-5a2b-b92a-e67268538558").value()}, /* TriangleNormalFilter */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("c257fabd-df24-55b2-80f1-041e207731ce");
} // namespace

SurfaceMeshingPlugin::SurfaceMeshingPlugin()
: AbstractPlugin(k_ID, "SurfaceMeshing", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

SurfaceMeshingPlugin::~SurfaceMeshingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SurfaceMeshingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SurfaceMeshingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "SurfaceMeshing/plugin_filter_registration.h"
