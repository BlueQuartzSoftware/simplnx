#include <catch2/catch.hpp>

#include "complex/Parameters/DataGroupSelectionParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/NearestPointFuseRegularGridsFilter.hpp"

#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;

namespace
{
const DataPath sampleImageGeomPath({"Sample ImageGeom"});
const std::string sampleCellDataName = "Sample CellData";
const DataPath sampleCellDataPath = sampleImageGeomPath.createChildPath(sampleCellDataName);
const DataPath sampleDataArrayPath = sampleCellDataPath.createChildPath("sampleDataArray");

const DataPath refImageGeomPath({"Reference ImageGeom"});
const std::string refCellDataName = "Reference CellData";
const DataPath refCellDataPath = refImageGeomPath.createChildPath(refCellDataName);
const DataPath refDataArrayPath = refCellDataPath.createChildPath("refDataArray");
const DataPath copiedDataArrayPath = refCellDataPath.createChildPath("sampleDataArray");

DataStructure CreateDualImageGeomDataStructure(CreateImageGeometryAction::DimensionType refDims, CreateImageGeometryAction::OriginType refOrigin, CreateImageGeometryAction::SpacingType refSpacing)
{
  DataStructure dataStructure;

  const CreateImageGeometryAction::DimensionType dims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};

  auto sampleAction = CreateImageGeometryAction(sampleImageGeomPath, dims, origin, spacing, sampleCellDataName, IGeometry::LengthUnit::Micrometer);
  Result<> sampleActionResult = sampleAction.apply(dataStructure, IDataAction::Mode::Execute);
  COMPLEX_RESULT_REQUIRE_VALID(sampleActionResult);

  auto sampleDataAction = CreateArrayAction(DataType::float64, {25}, {1}, sampleDataArrayPath);
  Result<> sampleDataActionResult = sampleDataAction.apply(dataStructure, IDataAction::Mode::Execute);
  COMPLEX_RESULT_REQUIRE_VALID(sampleDataActionResult);

  auto* sampleDataStore = dataStructure.getDataAs<Float64Array>(sampleDataArrayPath)->getDataStore();
  std::iota(sampleDataStore->begin(), sampleDataStore->end(), 1.0);

  auto refAction = CreateImageGeometryAction(refImageGeomPath, refDims, refOrigin, refSpacing, refCellDataName, IGeometry::LengthUnit::Micrometer);
  Result<> refActionResult = refAction.apply(dataStructure, IDataAction::Mode::Execute);
  COMPLEX_RESULT_REQUIRE_VALID(refActionResult);

  auto refDataAction = CreateArrayAction(DataType::int32, {25}, {1}, refDataArrayPath);
  Result<> refDataActionResult = refDataAction.apply(dataStructure, IDataAction::Mode::Execute);
  COMPLEX_RESULT_REQUIRE_VALID(refDataActionResult);

  auto* refDataStore = dataStructure.getDataAs<Int32Array>(refDataArrayPath)->getDataStore();
  std::iota(refDataStore->begin(), refDataStore->end(), 26);

  return dataStructure;
}
} // namespace

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: Basic Valid Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {2.5f, 2.5f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(std::move(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& copiedArray = dataStructure.getDataRefAs<Float64Array>(copiedDataArrayPath);
  REQUIRE(copiedArray[0] == 13.0);
  REQUIRE(copiedArray[1] == 14.0);
  REQUIRE(copiedArray[2] == 15.0);
  REQUIRE(copiedArray[3] == 9.8);
  REQUIRE(copiedArray[4] == 9.8);
  REQUIRE(copiedArray[5] == 18.0);
  REQUIRE(copiedArray[6] == 19.0);
  REQUIRE(copiedArray[7] == 20.0);
  REQUIRE(copiedArray[8] == 9.8);
  REQUIRE(copiedArray[9] == 9.8);
  REQUIRE(copiedArray[10] == 23.0);
  REQUIRE(copiedArray[11] == 24.0);
  REQUIRE(copiedArray[12] == 25.0);
  REQUIRE(copiedArray[13] == 9.8);
  REQUIRE(copiedArray[14] == 9.8);
  REQUIRE(copiedArray[15] == 9.8);
  REQUIRE(copiedArray[16] == 9.8);
  REQUIRE(copiedArray[17] == 9.8);
  REQUIRE(copiedArray[18] == 9.8);
  REQUIRE(copiedArray[19] == 9.8);
  REQUIRE(copiedArray[20] == 9.8);
  REQUIRE(copiedArray[21] == 9.8);
  REQUIRE(copiedArray[22] == 9.8);
  REQUIRE(copiedArray[23] == 9.8);
  REQUIRE(copiedArray[24] == 9.8);
}

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: No Overlap Valid Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {10.0f, 10.0f, 3.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(std::move(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& copiedArray = dataStructure.getDataRefAs<Float64Array>(copiedDataArrayPath);
  for(const auto& value : copiedArray)
  {
    REQUIRE(value == 9.8);
  }
}

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: Nested Valid Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {1.0f, 1.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {0.25f, 0.25f, 0.25f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(std::move(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& copiedArray = dataStructure.getDataRefAs<Float64Array>(copiedDataArrayPath);
  REQUIRE(copiedArray[0] == 7.0);
  REQUIRE(copiedArray[1] == 7.0);
  REQUIRE(copiedArray[2] == 7.0);
  REQUIRE(copiedArray[3] == 7.0);
  REQUIRE(copiedArray[4] == 8.0);
  REQUIRE(copiedArray[5] == 7.0);
  REQUIRE(copiedArray[6] == 7.0);
  REQUIRE(copiedArray[7] == 7.0);
  REQUIRE(copiedArray[8] == 7.0);
  REQUIRE(copiedArray[9] == 8.0);
  REQUIRE(copiedArray[10] == 7.0);
  REQUIRE(copiedArray[11] == 7.0);
  REQUIRE(copiedArray[12] == 7.0);
  REQUIRE(copiedArray[13] == 7.0);
  REQUIRE(copiedArray[14] == 8.0);
  REQUIRE(copiedArray[15] == 7.0);
  REQUIRE(copiedArray[16] == 7.0);
  REQUIRE(copiedArray[17] == 7.0);
  REQUIRE(copiedArray[18] == 7.0);
  REQUIRE(copiedArray[19] == 8.0);
  REQUIRE(copiedArray[20] == 12.0);
  REQUIRE(copiedArray[21] == 12.0);
  REQUIRE(copiedArray[22] == 12.0);
  REQUIRE(copiedArray[23] == 12.0);
  REQUIRE(copiedArray[24] == 13.0);
}

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: Encompassing Valid Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {-2.0f, -2.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {2.0f, 2.0f, 2.0f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(std::move(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  auto& copiedArray = dataStructure.getDataRefAs<Float64Array>(copiedDataArrayPath);
  REQUIRE(copiedArray[0] == 9.8);
  REQUIRE(copiedArray[1] == 9.8);
  REQUIRE(copiedArray[2] == 9.8);
  REQUIRE(copiedArray[3] == 9.8);
  REQUIRE(copiedArray[4] == 9.8);
  REQUIRE(copiedArray[5] == 9.8);
  REQUIRE(copiedArray[6] == 1.0);
  REQUIRE(copiedArray[7] == 3.0);
  REQUIRE(copiedArray[8] == 5.0);
  REQUIRE(copiedArray[9] == 9.8);
  REQUIRE(copiedArray[10] == 9.8);
  REQUIRE(copiedArray[11] == 11.0);
  REQUIRE(copiedArray[12] == 13.0);
  REQUIRE(copiedArray[13] == 15.0);
  REQUIRE(copiedArray[14] == 9.8);
  REQUIRE(copiedArray[15] == 9.8);
  REQUIRE(copiedArray[16] == 21.0);
  REQUIRE(copiedArray[17] == 23.0);
  REQUIRE(copiedArray[18] == 25.0);
  REQUIRE(copiedArray[19] == 9.8);
  REQUIRE(copiedArray[20] == 9.8);
  REQUIRE(copiedArray[21] == 9.8);
  REQUIRE(copiedArray[22] == 9.8);
  REQUIRE(copiedArray[23] == 9.8);
  REQUIRE(copiedArray[24] == 9.8);
}

TEST_CASE("ComplexCore::NearestPointFuseRegularGridsFilter: Invalid Execution", "[ComplexCore][NearestPointFuseRegularGridsFilter]")
{
  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(std::move(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing)));

  dataStructure.getDataAs<ImageGeom>(sampleImageGeomPath)->setSpacing(0, 0, 0);

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());
}
