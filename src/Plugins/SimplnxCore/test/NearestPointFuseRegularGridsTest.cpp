#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "SimplnxCore/Filters/NearestPointFuseRegularGridsFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath sampleImageGeomPath({"Sample ImageGeom"});
const std::string sampleCellDataName = "Sample CellData";
const DataPath sampleCellDataPath = sampleImageGeomPath.createChildPath(sampleCellDataName);
const DataPath sampleDataArrayPath = sampleCellDataPath.createChildPath("sampleDataArray");
const DataPath sampleBoolArrayPath = sampleCellDataPath.createChildPath("sampleBoolArray");
const DataPath sampleMultiComponentArrayPath = sampleCellDataPath.createChildPath("sampleMultiComponentArray");
const DataPath sampleNeighborListPath = sampleCellDataPath.createChildPath("sampleNeighborList");

const DataPath refImageGeomPath({"Reference ImageGeom"});
const std::string refCellDataName = "Reference CellData";
const DataPath refCellDataPath = refImageGeomPath.createChildPath(refCellDataName);
const DataPath refDataArrayPath = refCellDataPath.createChildPath("refDataArray");
const DataPath copiedDataArrayPath = refCellDataPath.createChildPath("sampleDataArray");
const DataPath copiedBoolArrayPath = refCellDataPath.createChildPath("sampleBoolArray");
const DataPath copiedMultiComponentArrayPath = refCellDataPath.createChildPath("sampleMultiComponentArray");
const DataPath copiedNeighborListPath = refCellDataPath.createChildPath("sampleNeighborList");

DataStructure CreateDualImageGeomDataStructure(CreateImageGeometryAction::DimensionType refDims, CreateImageGeometryAction::OriginType refOrigin, CreateImageGeometryAction::SpacingType refSpacing)
{
  DataStructure dataStructure;

  const CreateImageGeometryAction::DimensionType dims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};
  const CreateImageGeometryAction::DimensionType attrMatrixDims = {dims.rbegin(), dims.rend()};

  auto sampleAction = CreateImageGeometryAction(sampleImageGeomPath, dims, origin, spacing, sampleCellDataName, IGeometry::LengthUnit::Micrometer);
  Result<> sampleActionResult = sampleAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(sampleActionResult);

  auto sampleDataAction = CreateArrayAction(DataType::float64, attrMatrixDims, {1}, sampleDataArrayPath);
  Result<> sampleDataActionResult = sampleDataAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(sampleDataActionResult);

  auto* sampleDataStore = dataStructure.getDataAs<Float64Array>(sampleDataArrayPath)->getDataStore();
  std::iota(sampleDataStore->begin(), sampleDataStore->end(), 1.0);

  auto* sampleBoolArray =
      UnitTest::CreateTestDataArray<bool>(dataStructure, sampleBoolArrayPath.getTargetName(), attrMatrixDims, {1}, dataStructure.getDataRefAs<AttributeMatrix>(sampleCellDataPath).getId());
  REQUIRE(sampleBoolArray != nullptr);
  for(usize index = 0; index < sampleBoolArray->getNumberOfTuples(); index++)
  {
    (*sampleBoolArray)[index] = index % 2 == 0;
  }

  auto* sampleMultiComponentArray =
      UnitTest::CreateTestDataArray<int32>(dataStructure, sampleMultiComponentArrayPath.getTargetName(), attrMatrixDims, {3}, dataStructure.getDataRefAs<AttributeMatrix>(sampleCellDataPath).getId());
  REQUIRE(sampleMultiComponentArray != nullptr);
  for(usize index = 0; index < sampleMultiComponentArray->getNumberOfTuples(); index++)
  {
    (*sampleMultiComponentArray)[index * 3] = static_cast<int32>(index);
    (*sampleMultiComponentArray)[index * 3 + 1] = static_cast<int32>(index + 100);
    (*sampleMultiComponentArray)[index * 3 + 2] = static_cast<int32>(index + 200);
  }

  auto* sampleNeighborList =
      NeighborList<int32>::Create(dataStructure, sampleNeighborListPath.getTargetName(), attrMatrixDims, dataStructure.getDataRefAs<AttributeMatrix>(sampleCellDataPath).getId());
  sampleNeighborList->setList(0, std::vector<int32>{1, 2, 3});

  auto refAction = CreateImageGeometryAction(refImageGeomPath, refDims, refOrigin, refSpacing, refCellDataName, IGeometry::LengthUnit::Micrometer);
  Result<> refActionResult = refAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(refActionResult);

  auto refDataAction = CreateArrayAction(DataType::int32, attrMatrixDims, {1}, refDataArrayPath);
  Result<> refDataActionResult = refDataAction.apply(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(refDataActionResult);

  auto* refDataStore = dataStructure.getDataAs<Int32Array>(refDataArrayPath)->getDataStore();
  std::iota(refDataStore->begin(), refDataStore->end(), 26);

  return dataStructure;
}
} // namespace

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: Basic Valid Execution", "[SimplnxCore][NearestPointFuseRegularGridsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {2.5f, 2.5f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Configure the filter arguments.
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing));
  Arguments args;

  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(dataStructure.getData(copiedNeighborListPath) == nullptr);

  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(sampleDataArrayPath));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(dataStructure.getData(copiedNeighborListPath) == nullptr);

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

  auto& copiedBoolArray = dataStructure.getDataRefAs<BoolArray>(copiedBoolArrayPath);
  auto& copiedMultiComponentArray = dataStructure.getDataRefAs<Int32Array>(copiedMultiComponentArrayPath);
  REQUIRE(copiedBoolArray[0] == true);
  REQUIRE(copiedBoolArray[3] == static_cast<bool>(9.8));
  REQUIRE(copiedMultiComponentArray[0] == 12);
  REQUIRE(copiedMultiComponentArray[1] == 112);
  REQUIRE(copiedMultiComponentArray[2] == 212);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: No Overlap Valid Execution", "[SimplnxCore][NearestPointFuseRegularGridsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {10.0f, 10.0f, 3.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Configure the filter arguments.
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing));
  Arguments args;

  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(sampleDataArrayPath));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto& copiedArray = dataStructure.getDataRefAs<Float64Array>(copiedDataArrayPath);
  for(const auto& value : copiedArray)
  {
    REQUIRE(value == 9.8);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: Nested Valid Execution", "[SimplnxCore][NearestPointFuseRegularGridsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {1.0f, 1.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {0.25f, 0.25f, 0.25f};

  // Configure the filter arguments.
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing));
  Arguments args;

  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(sampleDataArrayPath));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: Encompassing Valid Execution", "[SimplnxCore][NearestPointFuseRegularGridsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope algorithmTestScope(scenario);

  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {-2.0f, -2.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {2.0f, 2.0f, 2.0f};

  // Configure the filter arguments.
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing));
  Arguments args;

  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  algorithmTestScope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(sampleDataArrayPath));
  auto executeResult = algorithmTestScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

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

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: Invalid Execution", "[SimplnxCore][NearestPointFuseRegularGridsFilter]")
{
  UnitTest::LoadPlugins();

  const CreateImageGeometryAction::DimensionType refDims = {5, 5, 1};
  const CreateImageGeometryAction::OriginType refOrigin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType refSpacing = {1.0f, 1.0f, 1.0f};

  // Configure the filter arguments.
  NearestPointFuseRegularGridsFilter filter;
  DataStructure dataStructure(CreateDualImageGeomDataStructure(refDims, refOrigin, refSpacing));

  dataStructure.getDataAs<ImageGeom>(sampleImageGeomPath)->setSpacing(0, 0, 0);

  Arguments args;

  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_UseFill_Key, std::make_any<bool>(true));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_FillValue_Key, std::make_any<float64>(9.8));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key, std::make_any<DataPath>(sampleImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(sampleCellDataPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(refImageGeomPath));
  args.insertOrAssign(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(refCellDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(!executeResult.result.valid());

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][NearestPointFuseRegularGridsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "NearestPointFuseRegularGridsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "NearestPointFuseRegularGridsFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<NearestPointFuseRegularGridsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(NearestPointFuseRegularGridsFilter::k_ReferenceGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(NearestPointFuseRegularGridsFilter::k_ReferenceCellAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(NearestPointFuseRegularGridsFilter::k_SamplingGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(NearestPointFuseRegularGridsFilter::k_SamplingCellAttributeMatrixPath_Key) == DataPath({"DataContainer", "CellData"}));
    }
  }
}

TEST_CASE("SimplnxCore::NearestPointFuseRegularGridsFilter: Geometry cell tuple count", "[SimplnxCore][NearestPointFuseRegularGridsFilter][TupleCount]")
{
  using FilterType = NearestPointFuseRegularGridsFilter;
  DataStructure dataStructure;
  auto* samplingGeomPtr = ImageGeom::Create(dataStructure, "Sampling");
  auto* referenceGeomPtr = ImageGeom::Create(dataStructure, "Reference");
  REQUIRE(samplingGeomPtr != nullptr);
  REQUIRE(referenceGeomPtr != nullptr);
  samplingGeomPtr->setDimensions({2, 3, 2});
  referenceGeomPtr->setDimensions({2, 5, 2});
  samplingGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  referenceGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* samplingCellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, samplingGeomPtr->getId());
  auto* referenceCellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{20}, referenceGeomPtr->getId());
  REQUIRE(samplingCellAMPtr != nullptr);
  REQUIRE(referenceCellAMPtr != nullptr);
  samplingGeomPtr->setCellData(*samplingCellAMPtr);
  referenceGeomPtr->setCellData(*referenceCellAMPtr);

  const auto counts = GENERATE((std::pair<usize, usize>{11, 20}), (std::pair<usize, usize>{12, 19}), (std::pair<usize, usize>{12, 20}), (std::pair<usize, usize>{13, 20}),
                               (std::pair<usize, usize>{12, 21}), (std::pair<usize, usize>{0, 20}), (std::pair<usize, usize>{12, 0}));
  const auto [samplingTuples, referenceTuples] = counts;
  CAPTURE(samplingTuples, referenceTuples);
  // Both selected matrices are outside their geometry's Cell Data; the reference matrix starts empty.
  auto* sampleAMPtr = AttributeMatrix::Create(dataStructure, "Other Sampling Data", ShapeType{samplingTuples});
  REQUIRE(sampleAMPtr != nullptr);
  REQUIRE(AttributeMatrix::Create(dataStructure, "Other Reference Data", ShapeType{referenceTuples}) != nullptr);
  REQUIRE(Int32Array::Create(dataStructure, "Values", std::make_shared<Int32DataStore>(ShapeType{samplingTuples}, ShapeType{1}, 0), sampleAMPtr->getId()) != nullptr);
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Strings", ShapeType{samplingTuples}, std::vector<std::string>(samplingTuples, "value"), sampleAMPtr->getId()) != nullptr);

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_SamplingGeometryPath_Key, std::make_any<DataPath>(DataPath({"Sampling"})));
  args.insertOrAssign(FilterType::k_ReferenceGeometryPath_Key, std::make_any<DataPath>(DataPath({"Reference"})));
  args.insertOrAssign(FilterType::k_SamplingCellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Other Sampling Data"})));
  args.insertOrAssign(FilterType::k_ReferenceCellAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath({"Other Reference Data"})));
  const auto result = filter.preflight(dataStructure, args);
  if(samplingTuples == 12 && referenceTuples == 20)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -5565);
    const bool samplingMismatch = samplingTuples != 12;
    REQUIRE(errorRef.message.find(samplingMismatch ? "Other Sampling Data" : "Other Reference Data") != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", samplingMismatch ? samplingTuples : referenceTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find(samplingMismatch ? "'Sampling' has 12 cells" : "'Reference' has 20 cells") != std::string::npos);
  }
}
