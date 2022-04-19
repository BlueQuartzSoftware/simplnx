#include "SamplingPlugin.hpp"
#include "Sampling/Sampling_filter_registration.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{52b2918a-4fb5-57aa-97d4-ccc084b89572}").value(), Uuid::FromString("52b2918a-4fb5-57aa-97d4-ccc084b89572").value()}, /* AppendImageGeometryZSlice */
    {Uuid::FromString("{baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf}").value(), Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf").value()}, /* CropImageGeometry */
    {Uuid::FromString("{e0555de5-bdc6-5bea-ba2f-aacfbec0a022}").value(), Uuid::FromString("e0555de5-bdc6-5bea-ba2f-aacfbec0a022").value()}, /* ExtractFlaggedFeatures */
    {Uuid::FromString("{cbaf9e68-5ded-560c-9440-509289100ea8}").value(), Uuid::FromString("cbaf9e68-5ded-560c-9440-509289100ea8").value()}, /* NearestPointFuseRegularGrids */
    {Uuid::FromString("{0df3da89-9106-538e-b1a9-6bbf1cf0aa92}").value(), Uuid::FromString("0df3da89-9106-538e-b1a9-6bbf1cf0aa92").value()}, /* RegularGridSampleSurfaceMesh */
    {Uuid::FromString("{bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce}").value(), Uuid::FromString("bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce").value()}, /* RegularizeZSpacing */
    {Uuid::FromString("{1966e540-759c-5798-ae26-0c6a3abc65c0}").value(), Uuid::FromString("1966e540-759c-5798-ae26-0c6a3abc65c0").value()}, /* ResampleImageGeom */
    {Uuid::FromString("{77befd69-4536-5856-9f81-02996d038f73}").value(), Uuid::FromString("77befd69-4536-5856-9f81-02996d038f73").value()}, /* ResampleRectGridToImageGeom */
    {Uuid::FromString("{0f44da6f-5272-5d69-8378-9bf0bc4ae4f9}").value(), Uuid::FromString("0f44da6f-5272-5d69-8378-9bf0bc4ae4f9").value()}, /* SampleSurfaceMeshSpecifiedPoints */
    {Uuid::FromString("{75cfeb9b-cd4b-5a20-a344-4170b39bbfaf}").value(), Uuid::FromString("75cfeb9b-cd4b-5a20-a344-4170b39bbfaf").value()}, /* UncertainRegularGridSampleSurfaceMesh */
    {Uuid::FromString("{520fc4c4-9c22-5520-9e75-a64b81a5a38d}").value(), Uuid::FromString("520fc4c4-9c22-5520-9e75-a64b81a5a38d").value()}, /* WarpRegularGrid */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("82c85742-5a33-5bc3-9b02-f5629005d046");
} // namespace

SamplingPlugin::SamplingPlugin()
: AbstractPlugin(k_ID, "Sampling", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
}

SamplingPlugin::~SamplingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SamplingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SamplingPlugin)
