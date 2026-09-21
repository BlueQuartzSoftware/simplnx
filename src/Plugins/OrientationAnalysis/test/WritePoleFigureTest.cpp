#include <catch2/catch.hpp>

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <numeric>
namespace fs = std::filesystem;

#include "OrientationAnalysis/Filters/WritePoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
using namespace nx::core::UnitTest;

// EbsdLib tests pin renderer pixels and projection details.
// These tests cover the simplnx value-add: parameter mapping, mask filtering,
// output-array creation, and SIMPL JSON conversion.

// The mask fixture contains 502 orientations with 251 selected values.
// Masked and unmasked outputs must differ by more than the configured threshold.
// Identical outputs would prove that mask filtering did not reach the renderer.
TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: Mask filter changes the rendered pole figure", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  // Keep the extracted fixture for the later HexConvention test.
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Pole_Figure_Exemplars_v6.tar.gz", "Pole_Figure_Exemplars_v6");

  auto baseDataFilePath = fs::path(fmt::format("{}/Pole_Figure_Exemplars_v6/Pole_Figure_Exemplars_v6.dream3d", unit_test::k_TestFilesDir));

  const DataPath k_Eulers({"Imported Data", "Eulers"});
  const DataPath k_Phases({"Imported Data", "Phases"});
  const DataPath k_Mask({"Imported Data", "Mask"});
  const DataPath k_CrystalStructures({"EnsembleAttributeMatrix", "CrystalStructures"});
  const DataPath k_MaterialNames({"EnsembleAttributeMatrix", "PhaseNames"});

  auto runWithMask = [&](bool useMask, const std::string& outGeomName) {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
    WritePoleFigureFilter filter;
    Arguments args;
    args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Mask Test"));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
    args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0)); // Color
    args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));         // Horizontal
    args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/MaskTestDir", unit_test::k_BinaryTestOutputDir))));
    args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>("mask_test_"));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(256));
    args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(useMask));
    args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_Mask));
    args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_Eulers));
    args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_Phases));
    args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructures));
    args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(k_MaterialNames));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({outGeomName})));
    args.insertOrAssign(WritePoleFigureFilter::k_HexConvention_Key, std::make_any<ChoicesParameter::ValueType>(1)); // X||a*
    args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(false));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Snapshot the rendered RGB array so we can compare across two filter runs
    // that live in different DataStructures (lambda-local).
    const DataPath imgPath = DataPath({outGeomName, "Cell Data", "Phase_1"});
    const auto& store = dataStructure.getDataRefAs<UInt8Array>(imgPath).getDataStoreRef();
    std::vector<uint8> snapshot(store.getSize());
    for(usize i = 0; i < store.getSize(); ++i)
    {
      snapshot[i] = store[i];
    }
    return snapshot;
  };

  const auto unmasked = runWithMask(false, "Unmasked PF");
  const auto masked = runWithMask(true, "Masked PF");

  REQUIRE(unmasked.size() == masked.size());
  REQUIRE(unmasked.size() > 0);

  usize diffBytes = 0;
  for(usize i = 0; i < unmasked.size(); ++i)
  {
    if(unmasked[i] != masked[i])
    {
      ++diffBytes;
    }
  }
  INFO(fmt::format("Bytes that differ between mask-off and mask-on: {} / {} ({:.2f}%)", diffBytes, unmasked.size(), 100.0 * static_cast<double>(diffBytes) / static_cast<double>(unmasked.size())));

  // The fixture masks roughly half of its orientations.
  // A one-percent byte difference is above the zero-difference failure case.
  REQUIRE(diffBytes > unmasked.size() / 100);
}

// HexConvention must reach both EbsdLib configuration objects.
// For hexagonal input, basal-plane families rotate 30 degrees between bases.
// Intensity and composite outputs must therefore differ.
TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: HexConvention choice reaches algorithm", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  // Keep the extracted fixture for both HexConvention runs.
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Pole_Figure_Exemplars_v6.tar.gz", "Pole_Figure_Exemplars_v6");

  auto baseDataFilePath = fs::path(fmt::format("{}/Pole_Figure_Exemplars_v6/Pole_Figure_Exemplars_v6.dream3d", unit_test::k_TestFilesDir));

  const DataPath k_Eulers({"Imported Data", "Eulers"});
  const DataPath k_Phases({"Imported Data", "Phases"});
  const DataPath k_CrystalStructures({"EnsembleAttributeMatrix", "CrystalStructures"});
  const DataPath k_MaterialNames({"EnsembleAttributeMatrix", "PhaseNames"});

  // Capture both intensity and composite RGB outputs.
  // They use separate configuration objects and must both honor HexConvention.
  struct ConvSnapshot
  {
    std::vector<float64> intensity;
    std::vector<uint8> compositeRgb;
  };

  auto runWithConv = [&](ChoicesParameter::ValueType convIndex, const std::string& geomName, const std::string& intensityName) {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
    WritePoleFigureFilter filter;
    Arguments args;
    args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Conv Test"));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
    args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0)); // Color
    args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));         // Horizontal
    args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/HexConvDir", unit_test::k_BinaryTestOutputDir))));
    args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>("conv_test_"));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(256));
    args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({geomName})));
    args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
    args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({intensityName})));
    args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(true));
    args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_Eulers));
    args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_Phases));
    args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructures));
    args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(k_MaterialNames));
    args.insertOrAssign(WritePoleFigureFilter::k_HexConvention_Key, std::make_any<ChoicesParameter::ValueType>(convIndex));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    ConvSnapshot snap;

    // Snapshot the second-family intensity array.
    // Basal-plane families rotate between X||a and X||a*, while the c-axis does not.
    const DataPath intensityPath = DataPath({intensityName, "Cell Data", "Phase_1_<011>"});
    const auto& intensityStore = dataStructure.getDataRefAs<Float64Array>(intensityPath).getDataStoreRef();
    snap.intensity.resize(intensityStore.getSize());
    for(usize i = 0; i < intensityStore.getSize(); ++i)
    {
      snap.intensity[i] = intensityStore[i];
    }

    // Snapshot the composite RGB array that becomes the PNG and downstream geometry.
    // This second snapshot verifies the composite configuration path separately.
    const DataPath compositePath = DataPath({geomName, "Cell Data", "Phase_1"});
    const auto& compositeStore = dataStructure.getDataRefAs<UInt8Array>(compositePath).getDataStoreRef();
    snap.compositeRgb.resize(compositeStore.getSize());
    for(usize i = 0; i < compositeStore.getSize(); ++i)
    {
      snap.compositeRgb[i] = compositeStore[i];
    }

    return snap;
  };

  const auto xa = runWithConv(0, "ConvTest_XA", "ConvIntensity_XA");
  const auto xastar = runWithConv(1, "ConvTest_XAStar", "ConvIntensity_XAStar");

  // ---- Assertion 1 -- intensity array honors hexConvention ----
  REQUIRE(xa.intensity.size() == xastar.intensity.size());
  REQUIRE(xa.intensity.size() > 0);

  usize diffPixels = 0;
  for(usize i = 0; i < xa.intensity.size(); ++i)
  {
    if(std::abs(xa.intensity[i] - xastar.intensity[i]) > 1.0e-9)
    {
      ++diffPixels;
    }
  }
  INFO(fmt::format("Intensity pixels that differ between X||a and X||a* (<10-10> family): {} / {}", diffPixels, xa.intensity.size()));

  // Hexagonal basal-plane intensity must differ between conventions.
  // Require more than one percent differing pixels.
  REQUIRE(diffPixels > xa.intensity.size() / 100);

  // The composite RGB output uses the same convention.
  // Its basal-plane figures rotate, so require more than one percent differing bytes.
  REQUIRE(xa.compositeRgb.size() == xastar.compositeRgb.size());
  REQUIRE(xa.compositeRgb.size() > 0);

  usize diffBytes = 0;
  for(usize i = 0; i < xa.compositeRgb.size(); ++i)
  {
    if(xa.compositeRgb[i] != xastar.compositeRgb[i])
    {
      ++diffBytes;
    }
  }
  INFO(fmt::format("Composite RGB bytes that differ between X||a and X||a*: {} / {} ({:.2f}%)", diffBytes, xa.compositeRgb.size(),
                   100.0 * static_cast<double>(diffBytes) / static_cast<double>(xa.compositeRgb.size())));

  REQUIRE(diffBytes > xa.compositeRgb.size() / 100);
}

// Discrete mode must reach the renderer and differ from color mode.
// Marker radii of 1 and 10 pixels must also produce different composites.
TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: Discrete mode and marker radius reach algorithm", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Pole_Figure_Exemplars_v6.tar.gz", "Pole_Figure_Exemplars_v6");

  auto baseDataFilePath = fs::path(fmt::format("{}/Pole_Figure_Exemplars_v6/Pole_Figure_Exemplars_v6.dream3d", unit_test::k_TestFilesDir));

  const DataPath k_Eulers({"Imported Data", "Eulers"});
  const DataPath k_Phases({"Imported Data", "Phases"});
  const DataPath k_CrystalStructures({"EnsembleAttributeMatrix", "CrystalStructures"});
  const DataPath k_MaterialNames({"EnsembleAttributeMatrix", "PhaseNames"});

  auto runPoleFigure = [&](ChoicesParameter::ValueType generationAlgorithm, int32 markerRadius, const std::string& geomName) {
    DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
    WritePoleFigureFilter filter;
    Arguments args;
    args.insertOrAssign(WritePoleFigureFilter::k_Title_Key, std::make_any<StringParameter::ValueType>("Discrete Test"));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(64));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(32));
    args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(generationAlgorithm));
    args.insertOrAssign(WritePoleFigureFilter::k_DiscreteMarkerRadius_Key, std::make_any<int32>(markerRadius));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0)); // Horizontal
    args.insertOrAssign(WritePoleFigureFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(fmt::format("{}/DiscreteTestDir", unit_test::k_BinaryTestOutputDir))));
    args.insertOrAssign(WritePoleFigureFilter::k_ImagePrefix_Key, std::make_any<StringParameter::ValueType>("discrete_test_"));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(256));
    args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
    args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({geomName})));
    args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(false));
    args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(k_Eulers));
    args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_Phases));
    args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(k_CrystalStructures));
    args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(k_MaterialNames));
    args.insertOrAssign(WritePoleFigureFilter::k_HexConvention_Key, std::make_any<ChoicesParameter::ValueType>(0)); // X||a

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    const DataPath compositePath = DataPath({geomName, "Cell Data", "Phase_1"});
    const auto& compositeStore = dataStructure.getDataRefAs<UInt8Array>(compositePath).getDataStoreRef();
    std::vector<uint8> snapshot(compositeStore.getSize());
    for(usize i = 0; i < compositeStore.getSize(); ++i)
    {
      snapshot[i] = compositeStore[i];
    }
    return snapshot;
  };

  auto countDiffBytes = [](const std::vector<uint8>& a, const std::vector<uint8>& b) {
    REQUIRE(a.size() == b.size());
    REQUIRE(a.size() > 0);
    usize diffBytes = 0;
    for(usize i = 0; i < a.size(); ++i)
    {
      if(a[i] != b[i])
      {
        ++diffBytes;
      }
    }
    return diffBytes;
  };

  const auto discreteSmall = runPoleFigure(1, 1, "Discrete_R1");
  const auto discreteLarge = runPoleFigure(1, 10, "Discrete_R10");
  const auto color = runPoleFigure(0, 3, "Color_PF");

  // Marker radius must reach the renderer: 1 px vs 10 px markers change the image.
  const usize radiusDiff = countDiffBytes(discreteSmall, discreteLarge);
  INFO(fmt::format("Bytes that differ between 1 px and 10 px markers: {} / {}", radiusDiff, discreteSmall.size()));
  REQUIRE(radiusDiff > discreteSmall.size() / 100);

  // Generation algorithm must reach the renderer: discrete and color composites differ.
  const usize modeDiff = countDiffBytes(discreteSmall, color);
  INFO(fmt::format("Bytes that differ between Discrete and Color modes: {} / {}", modeDiff, discreteSmall.size()));
  REQUIRE(modeDiff > discreteSmall.size() / 100);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: real HDF5 65536-block discrete-count oracle", "[OrientationAnalysis][WritePoleFigureFilter][.OocStoreContract]")
{
  UnitTest::LoadPlugins();
  REQUIRE(Application::Instance()->getIOManager("HDF5-OOC") != nullptr);
  const PreferencesSentinel preferencesSentinel(DataStorageMode::ForceOutOfCore, 1);
  constexpr usize k_BlockTuples = 65536;
  DataStructure dataStructure;
  const DataPath eulerPath({"Eulers"});
  const DataPath phasePath({"Phases"});
  const DataPath maskPath({"Mask"});
  const DataPath crystalPath({"CrystalStructures"});
  const DataPath namesPath({"Materials"});
  auto eulerStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, eulerPath, {k_BlockTuples + 1}, {3});
  auto phaseStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, phasePath, {k_BlockTuples + 1}, {1});
  auto maskStore = DataStoreUtilities::CreateDataStore<uint8>(dataStructure, maskPath, {k_BlockTuples + 1}, {1});
  auto crystalStore = DataStoreUtilities::CreateDataStore<uint32>(dataStructure, crystalPath, {2}, {1});
  auto* eulersPtr = Float32Array::Create(dataStructure, "Eulers", eulerStore);
  auto* phasesPtr = Int32Array::Create(dataStructure, "Phases", phaseStore);
  auto* maskPtr = UInt8Array::Create(dataStructure, "Mask", maskStore);
  auto* crystalsPtr = UInt32Array::Create(dataStructure, "CrystalStructures", crystalStore);
  REQUIRE(eulersPtr != nullptr);
  REQUIRE(phasesPtr != nullptr);
  REQUIRE(maskPtr != nullptr);
  REQUIRE(crystalsPtr != nullptr);
  REQUIRE(StringArray::CreateWithValues(dataStructure, "Materials", {2}, std::vector<std::string>{"Invalid", "Cubic"}) != nullptr);
  eulersPtr->fill(0);
  phasesPtr->fill(0);
  maskPtr->fill(0);
  (*phasesPtr)[1] = 1; // A masked-out orientation must not contribute a pole.
  (*phasesPtr)[k_BlockTuples - 1] = 1;
  (*phasesPtr)[k_BlockTuples] = 1;
  (*maskPtr)[k_BlockTuples - 1] = 1;
  (*maskPtr)[k_BlockTuples] = 1;
  (*eulersPtr)[k_BlockTuples * 3] = numbers::pi_v<float32> / 4.0F;
  (*crystalsPtr)[0] = 999;
  (*crystalsPtr)[1] = 1;
  REQUIRE(eulerStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(phaseStore->getDataFormat() == "HDF5-OOC");
  REQUIRE(maskStore->getDataFormat() == "HDF5-OOC");
  WritePoleFigureFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(32));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_NormalizeToMRD, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Counts"})));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityPlot1Name, std::make_any<std::string>("001"));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityPlot2Name, std::make_any<std::string>("011"));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityPlot3Name, std::make_any<std::string>("111"));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerPath));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(phasePath));
  args.insertOrAssign(WritePoleFigureFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalPath));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(namesPath));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const std::array<std::string, 3> families = {"001", "011", "111"};
  const std::array<float64, 3> expectedSums = {12.0, 24.0, 16.0};
  for(usize familyIdx = 0; familyIdx < families.size(); familyIdx++)
  {
    const DataPath outputPath({"Counts", "Cell Data", "Phase_1_" + families[familyIdx]});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(outputPath));
    const auto& output = dataStructure.getDataRefAs<Float64Array>(outputPath);
    REQUIRE(output.getDataStoreRef().getDataFormat() == "HDF5-OOC");
    std::vector<float64> values(32 * 32);
    auto readResult = output.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    REQUIRE(std::accumulate(values.begin(), values.end(), 0.0) == expectedSums[familyIdx]);
    if(familyIdx == 0)
    {
      // Two cubic <001> families give four poles at the center. Identity gives
      // four cardinal equatorial poles; the 45-degree tail gives four diagonals.
      // The stored image reverses row order, so the center is at (16,15).
      std::vector<float64> expected(32 * 32, 0.0);
      expected[15 * 32 + 16] = 4.0;
      for(usize index : {15 * 32 + 1, 15 * 32 + 31, 16, 30 * 32 + 16, 5 * 32 + 6, 5 * 32 + 26, 25 * 32 + 6, 25 * 32 + 26})
      {
        expected[index] = 1.0;
      }
      REQUIRE(values == expected);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// SIMPL JSON backwards compatibility.
// -----------------------------------------------------------------------------
TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][WritePoleFigureFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WritePoleFigureFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WritePoleFigureFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WritePoleFigureFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      // The legacy ImageFormat choice is intentionally dropped during conversion (always PNG; deviation D5).
      CHECK(args.value<std::string>(WritePoleFigureFilter::k_Title_Key) == "TestName");
      CHECK(args.value<ChoicesParameter::ValueType>(WritePoleFigureFilter::k_GenerationAlgorithm_Key) == 0);
      CHECK(args.value<int32>(WritePoleFigureFilter::k_LambertSize_Key) == 5);
      CHECK(args.value<int32>(WritePoleFigureFilter::k_NumColors_Key) == 5);
      CHECK(args.value<ChoicesParameter::ValueType>(WritePoleFigureFilter::k_ImageLayout_Key) == 0);
      CHECK(args.value<FileSystemPathParameter::ValueType>(WritePoleFigureFilter::k_OutputPath_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<std::string>(WritePoleFigureFilter::k_ImagePrefix_Key) == "TestName");
      CHECK(args.value<int32>(WritePoleFigureFilter::k_ImageSize_Key) == 5);
      CHECK(args.value<bool>(WritePoleFigureFilter::k_UseMask_Key) == true);
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CellPhasesArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Legacy conversion uses the default output geometry path.
      // The old filter had no geometry parameter.
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_ImageGeometryPath_Key) == DataPath({"PoleFigure"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(WritePoleFigureFilter::k_MaterialNameArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
    }
  }
}
