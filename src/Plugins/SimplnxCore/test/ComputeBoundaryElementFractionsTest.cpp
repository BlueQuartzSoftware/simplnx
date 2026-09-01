#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/ComputeBoundaryElementFractionsFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_BCFName = "Boundary Cell Fractions";

const DataPath k_FeatureDataAMPath = DataPath({Constants::k_SmallIN100, Constants::k_Grain_Data});

const DataPath k_ExemplarBCFPath = k_FeatureDataAMPath.createChildPath(" Surface Element Fractions");
const DataPath k_GeneratedBCFPath = k_FeatureDataAMPath.createChildPath(k_BCFName);

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkFeatureCount = 4;
constexpr usize k_BenchmarkBoundaryPatternPeriod = 5;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr StringLiteral k_BenchmarkGeomName = "Benchmark ImageGeom";
constexpr StringLiteral k_BenchmarkCellDataName = "Cell Data";
constexpr StringLiteral k_BenchmarkFeatureDataName = "Feature Data";
constexpr StringLiteral k_BenchmarkFeatureIdsName = "FeatureIds";
constexpr StringLiteral k_BenchmarkBoundaryCellsName = "BoundaryCells";
constexpr StringLiteral k_BenchmarkFractionsName = "Boundary Cell Fractions";
const DataPath k_BenchmarkGeomPath({k_BenchmarkGeomName});
const DataPath k_BenchmarkCellDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkCellDataName);
const DataPath k_BenchmarkFeatureDataPath = k_BenchmarkGeomPath.createChildPath(k_BenchmarkFeatureDataName);
const DataPath k_BenchmarkFeatureIdsPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkFeatureIdsName);
const DataPath k_BenchmarkBoundaryCellsPath = k_BenchmarkCellDataPath.createChildPath(k_BenchmarkBoundaryCellsName);
const DataPath k_BenchmarkFractionsPath = k_BenchmarkFeatureDataPath.createChildPath(k_BenchmarkFractionsName);

void BuildBenchmarkInput(DataStructure& dataStructure)
{
  const ShapeType cellTupleShape = {k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim};

  auto* imageGeom = ImageGeom::Create(dataStructure, k_BenchmarkGeomName);
  imageGeom->setDimensions({k_BenchmarkDim, k_BenchmarkDim, k_BenchmarkDim});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});

  auto* cellData = AttributeMatrix::Create(dataStructure, k_BenchmarkCellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellData);

  auto featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_BenchmarkFeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIds = Int32Array::Create(dataStructure, k_BenchmarkFeatureIdsName, featureIdsStore, cellData->getId());
  REQUIRE(featureIds != nullptr);

  auto boundaryCellsStore = DataStoreUtilities::CreateDataStore<int8>(dataStructure, k_BenchmarkBoundaryCellsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* boundaryCells = Int8Array::Create(dataStructure, k_BenchmarkBoundaryCellsName, boundaryCellsStore, cellData->getId());
  REQUIRE(boundaryCells != nullptr);

  auto featureIdsBuffer = std::make_unique<int32[]>(k_BenchmarkSliceTuples);
  auto boundaryCellsBuffer = std::make_unique<int8[]>(k_BenchmarkSliceTuples);
  for(usize y = 0; y < k_BenchmarkDim; y++)
  {
    for(usize x = 0; x < k_BenchmarkDim; x++)
    {
      const usize index = (y * k_BenchmarkDim) + x;
      const usize featureId = (x % k_BenchmarkFeatureCount) + 1;
      const usize featureOccurrence = x / k_BenchmarkFeatureCount;
      featureIdsBuffer[index] = static_cast<int32>(featureId);
      boundaryCellsBuffer[index] = static_cast<int8>((featureOccurrence % k_BenchmarkBoundaryPatternPeriod) < featureId);
    }
  }

  for(usize z = 0; z < k_BenchmarkDim; z++)
  {
    const usize offset = z * k_BenchmarkSliceTuples;
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStore->copyFromBuffer(offset, nonstd::span<const int32>(featureIdsBuffer.get(), k_BenchmarkSliceTuples)));
    SIMPLNX_RESULT_REQUIRE_VALID(boundaryCellsStore->copyFromBuffer(offset, nonstd::span<const int8>(boundaryCellsBuffer.get(), k_BenchmarkSliceTuples)));
  }

  auto* featureData = AttributeMatrix::Create(dataStructure, k_BenchmarkFeatureDataName, {k_BenchmarkFeatureCount + 1}, imageGeom->getId());
  REQUIRE(featureData != nullptr);
}
} // namespace

TEST_CASE("SimplnxCore::ComputeBoundaryElementFractionsFilter: Valid Filter Execution", "[SimplnxCore][ComputeBoundaryElementFractionsFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_find_feature_boundary_element_fractions.tar.gz", "6_6_find_feature_boundary_element_fractions");

  DataStructure dataStructure =
      UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_boundary_element_fractions/6_6_find_feature_boundary_element_fractions.dream3d", unit_test::k_TestFilesDir)));

  {
    // Create the filter arguments for the calculation.
    ComputeBoundaryElementFractionsFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key,
                        std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BoundaryCells"})));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key, std::make_any<DataPath>(k_FeatureDataAMPath));
    args.insertOrAssign(ComputeBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key, std::make_any<std::string>(::k_BCFName));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float32>(dataStructure, k_ExemplarBCFPath, k_GeneratedBCFPath);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ComputeBoundaryElementFractionsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeBoundaryElementFractionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeBoundaryElementFractionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeBoundaryElementFractionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeBoundaryElementFractionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_FeatureDataAMPath_Key) == DataPath({"DataContainer", "CellData"}));
      CHECK(args.value<DataPath>(ComputeBoundaryElementFractionsFilter::k_BoundaryCellsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeBoundaryElementFractionsFilter::k_BoundaryCellFractionsArrayName_Key) == "TestArray");
    }
  }
}
