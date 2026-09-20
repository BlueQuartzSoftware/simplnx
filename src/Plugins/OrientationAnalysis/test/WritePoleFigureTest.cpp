#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
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
