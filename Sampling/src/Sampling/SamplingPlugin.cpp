#include "SamplingPlugin.hpp"

using namespace complex;

namespace
{
// This maps previous filters from DREAM.3D Version 6.x to DREAM.3D Version 7.x
std::map<complex::Uuid, complex::Uuid> k_SimplToComplexFilterMapping = {
    {Uuid::FromString("{52b2918a-4fb5-57aa-97d4-ccc084b89572}").value(), Uuid::FromString("b344db62-a44e-58cc-95f9-d07958f6d6a9").value()}, /* AppendImageGeometryZSlice */
    {Uuid::FromString("{baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf}").value(), Uuid::FromString("e73ba983-fca2-5557-9189-ffd6d08bfac8").value()}, /* CropImageGeometry */
    {Uuid::FromString("{e0555de5-bdc6-5bea-ba2f-aacfbec0a022}").value(), Uuid::FromString("a4025028-a12d-565e-88eb-272e6942de86").value()}, /* ExtractFlaggedFeatures */
    {Uuid::FromString("{cbaf9e68-5ded-560c-9440-509289100ea8}").value(), Uuid::FromString("baeeb05b-0d98-5590-998c-2b70bb774f88").value()}, /* NearestPointFuseRegularGrids */
    {Uuid::FromString("{0df3da89-9106-538e-b1a9-6bbf1cf0aa92}").value(), Uuid::FromString("c02ab065-d8ab-5d90-be58-3b5104a5efb5").value()}, /* RegularGridSampleSurfaceMesh */
    {Uuid::FromString("{bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce}").value(), Uuid::FromString("b5dae0ef-c748-5f87-82c4-513de3df476e").value()}, /* RegularizeZSpacing */
    {Uuid::FromString("{1966e540-759c-5798-ae26-0c6a3abc65c0}").value(), Uuid::FromString("4bc6846c-00ab-50b5-862b-aa6712260d41").value()}, /* ResampleImageGeom */
    {Uuid::FromString("{77befd69-4536-5856-9f81-02996d038f73}").value(), Uuid::FromString("75e973cb-3225-55f1-945a-622a21ea464c").value()}, /* ResampleRectGridToImageGeom */
    {Uuid::FromString("{0f44da6f-5272-5d69-8378-9bf0bc4ae4f9}").value(), Uuid::FromString("b940c282-f89d-5058-96ba-3767c843ccbb").value()}, /* SampleSurfaceMeshSpecifiedPoints */
    {Uuid::FromString("{75cfeb9b-cd4b-5a20-a344-4170b39bbfaf}").value(), Uuid::FromString("64c3aa41-4a2b-532d-baa4-89ffe05331e7").value()}, /* UncertainRegularGridSampleSurfaceMesh */
    {Uuid::FromString("{520fc4c4-9c22-5520-9e75-a64b81a5a38d}").value(), Uuid::FromString("d1c2f8a8-d046-5bcc-b968-01df587db79b").value()}, /* WarpRegularGrid */
};
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("82c85742-5a33-5bc3-9b02-f5629005d046");
} // namespace

SamplingPlugin::SamplingPlugin()
: AbstractPlugin(k_ID, "Sampling", "<<--Description was not read-->>", "BlueQuartz Software, LLC")
{
  registerFilters();
}

SamplingPlugin::~SamplingPlugin() = default;

std::vector<complex::H5::IDataFactory*> SamplingPlugin::getDataFactories() const
{
  return {};
}

COMPLEX_DEF_PLUGIN(SamplingPlugin)

// The below file is generated at CMake configure time. This is done because
// the cmake system knows what filters are being compiled. This saves the
// developer from having to upkeep these lists.
#include "Sampling/plugin_filter_registration.h"
