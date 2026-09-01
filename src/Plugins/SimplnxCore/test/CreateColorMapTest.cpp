#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>

#include <nonstd/span.hpp>

#include "SimplnxCore/Filters/CreateColorMapFilter.hpp"
#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <limits>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path k_TestFilesDir = fs::path(nx::core::unit_test::k_DREAM3DDataDir.str()) / "TestFiles" / "generate_color_table_test";
const fs::path k_InputImageFilePath = k_TestFilesDir / "ColorTableTestFile.txt";

const fs::path k_BlackBlueWhitePresetPath = k_TestFilesDir / "BlackBlueWhite.txt";
const fs::path k_BlackOrangeWhitePresetPath = k_TestFilesDir / "BlackOrangeWhite.txt";
const fs::path k_BlackBodyRadiationPresetPath = k_TestFilesDir / "BlackBodyRadiation.txt";
const fs::path k_BlueToYellowPresetPath = k_TestFilesDir / "BlueToYellow.txt";
const fs::path k_ColdAndHotPresetPath = k_TestFilesDir / "ColdAndHot.txt";
const fs::path k_GrayscalePresetPath = k_TestFilesDir / "Grayscale.txt";
const fs::path k_HazePresetPath = k_TestFilesDir / "Haze.txt";
const fs::path k_HSVPresetPath = k_TestFilesDir / "Hsv.txt";
const fs::path k_JetPresetPath = k_TestFilesDir / "Jet.txt";
const fs::path k_RainbowBlendedBlackPresetPath = k_TestFilesDir / "RainbowBlendedBlack.txt";
const fs::path k_RainbowBlendedGreyPresetPath = k_TestFilesDir / "RainbowBlendedGrey.txt";
const fs::path k_RainbowBlendedWhitePresetPath = k_TestFilesDir / "RainbowBlendedWhite.txt";
const fs::path k_RainbowDesaturatedPresetPath = k_TestFilesDir / "RainbowDesaturated.txt";
const fs::path k_RainbowPresetPath = k_TestFilesDir / "Rainbow.txt";
const fs::path k_XRayPresetPath = k_TestFilesDir / "XRay.txt";

const std::string k_BlackBlueWhitePresetName = "Black, Blue and White";
const std::string k_BlackOrangeWhitePresetName = "Black, Orange and White";
const std::string k_BlackBodyRadiationPresetName = "Black-Body Radiation";
const std::string k_BlueToYellowPresetName = "Blue to Yellow";
const std::string k_ColdAndHotPresetName = "Cold and Hot";
const std::string k_GrayscalePresetName = "Grayscale";
const std::string k_HazePresetName = "Haze";
const std::string k_HSVPresetName = "hsv";
const std::string k_JetPresetName = "Jet";
const std::string k_RainbowBlendedBlackPresetName = "Rainbow Blended Black";
const std::string k_RainbowBlendedGreyPresetName = "Rainbow Blended Grey";
const std::string k_RainbowBlendedWhitePresetName = "Rainbow Blended White";
const std::string k_RainbowDesaturatedPresetName = "Rainbow Desaturated";
const std::string k_RainbowPresetName = "Rainbow";
const std::string k_XRayPresetName = "X Ray";

constexpr usize k_BenchmarkDim = 200;
constexpr usize k_BenchmarkSliceTuples = k_BenchmarkDim * k_BenchmarkDim;
constexpr usize k_BenchmarkTotalTuples = k_BenchmarkDim * k_BenchmarkDim * k_BenchmarkDim;
// Samples the preset endpoints, both internal control-point boundaries, and each interval midpoint.
constexpr std::array<uint16, 7> k_BenchmarkInputValues = {0, 166, 333, 500, 666, 833, 1000};
constexpr std::array<std::array<uint8, 3>, 7> k_BenchmarkExpectedColors = {std::array<uint8, 3>{0, 0, 0},      std::array<uint8, 3>{0, 0, 63},    std::array<uint8, 3>{0, 0, 128},
                                                                           std::array<uint8, 3>{0, 64, 191},   std::array<uint8, 3>{0, 128, 255}, std::array<uint8, 3>{127, 191, 255},
                                                                           std::array<uint8, 3>{255, 255, 255}};
const DataPath k_BenchmarkInputPath({"Color Map Input"});
const std::string k_BenchmarkOutputName = "Color Map RGB";
const DataPath k_BenchmarkOutputPath({k_BenchmarkOutputName});

std::map<std::string, nlohmann::json> ReadPresets()
{
  Result<nlohmann::json> result = ColorTableUtilities::LoadAllRGBPresets();
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::map<std::string, nlohmann::json> presetsMap;
  for(const nlohmann::json& preset : result.value())
  {
    if(ColorTableUtilities::IsValidPreset(preset))
    {
      presetsMap.insert({preset["Name"].get<std::string>(), preset});
    }
  }

  return presetsMap;
}
} // namespace

TEST_CASE("SimplnxCore::CreateColorMapFilter: Valid filter execution")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "generate_color_table_test.tar.gz", "generate_color_table_test");

  DataStructure dataStructure;

  std::map<std::string, nlohmann::json> presetsMap = ReadPresets();

  // Load the image input.
  {
    const ReadTextDataArrayFilter filter;
    Arguments args;

    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(k_InputImageFilePath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(37989)}}));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(DataPath{{Constants::k_Confidence_Index.str()}}));

    IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Apply the selected color preset.
  const CreateColorMapFilter filter;
  Arguments args;
  fs::path presetFilePath;

  SECTION(k_BlackBlueWhitePresetName)
  {
    REQUIRE(!presetsMap[k_BlackBlueWhitePresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackBlueWhitePresetName));
    presetFilePath = k_BlackBlueWhitePresetPath;
  }
  SECTION(k_BlackOrangeWhitePresetName)
  {
    REQUIRE(!presetsMap[k_BlackOrangeWhitePresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackOrangeWhitePresetName));
    presetFilePath = k_BlackOrangeWhitePresetPath;
  }
  SECTION(k_BlackBodyRadiationPresetName)
  {
    REQUIRE(!presetsMap[k_BlackBodyRadiationPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackBodyRadiationPresetName));
    presetFilePath = k_BlackBodyRadiationPresetPath;
  }
  SECTION(k_BlueToYellowPresetName)
  {
    REQUIRE(!presetsMap[k_BlueToYellowPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlueToYellowPresetName));
    presetFilePath = k_BlueToYellowPresetPath;
  }
  SECTION(k_ColdAndHotPresetName)
  {
    REQUIRE(!presetsMap[k_ColdAndHotPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_ColdAndHotPresetName));
    presetFilePath = k_ColdAndHotPresetPath;
  }
  SECTION(k_GrayscalePresetName)
  {
    REQUIRE(!presetsMap[k_GrayscalePresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_GrayscalePresetName));
    presetFilePath = k_GrayscalePresetPath;
  }
  SECTION(k_HazePresetName)
  {
    REQUIRE(!presetsMap[k_HazePresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_HazePresetName));
    presetFilePath = k_HazePresetPath;
  }
  SECTION(k_HSVPresetName)
  {
    REQUIRE(!presetsMap[k_HSVPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_HSVPresetName));
    presetFilePath = k_HSVPresetPath;
  }
  SECTION(k_JetPresetName)
  {
    REQUIRE(!presetsMap[k_JetPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_JetPresetName));
    presetFilePath = k_JetPresetPath;
  }
  SECTION(k_RainbowBlendedBlackPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedBlackPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedBlackPresetName));
    presetFilePath = k_RainbowBlendedBlackPresetPath;
  }
  SECTION(k_RainbowBlendedGreyPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedGreyPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedGreyPresetName));
    presetFilePath = k_RainbowBlendedGreyPresetPath;
  }
  SECTION(k_RainbowBlendedWhitePresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedWhitePresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedWhitePresetName));
    presetFilePath = k_RainbowBlendedWhitePresetPath;
  }
  SECTION(k_RainbowDesaturatedPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowDesaturatedPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowDesaturatedPresetName));
    presetFilePath = k_RainbowDesaturatedPresetPath;
  }
  SECTION(k_RainbowPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowPresetName));
    presetFilePath = k_RainbowPresetPath;
  }
  SECTION(k_XRayPresetName)
  {
    REQUIRE(!presetsMap[k_XRayPresetName].empty());
    args.insertOrAssign(CreateColorMapFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_XRayPresetName));
    presetFilePath = k_XRayPresetPath;
  }

  {
    args.insertOrAssign(CreateColorMapFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(DataPath{{Constants::k_Confidence_Index.str()}}));
    args.insertOrAssign(CreateColorMapFilter::k_RgbArrayPath_Key, std::make_any<std::string>("CI_RGB"));

    IFilter::ExecuteResult executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    // Validate the generated colors.
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(DataPath{{"CI_RGB"}}));
    const UInt8Array& resultArray = dataStructure.getDataRefAs<UInt8Array>(DataPath{{"CI_RGB"}});
    const AbstractDataStore<uint8>& resultStore = resultArray.getDataStoreRef();

    std::string buf;
    std::ifstream inStream(presetFilePath);
    usize currentLine = 0;
    while(!inStream.eof())
    {
      std::getline(inStream, buf);
      std::vector<std::string> list = StringUtilities::split(buf, ',');
      for(int i = 0; i < list.size(); i++)
      {
        REQUIRE_NOTHROW(std::stoi(list[i]));
        const uint8 exemplar = std::stoi(list[i]);
        const uint8 generated = resultStore.getComponentValue(currentLine, i);
        REQUIRE(exemplar == generated);
      }
      currentLine++;
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ColorTableUtilities: RGB interpolation helpers", "[SimplnxCore][ColorTableUtilities]")
{
  // Two-color black->white map. Flattened control points: {A, R, G, B} per color.
  const std::vector<float32> twoColor = {0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F};
  const std::vector<float32> binPoints2 = ColorTableUtilities::NormalizeBinPoints(twoColor);
  REQUIRE(binPoints2.size() == 2);
  REQUIRE(binPoints2[0] == Approx(0.0F));
  REQUIRE(binPoints2[1] == Approx(1.0F));

  // Midpoint of black->white: 0.5 * 255 == 127.5 -> truncates to 127.
  const std::array<uint8, 3> mid = ColorTableUtilities::ComputeRgbFromControlPoints(0.5F, binPoints2, twoColor, 2);
  REQUIRE(mid[0] == 127);
  REQUIRE(mid[1] == 127);
  REQUIRE(mid[2] == 127);

  // Three-color map with non-[0,1] A-values -> normalized bin points {0, 0.5, 1}.
  const std::vector<float32> threeColor = {0.0F, 0.0F, 0.0F, 0.0F, 5.0F, 1.0F, 0.0F, 0.0F, 10.0F, 0.0F, 0.0F, 1.0F};
  const std::vector<float32> binPoints3 = ColorTableUtilities::NormalizeBinPoints(threeColor);
  REQUIRE(binPoints3[1] == Approx(0.5F));
  const std::array<uint8, 3> pureRed = ColorTableUtilities::ComputeRgbFromControlPoints(0.5F, binPoints3, threeColor, 3);
  REQUIRE(pureRed[0] == 255);
  REQUIRE(pureRed[1] == 0);
  REQUIRE(pureRed[2] == 0);

  // min == max guard: normalized value is 0.0 regardless of value.
  REQUIRE(ColorTableUtilities::NormalizeValue<int32>(5, 5, 5) == Approx(0.0F));
  REQUIRE(ColorTableUtilities::NormalizeValue<float32>(2.0F, 0.0F, 4.0F) == Approx(0.5F));

  // Negative-min normalization: differences must be handled without signed-integer overflow.
  REQUIRE(ColorTableUtilities::NormalizeValue<int32>(-8, -8, 7) == Approx(0.0F));
  REQUIRE(ColorTableUtilities::NormalizeValue<int32>(7, -8, 7) == Approx(1.0F));
  REQUIRE(ColorTableUtilities::NormalizeValue<int32>(0, -8, 7) == Approx(8.0F / 15.0F));

  // Full int64 range would overflow a signed subtraction; the hardened helper returns a finite value in [0, 1].
  const float32 fullRange = ColorTableUtilities::NormalizeValue<int64>(0, std::numeric_limits<int64>::min(), std::numeric_limits<int64>::max());
  REQUIRE(std::isfinite(fullRange));
  REQUIRE(fullRange >= 0.0F);
  REQUIRE(fullRange <= 1.0F);
  REQUIRE(fullRange == Approx(0.5F));

  // Non-finite guard: NaN inputs deterministically map to the first control color (0.0F).
  REQUIRE(ColorTableUtilities::NormalizeValue<float32>(std::numeric_limits<float32>::quiet_NaN(), 0.0F, 1.0F) == 0.0F);

  // Degenerate bin points: a single control color must not produce NaN from a 0/0 divide.
  const std::vector<float32> singleColor = {0.5F, 1.0F, 0.0F, 0.0F};
  const std::vector<float32> binPoints1 = ColorTableUtilities::NormalizeBinPoints(singleColor);
  REQUIRE(binPoints1.size() == 1);
  REQUIRE(binPoints1[0] == Approx(0.0F));
}

TEST_CASE("SimplnxCore::CreateColorMapFilter: SIMPL Backwards Compatibility", "[SimplnxCore][CreateColorMapFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "CreateColorMapFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "CreateColorMapFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<CreateColorMapFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<std::string>(CreateColorMapFilter::k_SelectedPreset_Key) == "TestName");
      CHECK(args.value<DataPath>(CreateColorMapFilter::k_SelectedDataArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(CreateColorMapFilter::k_RgbArrayPath_Key) == "TestName");
    }
  }
}
