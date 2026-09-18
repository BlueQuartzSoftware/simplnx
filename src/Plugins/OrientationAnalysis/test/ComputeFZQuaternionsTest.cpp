#include "OrientationAnalysis/Filters/ComputeFZQuaternionsFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;
using namespace nx::core::Constants;
namespace fs = std::filesystem;

namespace
{
DataStructure CreateDataStructure()
{
  DataStructure dataStructure;

  DataGroup* group = nx::core::DataGroup::Create(dataStructure, nx::core::Constants::k_SmallIN100);
  DataGroup* scanData = nx::core::DataGroup::Create(dataStructure, nx::core::Constants::k_EbsdScanData, group->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_SmallIn100ImageGeom, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  nx::core::SizeVec3 imageGeomDims = {100, 100, 2};
  imageGeom->setDimensions(imageGeomDims);

  // DataStructure owns created arrays. Do not create a second shared owner.
  std::vector<size_t> compDims = {4};
  std::vector<size_t> tupleDims = {100, 100, 2};

  std::string filePath = nx::core::unit_test::k_DataDir.str();

  std::string fileName = "/quats.raw";
  nx::core::ImportFromBinaryFile<float>(filePath + fileName, k_Quats, dataStructure, tupleDims, compDims, scanData->getId());

  fileName = "/fz_quats.raw";
  nx::core::ImportFromBinaryFile<float>(filePath + fileName, "FZ_QUATS_EXEMPLAR", dataStructure, tupleDims, compDims, scanData->getId());

  Int32Array* phases_data = nx::core::UnitTest::CreateTestDataArray<int32>(dataStructure, k_Phases, tupleDims, {1}, scanData->getId());
  phases_data->fill(1);

  DataGroup* phaseGroup = nx::core::DataGroup::Create(dataStructure, k_PhaseData, group->getId());
  UInt32Array* laueClass = UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, k_LaueClass, {2}, {1}, phaseGroup->getId());
  (*laueClass)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*laueClass)[1] = ebsdlib::CrystalStructure::Cubic_High;

  return dataStructure;
}

void MessageHandlerFunction(const IFilter::Message& message)
{
}

const DataPath k_AnalyticalQuatsPath({"AnalyticalQuaternions"});
const DataPath k_AnalyticalPhasesPath({"AnalyticalPhases"});
const DataPath k_AnalyticalCrystalStructuresPath({"AnalyticalCrystalStructures"});
const DataPath k_AnalyticalBoolMaskPath({"AnalyticalBoolMask"});
const DataPath k_AnalyticalUInt8MaskPath({"AnalyticalUInt8Mask"});
const std::string k_AnalyticalFzQuatsName = "AnalyticalFzQuaternions";
constexpr int k_NoMask = 0;
constexpr int k_BoolMask = 1;
constexpr int k_UInt8Mask = 2;

DataStructure CreateAnalyticalDataStructure(const std::array<int32, 4>& phases)
{
  DataStructure dataStructure;

  auto quatsStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_AnalyticalQuatsPath, {4}, {4});
  auto phasesStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_AnalyticalPhasesPath, {4}, {1});
  auto crystalStructuresStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, k_AnalyticalCrystalStructuresPath, {2}, {1});
  auto boolMaskStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_AnalyticalBoolMaskPath, {4}, {1});
  auto uint8MaskStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, k_AnalyticalUInt8MaskPath, {4}, {1});
  REQUIRE(quatsStore != nullptr);
  REQUIRE(phasesStore != nullptr);
  REQUIRE(crystalStructuresStore != nullptr);
  REQUIRE(boolMaskStore != nullptr);
  REQUIRE(uint8MaskStore != nullptr);

  auto* quats = Float32Array::Create(dataStructure, k_AnalyticalQuatsPath.getTargetName(), quatsStore);
  auto* phaseArray = Int32Array::Create(dataStructure, k_AnalyticalPhasesPath.getTargetName(), phasesStore);
  auto* crystalStructures = UInt32Array::Create(dataStructure, k_AnalyticalCrystalStructuresPath.getTargetName(), crystalStructuresStore);
  auto* boolMask = BoolArray::Create(dataStructure, k_AnalyticalBoolMaskPath.getTargetName(), boolMaskStore);
  auto* uint8Mask = UInt8Array::Create(dataStructure, k_AnalyticalUInt8MaskPath.getTargetName(), uint8MaskStore);
  REQUIRE(quats != nullptr);
  REQUIRE(phaseArray != nullptr);
  REQUIRE(crystalStructures != nullptr);
  REQUIRE(boolMask != nullptr);
  REQUIRE(uint8Mask != nullptr);

  for(usize tupleIdx = 0; tupleIdx < 4; tupleIdx++)
  {
    (*quats)[tupleIdx * 4] = 0.0F;
    (*quats)[tupleIdx * 4 + 1] = 0.0F;
    (*quats)[tupleIdx * 4 + 2] = 0.0F;
    (*quats)[tupleIdx * 4 + 3] = 1.0F;
    (*phaseArray)[tupleIdx] = phases[tupleIdx];
  }
  (*crystalStructures)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High;

  const std::array<bool, 4> mask = {true, false, false, false};
  for(usize tupleIdx = 0; tupleIdx < mask.size(); tupleIdx++)
  {
    (*boolMask)[tupleIdx] = mask[tupleIdx];
    (*uint8Mask)[tupleIdx] = mask[tupleIdx] ? 1U : 0U;
  }

  return dataStructure;
}

Arguments CreateAnalyticalArguments(bool useMask, const DataPath& maskPath)
{
  Arguments args = ComputeFZQuaternionsFilter().getDefaultArguments();
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(k_AnalyticalQuatsPath));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key, std::make_any<std::string>(k_AnalyticalFzQuatsName));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_AnalyticalPhasesPath));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_AnalyticalCrystalStructuresPath));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_UseMask_Key, std::make_any<bool>(useMask));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  return args;
}

} // namespace

TEST_CASE("OrientationAnalysis::ComputeFZQuaternions", "[OrientationAnalysis][ComputeFZQuaternions]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  ComputeFZQuaternionsFilter filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  DataPath scanDataPath = DataPath({nx::core::Constants::k_SmallIN100, nx::core::Constants::k_EbsdScanData});
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Quats)));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key, std::make_any<std::string>(k_FZQuats));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_PhaseData, k_LaueClass})));

  args.insertOrAssign(ComputeFZQuaternionsFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  auto generatedFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath(k_FZQuats));
  auto exemplarFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath("FZ_QUATS_EXEMPLAR"));
  UnitTest::CompareArrays<float32>(dataStructure, scanDataPath.createChildPath(k_FZQuats), scanDataPath.createChildPath("FZ_QUATS_EXEMPLAR"));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFZQuaternionsFilter: Negative Phase Bounds", "[OrientationAnalysis][ComputeFZQuaternions]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure = CreateDataStructure();
  const DataPath scanDataPath({k_SmallIN100, k_EbsdScanData});
  const DataPath phasesPath = scanDataPath.createChildPath(k_Phases);
  const DataPath maskPath = scanDataPath.createChildPath("Mask");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(phasesPath));
  auto& cellPhasesArrayRef = dataStructure.getDataRefAs<Int32Array>(phasesPath);
  cellPhasesArrayRef.getDataStoreRef()[0] = -1;

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataGroup>(scanDataPath));
  const auto& scanDataGroupRef = dataStructure.getDataRefAs<DataGroup>(scanDataPath);
  auto* maskArrayPtr = UnitTest::CreateTestDataArray<bool>(dataStructure, maskPath.getTargetName(), cellPhasesArrayRef.getTupleShape(), {1}, scanDataGroupRef.getId());
  REQUIRE(maskArrayPtr != nullptr);
  maskArrayPtr->fill(true);

  ComputeFZQuaternionsFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Quats)));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key, std::make_any<std::string>(k_FZQuats));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasesPath));
  args.insertOrAssign(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_PhaseData, k_LaueClass})));

  SECTION("Unmasked negative Phase returns an error")
  {
    args.insertOrAssign(ComputeFZQuaternionsFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -49009);
  }

  SECTION("Masked negative Phase is ignored")
  {
    maskArrayPtr->getDataStoreRef()[0] = false;
    args.insertOrAssign(ComputeFZQuaternionsFilter::k_UseMask_Key, std::make_any<bool>(true));
    args.insertOrAssign(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
}

TEST_CASE("OrientationAnalysis::ComputeFZQuaternionsFilter: analytical Bool, UInt8, and no-mask output", "[OrientationAnalysis][ComputeFZQuaternions]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto maskType = GENERATE(0, 1, 2);
  CAPTURE(scenario, maskType);
  UnitTest::AlgorithmTestScope scope(scenario);

  const std::array<int32, 4> phases = maskType == k_NoMask ? std::array<int32, 4>{1, 1, 1, 1} : std::array<int32, 4>{1, 1, -1, 1};
  DataStructure dataStructure = CreateAnalyticalDataStructure(phases);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalQuatsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalQuatsPath));

  const bool useMask = maskType != k_NoMask;
  const DataPath maskPath = maskType == k_BoolMask ? k_AnalyticalBoolMaskPath : (maskType == k_UInt8Mask ? k_AnalyticalUInt8MaskPath : DataPath{});
  const ComputeFZQuaternionsFilter filter;
  const Arguments args = CreateAnalyticalArguments(useMask, maskPath);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath outputPath = k_AnalyticalQuatsPath.replaceName(k_AnalyticalFzQuatsName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32Array>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Float32Array>(outputPath);
  for(usize tupleIdx = 0; tupleIdx < 4; tupleIdx++)
  {
    const bool included = maskType == k_NoMask || tupleIdx == 0;
    const std::array<float32, 4> expected = included ? std::array<float32, 4>{0.0F, 0.0F, 0.0F, 1.0F} : std::array<float32, 4>{0.0F, 0.0F, 0.0F, 0.0F};
    for(usize componentIdx = 0; componentIdx < expected.size(); componentIdx++)
    {
      INFO("tuple " << tupleIdx << ", component " << componentIdx);
      REQUIRE(output[tupleIdx * 4 + componentIdx] == expected[componentIdx]);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeFZQuaternionsFilter: enabled phase bounds return documented errors", "[OrientationAnalysis][ComputeFZQuaternions]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto expectedError = GENERATE(-49009, -49008);
  CAPTURE(scenario, expectedError);
  UnitTest::AlgorithmTestScope scope(scenario);

  const bool negativePhase = expectedError == -49009;
  DataStructure dataStructure = CreateAnalyticalDataStructure(negativePhase ? std::array<int32, 4>{-1, 1, 1, 1} : std::array<int32, 4>{2, 1, 1, 1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalQuatsPath));
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(k_AnalyticalQuatsPath));

  const DataPath maskPath = negativePhase ? k_AnalyticalBoolMaskPath : DataPath{};
  const Arguments args = CreateAnalyticalArguments(negativePhase, maskPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Int32Array>(k_AnalyticalPhasesPath));
  const auto& sourcePhases = dataStructure.getDataRefAs<Int32Array>(k_AnalyticalPhasesPath);
  REQUIRE(sourcePhases[0] == (negativePhase ? -1 : 2));

  const ComputeFZQuaternionsFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
  REQUIRE(executeResult.result.errors()[0].code == expectedError);
}

TEST_CASE("OrientationAnalysis::ComputeFZQuaternionsFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeFZQuaternionsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFZQuaternionsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFZQuaternionsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFZQuaternionsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<bool>(ComputeFZQuaternionsFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_QuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeFZQuaternionsFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ComputeFZQuaternionsFilter::k_FZQuatsArrayName_Key) == "TestArray");
    }
  }
}
