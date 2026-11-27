#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>
#include <vector>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/Silhouette.hpp"
#include "SimplnxCore/Filters/SilhouetteFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_QuadGeomPath = DataPath({constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(constants::k_CellData);

const DataPath k_MedoidsClusterIdsPath = k_CellPath.createChildPath("MedoidsClusterIds");
const DataPath k_MeansClusterIdsPath = k_CellPath.createChildPath("MeansClusterIds");

const std::string k_MedoidsSilhouetteName = "MedoidsSilhouette";
const std::string k_MeansSilhouetteName = "MeansSilhouette";

const DataPath k_MedoidsSilhouettePath = k_CellPath.createChildPath(k_MedoidsSilhouetteName);
const DataPath k_MeansSilhouettePath = k_CellPath.createChildPath(k_MeansSilhouetteName);

const DataPath k_MedoidsSilhouettePathNX = k_CellPath.createChildPath(k_MedoidsSilhouetteName + "NX");
const DataPath k_MeansSilhouettePathNX = k_CellPath.createChildPath(k_MeansSilhouetteName + "NX");

const DataPath k_ParityGeomPath({"Silhouette Parity ImageGeom"});
const DataPath k_ParityMatrixPath = k_ParityGeomPath.createChildPath("Cell Data");
const DataPath k_ParityInputPath = k_ParityMatrixPath.createChildPath("Input");
const DataPath k_ParityFeatureIdsPath = k_ParityMatrixPath.createChildPath("FeatureIds");
const DataPath k_ParityOutputPath = k_ParityMatrixPath.createChildPath("Silhouette");

void BuildParityInput(DataStructure& dataStructure)
{
  const ShapeType kTupleShape = {1, 1, 6};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Silhouette Parity ImageGeom");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({6, 1, 1});
  auto* attributeMatrixPtr = AttributeMatrix::Create(dataStructure, "Cell Data", kTupleShape, imageGeomPtr->getId());
  REQUIRE(attributeMatrixPtr != nullptr);
  imageGeomPtr->setCellData(*attributeMatrixPtr);

  auto inputStore = std::make_shared<DataStore<float64>>(kTupleShape, ShapeType{2}, std::optional<float64>{});
  auto* inputPtr = Float64ArrayType::Create(dataStructure, "Input", inputStore, attributeMatrixPtr->getId());
  REQUIRE(inputPtr != nullptr);
  const std::array<float64, 12> inputValues = {0.0, 0.0, 0.0, 1.0, 4.0, 4.0, 4.0, 5.0, 8.0, 8.0, 8.0, 9.0};
  auto inputStoreWriteResult = inputStore->copyFromBuffer(0, nonstd::span<const float64>(inputValues.data(), inputValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(inputStoreWriteResult);

  auto featureIdsStore = std::make_shared<DataStore<int32>>(kTupleShape, ShapeType{1}, std::optional<int32>{});
  auto* featureIdsPtr = Int32ArrayType::Create(dataStructure, "FeatureIds", featureIdsStore, attributeMatrixPtr->getId());
  REQUIRE(featureIdsPtr != nullptr);
  const std::array<int32, 6> featureValues = {0, 1, 1, 7, 7, 7};
  auto featureIdsStoreWriteResult = featureIdsStore->copyFromBuffer(0, nonstd::span<const int32>(featureValues.data(), featureValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStoreWriteResult);
}

SilhouetteInputValues MakeParityInputValues(const DataPath& maskPath, int32 metric)
{
  SilhouetteInputValues inputValues;
  inputValues.distance_metric = static_cast<cluster_utilities::DistanceMetric>(metric);
  inputValues.use_mask = true;
  inputValues.clustering_array_path = k_ParityInputPath;
  inputValues.mask_array_path = maskPath;
  inputValues.feature_ids_array_path = k_ParityFeatureIdsPath;
  inputValues.silhouette_array_path = k_ParityOutputPath;
  return inputValues;
}

template <typename MaskT>
void AddParityArrays(DataStructure& dataStructure, const StringLiteralType& maskName, const std::array<MaskT, 6>& maskValues)
{
  const ShapeType kTupleShape = {1, 1, 6};
  auto& attributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(k_ParityMatrixPath);
  auto outputStore = std::make_shared<DataStore<float64>>(kTupleShape, ShapeType{1}, std::optional<float64>{});
  REQUIRE(Float64ArrayType::Create(dataStructure, "Silhouette", outputStore, attributeMatrix.getId()) != nullptr);
  outputStore->fill(42.0);
  if constexpr(std::is_same_v<MaskT, bool>)
  {
    auto* maskPtr = BoolArrayType::CreateWithStore<BoolDataStoreType>(dataStructure, maskName, kTupleShape, ShapeType{1}, attributeMatrix.getId());
    REQUIRE(maskPtr != nullptr);
    auto maskWriteResult = maskPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const bool>(maskValues.data(), maskValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(maskWriteResult);
  }
  else
  {
    auto maskStore = std::make_shared<DataStore<uint8>>(kTupleShape, ShapeType{1}, std::optional<uint8>{});
    auto* maskPtr = UInt8ArrayType::Create(dataStructure, maskName, maskStore, attributeMatrix.getId());
    REQUIRE(maskPtr != nullptr);
    auto maskStoreWriteResult = maskStore->copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(maskStoreWriteResult);
  }
  const std::array<int32, 6> featureIds = {0, 1, 1, 2, 2, 2};
  auto parityFeatureIdsWriteResult =
      dataStructure.getDataRefAs<Int32ArrayType>(k_ParityFeatureIdsPath).getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(parityFeatureIdsWriteResult);
}
} // namespace

TEST_CASE("SimplnxCore::Silhouette: Direct and Scanline Bool UInt8 metric parity", "[SimplnxCore][Silhouette]")
{
  const auto [maskName, maskValues] =
      GENERATE(std::make_tuple(StringLiteralType{"BoolMask"}, std::array<uint8, 6>{1, 1, 0, 1, 1, 0}), std::make_tuple(StringLiteralType{"UInt8Mask"}, std::array<uint8, 6>{1, 1, 0, 1, 1, 0}));
  for(const int32 metric : {0, 1, 2, 3, 4, 5})
  {
    DYNAMIC_SECTION(fmt::format("{} metric {}", maskName, metric))
    {
      const std::atomic_bool shouldCancel = false;
      std::vector<float64> expectedOutput;
      bool hasExpectedOutput = false;
      const auto scenarios = unit_test::SelectAlgorithmTestScenariosForInMemoryStores();
      for(const auto scenario : scenarios)
      {
        CAPTURE(scenario);
        DataStructure dataStructure;
        BuildParityInput(dataStructure);
        if(maskName.view() == "BoolMask")
        {
          const std::array<bool, 6> boolValues = {true, true, false, true, true, false};
          AddParityArrays(dataStructure, maskName, boolValues);
        }
        else
        {
          AddParityArrays(dataStructure, maskName, maskValues);
        }
        auto values = MakeParityInputValues(k_ParityMatrixPath.createChildPath(maskName), metric);
        unit_test::AlgorithmTestScope scope(scenario);
        auto executeResult2 = scope.execute([&] { return Silhouette(dataStructure, {}, shouldCancel, &values)(); });
        SIMPLNX_RESULT_REQUIRE_VALID(executeResult2);
        const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(k_ParityOutputPath);
        std::vector<float64> outputValues(output.getSize());
        auto getDataStoreRefResult = output.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(outputValues.data(), outputValues.size()));
        SIMPLNX_RESULT_REQUIRE_VALID(getDataStoreRefResult);
        CHECK(outputValues[2] == 0.0);
        CHECK(outputValues[5] == 0.0);
        // Eligible tuples are (0,0), (0,1), (4,5), (8,8), in clusters
        // 0, 1, 2, 2. The current contract includes self-distance in the
        // own-cluster mean and excludes cluster zero as a competing cluster.
        // These expressions use those four points, not the distance utility.
        std::array<float64, 6> analytical = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
        if(metric <= 2)
        {
          const float64 ownMean = metric == 0 ? 2.5 : (metric == 1 ? 12.5 : 3.5);
          const float64 distanceC = metric == 0 ? std::sqrt(32.0) : (metric == 1 ? 32.0 : 8.0);
          const float64 distanceD = metric == 0 ? std::sqrt(113.0) : (metric == 1 ? 113.0 : 15.0);
          analytical[3] = 1.0 - ownMean / distanceC;
          analytical[4] = 1.0 - ownMean / distanceD;
        }
        else if(metric == 3)
        {
          analytical[0] = 0.0; // Zero vectors have cosine distance one, including self-distance.
          const float64 ownMean = (1.0 - 9.0 / std::sqrt(82.0)) / 2.0;
          analytical[3] = 1.0 - ownMean / (1.0 - 5.0 / std::sqrt(41.0));
          analytical[4] = 1.0 - ownMean / (1.0 - 1.0 / std::sqrt(2.0));
        }
        else
        {
          // Constant vectors have correlation distance one; the two
          // increasing vectors have correlation distance zero.
          analytical = {0.0, 1.0, 0.0, -1.0, 0.0, 0.0};
        }
        REQUIRE(outputValues.size() == analytical.size());
        for(usize tupleIdx = 0; tupleIdx < analytical.size(); ++tupleIdx)
        {
          CAPTURE(tupleIdx, metric);
          CHECK(outputValues[tupleIdx] == Approx(analytical[tupleIdx]).margin(1.0E-12));
        }
        if(hasExpectedOutput)
        {
          CHECK(outputValues == expectedOutput);
        }
        else
        {
          expectedOutput = std::move(outputValues);
          hasExpectedOutput = true;
        }
      }
    }
  }
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Medoids Test", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = unit_test::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    SilhouetteFilter filter;
    Arguments args;

    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MedoidsClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MedoidsSilhouettePathNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult = preflightResult.output_actions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult);
    CHECK(preflightResult.output_actions.value().actions.size() == 1);
    CHECK(preflightResult.output_actions.value().deferred_actions.empty());
    CHECK(dataStructure.getDataAs<IDataArray>(DataPath({"temp_mask"})) == nullptr);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult2 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult2);
  }

  unit_test::CompareArrays<float64>(dataStructure, k_MedoidsSilhouettePath, k_MedoidsSilhouettePathNX);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Means Test", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = unit_test::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    SilhouetteFilter filter;
    Arguments args;

    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MeansClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MeansSilhouettePathNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    auto assertionResult3 = preflightResult.output_actions;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult3);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    auto assertionResult4 = executeResult.result;
    SIMPLNX_RESULT_REQUIRE_VALID(assertionResult4);
  }

  unit_test::CompareArrays<float64>(dataStructure, k_MeansSilhouettePath, k_MeansSilhouettePathNX);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SilhouetteFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SilhouetteFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SilhouetteFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<SilhouetteFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(SilhouetteFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(SilhouetteFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(SilhouetteFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

namespace
{
template <typename T>
void CreateRegressionArray(DataStructure& dataStructure, const std::string& name, const std::vector<T>& values, usize componentCount = 1)
{
  auto store = data_store_utilities::CreateDataStore<T>(dataStructure, DataPath({name}), {values.size() / componentCount}, {componentCount});
  REQUIRE(DataArray<T>::Create(dataStructure, name, store) != nullptr);
  auto writeResult = store->copyFromBuffer(0, nonstd::span<const T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
}

Arguments MakeRegressionArguments()
{
  auto args = SilhouetteFilter().getDefaultArguments();
  args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(DataPath({"Input"})));
  args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({"Cluster Ids"})));
  args.insertOrAssign(SilhouetteFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"Mask"})));
  args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(DataPath({"Silhouette"})));
  args.insertOrAssign(SilhouetteFilter::k_DistanceMetric_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
  return args;
}
} // namespace

TEST_CASE("SimplnxCore::SilhouetteFilter: Sparse Feature Ids", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  CreateRegressionArray<float64>(dataStructure, "Input", {0, 1, 10, 12});
  CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {3, 3, 7, 7});
  SilhouetteFilter filter;
  auto args = MakeRegressionArguments();
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(DataPath({"Silhouette"})));
  const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(DataPath({"Silhouette"}));
  const std::array<float64, 4> expected = {21.0 / 22.0, 19.0 / 20.0, 17.0 / 19.0, 21.0 / 23.0};
  for(usize tupleIdx = 0; tupleIdx < expected.size(); tupleIdx++)
  {
    CAPTURE(tupleIdx);
    REQUIRE(output[tupleIdx] == Approx(expected[tupleIdx]).margin(1.0E-12));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Negative Cluster Ids", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  CreateRegressionArray<float64>(dataStructure, "Input", {0, 1, 10, 12});
  CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {-1, 1, 1, 2});
  SilhouetteFilter filter;
  auto args = MakeRegressionArguments();
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors().front().code == -54081);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Cluster Zero Mean", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  DataStructure dataStructure;
  CreateRegressionArray<float64>(dataStructure, "Input", {0, 2, 10, 11, 20, 24});
  CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {0, 0, 1, 1, 2, 2});
  SilhouetteFilter filter;
  auto args = MakeRegressionArguments();
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64ArrayType>(DataPath({"Silhouette"})));
  const auto& output = dataStructure.getDataRefAs<Float64ArrayType>(DataPath({"Silhouette"}));
  const std::array<float64, 6> expected = {19.0 / 21.0, 15.0 / 17.0, 23.0 / 24.0, 21.0 / 22.0, 15.0 / 19.0, 23.0 / 27.0};
  for(usize tupleIdx = 0; tupleIdx < expected.size(); tupleIdx++)
  {
    CAPTURE(tupleIdx);
    REQUIRE(output[tupleIdx] == Approx(expected[tupleIdx]).margin(1.0E-12));
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Invalid Mask Tuple Count", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreateRegressionArray<float64>(dataStructure, "Input", {0, 2, 10, 11, 20, 24});
  CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {0, 0, 1, 1, 2, 2});
  CreateRegressionArray<uint8>(dataStructure, "Mask", {1, 1, 1, 1, 1});
  SilhouetteFilter filter;
  auto args = MakeRegressionArguments();
  args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().front().code == -8977);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Invalid Component Shapes", "[SimplnxCore][SilhouetteFilter]")
{
  unit_test::LoadPlugins();
  DataStructure dataStructure;
  CreateRegressionArray<float64>(dataStructure, "Input", {0, 2, 10, 11, 20, 24});
  std::string invalidArray;
  SECTION("Mask Array")
  {
    invalidArray = "Mask Array";
    CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {0, 0, 1, 1, 2, 2});
    CreateRegressionArray<uint8>(dataStructure, "Mask", std::vector<uint8>(12, 1), 2);
  }
  SECTION("Cluster Ids")
  {
    invalidArray = "Cluster Ids";
    CreateRegressionArray<int32>(dataStructure, "Cluster Ids", {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, 2);
    CreateRegressionArray<uint8>(dataStructure, "Mask", std::vector<uint8>(6, 1));
  }
  CAPTURE(invalidArray);
  SilhouetteFilter filter;
  auto args = MakeRegressionArguments();
  args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().front().code == -208);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
