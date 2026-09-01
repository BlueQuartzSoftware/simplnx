#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
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

#include "SimplnxCore/Filters/Algorithms/Silhouette.hpp"
#include "SimplnxCore/Filters/SilhouetteFilter.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const DataPath k_QuadGeomPath = DataPath({Constants::k_DataContainer});
const DataPath k_CellPath = k_QuadGeomPath.createChildPath(Constants::k_CellData);

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

void buildParityInput(DataStructure& dataStructure)
{
  const ShapeType k_TupleShape = {1, 1, 6};
  auto* imageGeom = ImageGeom::Create(dataStructure, "Silhouette Parity ImageGeom");
  REQUIRE(imageGeom != nullptr);
  imageGeom->setDimensions({6, 1, 1});
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, "Cell Data", k_TupleShape, imageGeom->getId());
  REQUIRE(attributeMatrix != nullptr);
  imageGeom->setCellData(*attributeMatrix);

  auto inputStore = std::make_shared<DataStore<float64>>(k_TupleShape, ShapeType{2}, std::optional<float64>{});
  auto* input = Float64Array::Create(dataStructure, "Input", inputStore, attributeMatrix->getId());
  REQUIRE(input != nullptr);
  const std::array<float64, 12> inputValues = {0.0, 0.0, 0.0, 1.0, 4.0, 4.0, 4.0, 5.0, 8.0, 8.0, 8.0, 9.0};
  SIMPLNX_RESULT_REQUIRE_VALID(inputStore->copyFromBuffer(0, nonstd::span<const float64>(inputValues.data(), inputValues.size())));

  auto featureIdsStore = std::make_shared<DataStore<int32>>(k_TupleShape, ShapeType{1}, std::optional<int32>{});
  auto* featureIds = Int32Array::Create(dataStructure, "FeatureIds", featureIdsStore, attributeMatrix->getId());
  REQUIRE(featureIds != nullptr);
  const std::array<int32, 6> featureValues = {0, 1, 1, 7, 7, 7};
  SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStore->copyFromBuffer(0, nonstd::span<const int32>(featureValues.data(), featureValues.size())));
}

SilhouetteInputValues makeParityInputValues(const DataPath& maskPath, int32 metric)
{
  SilhouetteInputValues inputValues;
  inputValues.DistanceMetric = static_cast<ClusterUtilities::DistanceMetric>(metric);
  inputValues.UseMask = true;
  inputValues.ClusteringArrayPath = k_ParityInputPath;
  inputValues.MaskArrayPath = maskPath;
  inputValues.FeatureIdsArrayPath = k_ParityFeatureIdsPath;
  inputValues.SilhouetteArrayPath = k_ParityOutputPath;
  return inputValues;
}

template <typename MaskT>
void addParityArrays(DataStructure& dataStructure, const StringLiteral& maskName, const std::array<MaskT, 6>& maskValues)
{
  const ShapeType k_TupleShape = {1, 1, 6};
  auto& attributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(k_ParityMatrixPath);
  auto outputStore = std::make_shared<DataStore<float64>>(k_TupleShape, ShapeType{1}, std::optional<float64>{});
  REQUIRE(Float64Array::Create(dataStructure, "Silhouette", outputStore, attributeMatrix.getId()) != nullptr);
  outputStore->fill(42.0);
  if constexpr(std::is_same_v<MaskT, bool>)
  {
    auto* mask = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, maskName, k_TupleShape, ShapeType{1}, attributeMatrix.getId());
    REQUIRE(mask != nullptr);
    SIMPLNX_RESULT_REQUIRE_VALID(mask->getDataStoreRef().copyFromBuffer(0, nonstd::span<const bool>(maskValues.data(), maskValues.size())));
  }
  else
  {
    auto maskStore = std::make_shared<DataStore<uint8>>(k_TupleShape, ShapeType{1}, std::optional<uint8>{});
    auto* mask = UInt8Array::Create(dataStructure, maskName, maskStore, attributeMatrix.getId());
    REQUIRE(mask != nullptr);
    SIMPLNX_RESULT_REQUIRE_VALID(maskStore->copyFromBuffer(0, nonstd::span<const uint8>(maskValues.data(), maskValues.size())));
  }
  const std::array<int32, 6> featureIds = {0, 1, 1, 2, 2, 2};
  SIMPLNX_RESULT_REQUIRE_VALID(dataStructure.getDataRefAs<Int32Array>(k_ParityFeatureIdsPath).getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size())));
}
} // namespace

#if SIMPLNX_TEST_ALGORITHM_PATH == 0
TEST_CASE("SimplnxCore::Silhouette: Direct and Scanline Bool UInt8 metric parity", "[SimplnxCore][Silhouette]")
{
  const auto [maskName, maskValues] =
      GENERATE(std::make_tuple(StringLiteral{"BoolMask"}, std::array<uint8, 6>{1, 1, 0, 1, 1, 0}), std::make_tuple(StringLiteral{"UInt8Mask"}, std::array<uint8, 6>{1, 1, 0, 1, 1, 0}));
  for(const int32 metric : {0, 1, 2, 3, 4, 5})
  {
    DYNAMIC_SECTION(fmt::format("{} metric {}", maskName, metric))
    {
      DataStructure directDataStructure;
      buildParityInput(directDataStructure);
      if(maskName.view() == "BoolMask")
      {
        const std::array<bool, 6> boolValues = {true, true, false, true, true, false};
        addParityArrays(directDataStructure, maskName, boolValues);
      }
      else
      {
        addParityArrays(directDataStructure, maskName, maskValues);
      }
      const std::atomic_bool shouldCancel = false;
      auto directValues = makeParityInputValues(k_ParityMatrixPath.createChildPath(maskName), metric);
      {
        UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
        SIMPLNX_RESULT_REQUIRE_VALID(scope.execute([&] { return Silhouette(directDataStructure, {}, shouldCancel, &directValues)(); }));
      }

      DataStructure scanlineDataStructure;
      buildParityInput(scanlineDataStructure);
      if(maskName.view() == "BoolMask")
      {
        const std::array<bool, 6> boolValues = {true, true, false, true, true, false};
        addParityArrays(scanlineDataStructure, maskName, boolValues);
      }
      else
      {
        addParityArrays(scanlineDataStructure, maskName, maskValues);
      }
      auto scanlineValues = makeParityInputValues(k_ParityMatrixPath.createChildPath(maskName), metric);
      {
        UnitTest::AlgorithmTestScope scope(UnitTest::AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
        SIMPLNX_RESULT_REQUIRE_VALID(scope.execute([&] { return Silhouette(scanlineDataStructure, {}, shouldCancel, &scanlineValues)(); }));
      }
      const auto& directOutput = directDataStructure.getDataRefAs<Float64Array>(k_ParityOutputPath);
      const auto& scanlineOutput = scanlineDataStructure.getDataRefAs<Float64Array>(k_ParityOutputPath);
      for(usize i = 0; i < directOutput.getNumberOfTuples(); i++)
      {
        CHECK(std::abs(directOutput[i] - scanlineOutput[i]) < 1.0e-12);
      }
      CHECK(directOutput[2] == 0.0);
      CHECK(directOutput[5] == 0.0);
      CHECK(scanlineOutput[2] == 0.0);
      CHECK(scanlineOutput[5] == 0.0);
    }
  }
}
#endif

TEST_CASE("SimplnxCore::SilhouetteFilter: Medoids Test", "[SimplnxCore][SilhouetteFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    SilhouetteFilter filter;
    Arguments args;

    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MedoidsClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MedoidsSilhouettePathNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    CHECK(preflightResult.outputActions.value().actions.size() == 1);
    CHECK(preflightResult.outputActions.value().deferredActions.empty());
    CHECK(dataStructure.getDataAs<IDataArray>(DataPath({"temp_mask"})) == nullptr);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MedoidsSilhouettePath, k_MedoidsSilhouettePathNX);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: Means Test", "[SimplnxCore][SilhouetteFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "k_files_v2.tar.gz", "k_files_v2");
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/k_files_v2/7_0_silhouette_exemplar.dream3d", unit_test::k_TestFilesDir)));

  {
    // Configure the filter arguments.
    SilhouetteFilter filter;
    Arguments args;

    args.insertOrAssign(SilhouetteFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(SilhouetteFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_CellPath.createChildPath("DAMAGE")));
    args.insertOrAssign(SilhouetteFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_MeansClusterIdsPath));
    args.insertOrAssign(SilhouetteFilter::k_SilhouetteArrayPath_Key, std::make_any<DataPath>(k_MeansSilhouettePathNX));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CompareArrays<float64>(dataStructure, k_MeansSilhouettePath, k_MeansSilhouettePathNX);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SilhouetteFilter: SIMPL Backwards Compatibility", "[SimplnxCore][SilhouetteFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "SilhouetteFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "SilhouetteFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<SilhouetteFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(SilhouetteFilter::k_DistanceMetric_Key) == 0);
      CHECK(args.value<bool>(SilhouetteFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(SilhouetteFilter::k_SelectedArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(SilhouetteFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}
