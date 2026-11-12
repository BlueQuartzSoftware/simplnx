#include <catch2/catch.hpp>

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
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

#include <algorithm>
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

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({geomName})));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(DataPath({geomName})).getUnits() == IGeometry::LengthUnit::Unspecified);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(DataPath({intensityName})));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(DataPath({intensityName})).getUnits() == IGeometry::LengthUnit::Unspecified);

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
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(128));
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
    std::vector<float64> values(128 * 128);
    auto readResult = output.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float64>(values.data(), values.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    REQUIRE(std::accumulate(values.begin(), values.end(), 0.0) == expectedSums[familyIdx]);
    if(familyIdx == 0)
    {
      // Two cubic <001> families give four poles at the center. Identity gives
      // four cardinal equatorial poles; the 45-degree tail gives four diagonals.
      // The stored image reverses row order, so the center is at (64,63).
      std::vector<float64> expected(128 * 128, 0.0);
      expected[63 * 128 + 64] = 4.0;
      for(usize index : {63 * 128 + 1, 63 * 128 + 127, 64, 126 * 128 + 64, 19 * 128 + 20, 19 * 128 + 108, 107 * 128 + 20, 107 * 128 + 108})
      {
        expected[index] = 1.0;
      }
      REQUIRE(values == expected);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: composite dimensions use the measured X width", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::TestFileSentinel sentinel(unit_test::k_TestFilesDir, "Pole_Figure_Exemplars_v6.tar.gz", "Pole_Figure_Exemplars_v6");
  auto dataStructure = UnitTest::LoadDataStructure(fs::path(unit_test::k_TestFilesDir.view()) / "Pole_Figure_Exemplars_v6/Pole_Figure_Exemplars_v6.dream3d");
  const WritePoleFigureFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"Imported Data", "Eulers"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Imported Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix", "PhaseNames"})));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Layout Pole Figures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Layout Counts"})));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(128));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageLayout_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveAsImageGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(WritePoleFigureFilter::k_SaveIntensityDataArrays, std::make_any<bool>(false));

  // Independent font-table oracle: embedded LatoBold.hpp, cmap 'X' -> X,
  // hmtx advance 1358, head units_per_em 2000 (read with fontTools.ttLib).
  // At Image Size 128: font = 8, margin = 4, X width = 1358 * 8 / 2000 = 5.432.
  // Horizontal width = trunc(4 * (4 + 128 + 5.432 + 4)) = 565.
  // Height = (4 + 8) + (4 + 8 + 128 + 16 + 8) = 176.
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto preview = dataStructure;
  auto applied = preflight.outputActions.value().applyAll(preview, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(applied);
  const DataPath geometryPath({"Layout Pole Figures"});
  REQUIRE_NOTHROW(preview.getDataRefAs<ImageGeom>(geometryPath));
  // This checks the X width that preflight measures. The preflight value is the current provisional
  // behavior, not the rendered size: preflight passes the page dimensions in ZYX order and leaves out
  // a margin, so it differs from execution (issue #1867, WritePoleFigure saved composite item).
  REQUIRE(preview.getDataRefAs<ImageGeom>(geometryPath).getDimensions() == SizeVec3{1, 176, 565});
  auto executed = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executed.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geometryPath));
  // The renderer includes a third margin per panel and truncates before multiplying:
  // 4 * trunc(128 + 3 * 4 + 5.432) = 580 (PoleFigureCompositor layout).
  // This value records the current EbsdLib compositor layout; only the X width above is an independent oracle.
  REQUIRE(dataStructure.getDataRefAs<ImageGeom>(geometryPath).getDimensions() == SizeVec3{580, 176, 1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::WritePoleFigureFilter: supported sampling dimensions", "[OrientationAnalysis][WritePoleFigureFilter]")
{
  const UnitTest::TestFileSentinel sentinel(unit_test::k_TestFilesDir, "Pole_Figure_Exemplars_v6.tar.gz", "Pole_Figure_Exemplars_v6");
  auto dataStructure = UnitTest::LoadDataStructure(fs::path(unit_test::k_TestFilesDir.view()) / "Pole_Figure_Exemplars_v6/Pole_Figure_Exemplars_v6.dream3d");
  WritePoleFigureFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(WritePoleFigureFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({"Imported Data", "Eulers"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({"Imported Data", "Phases"})));
  args.insertOrAssign(WritePoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix", "CrystalStructures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({"EnsembleAttributeMatrix", "PhaseNames"})));
  args.insertOrAssign(WritePoleFigureFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_WriteImageToDisk, std::make_any<bool>(false));
  args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WritePoleFigureFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Sampling Pole Figures"})));
  args.insertOrAssign(WritePoleFigureFilter::k_IntensityGeometryPath, std::make_any<DataPath>(DataPath({"Sampling Counts"})));
  struct Boundary
  {
    std::string key;
    int32 minimum;
    int32 maximum;
    int32 code;
  };
  const std::array<Boundary, 3> boundaries = {
      {{WritePoleFigureFilter::k_ImageSize_Key, 128, 8000, -680002}, {WritePoleFigureFilter::k_LambertSize_Key, 16, 1024, -680004}, {WritePoleFigureFilter::k_NumColors_Key, 4, 256, -680005}}};
  for(const auto& boundary : boundaries)
  {
    for(const int32 value : {boundary.minimum - 1, boundary.minimum, boundary.maximum, boundary.maximum + 1})
    {
      DYNAMIC_SECTION(boundary.key << " = " << value)
      {
        args.insertOrAssign(boundary.key, std::make_any<int32>(value));
        auto result = filter.preflight(dataStructure, args);
        if(value < boundary.minimum || value > boundary.maximum)
        {
          SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
          REQUIRE(result.outputActions.errors().size() == 1);
          REQUIRE(result.outputActions.errors()[0].code == boundary.code);
        }
        else
        {
          SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
        }
      }
    }
    if(boundary.key != WritePoleFigureFilter::k_ImageSize_Key)
    {
      for(const int32 value : {boundary.minimum - 1, boundary.maximum + 1})
      {
        DYNAMIC_SECTION("Discrete ignores " << boundary.key << " = " << value)
        {
          args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
          args.insertOrAssign(boundary.key, std::make_any<int32>(value));
          auto result = filter.preflight(dataStructure, args);
          SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
        }
      }
    }
  }
  for(const int32 value : {127, 128, 8000, 8001})
  {
    DYNAMIC_SECTION("Discrete Image Size = " << value)
    {
      args.insertOrAssign(WritePoleFigureFilter::k_GenerationAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(1));
      args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(value));
      auto result = filter.preflight(dataStructure, args);
      if(value == 127 || value == 8001)
      {
        SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
        REQUIRE(result.outputActions.errors().size() == 1);
        REQUIRE(result.outputActions.errors()[0].code == -680002);
      }
      else
      {
        SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
      }
    }
  }
  SECTION("Color Intensity reports all unsupported sampling dimensions")
  {
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(127));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(15));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(3));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 3);
    for(const int32 code : {-680002, -680004, -680005})
    {
      REQUIRE(std::count_if(result.outputActions.errors().begin(), result.outputActions.errors().end(), [code](const Error& error) { return error.code == code; }) == 1);
    }
  }
  SECTION("Color Intensity accepts all maximum dimensions")
  {
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(8000));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(1024));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(256));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  SECTION("Image Size 1 is unsupported")
  {
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(1));
    auto result = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors()[0].code == -680002);
  }
  SECTION("Color Intensity executes at all minimum dimensions")
  {
    args.insertOrAssign(WritePoleFigureFilter::k_ImageSize_Key, std::make_any<int32>(128));
    args.insertOrAssign(WritePoleFigureFilter::k_LambertSize_Key, std::make_any<int32>(16));
    args.insertOrAssign(WritePoleFigureFilter::k_NumColors_Key, std::make_any<int32>(4));
    auto result = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    const DataPath countsPath({"Sampling Counts", "Cell Data", "Phase_1_<001>"});
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(countsPath));
    REQUIRE(dataStructure.getDataRefAs<Float64Array>(countsPath).getNumberOfTuples() == 128 * 128);
    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
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
