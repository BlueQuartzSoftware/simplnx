#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/GenerateColorTableFilter.hpp"
#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path k_TestFilesDir = fs::path(nx::core::unit_test::k_DREAM3DDataDir.str()) / "TestFiles" / "GenerateColorTableTest";
const fs::path k_PresetsFilePath = k_TestFilesDir / "ColorTablePresets.json";
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
const std::string k_JetPresetName = "jet";
const std::string k_RainbowBlendedBlackPresetName = "Rainbow Blended Black";
const std::string k_RainbowBlendedGreyPresetName = "Rainbow Blended Grey";
const std::string k_RainbowBlendedWhitePresetName = "Rainbow Blended White";
const std::string k_RainbowDesaturatedPresetName = "Rainbow Desaturated";
const std::string k_RainbowPresetName = "rainbow";
const std::string k_XRayPresetName = "X Ray";

std::map<std::string, nlohmann::json> ReadPresets()
{
  Result<nlohmann::json> result = ColorTableUtilities::LoadAllRGBPresets();
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  std::map<std::string, nlohmann::json> presetsMap;
  for(const nlohmann::json& preset : result.value())
  {
    if(preset.contains("Name") && preset.contains("RGBPoints") && preset["Name"].is_string())
    {
      presetsMap.insert({preset["Name"].get<std::string>(), preset});
    }
  }

  return presetsMap;
}
} // namespace

TEST_CASE("SimplnxCore::GenerateColorTableFilter: Valid filter execution")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "GenerateColorTableTest.tar.gz", "GenerateColorTableTest");

  DataStructure dataStructure;

  std::map<std::string, nlohmann::json> presetsMap = ReadPresets();

  // Read Image File
  {
    const ReadTextDataArrayFilter filter;
    Arguments args;

    args.insertOrAssign(ReadTextDataArrayFilter::k_InputFileKey, std::make_any<fs::path>(k_InputImageFilePath));
    args.insertOrAssign(ReadTextDataArrayFilter::k_ScalarTypeKey, std::make_any<NumericType>(NumericType::float32));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NCompKey, std::make_any<uint64>(1));
    args.insertOrAssign(ReadTextDataArrayFilter::k_NTuplesKey, std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{static_cast<double>(37989)}}));
    args.insertOrAssign(ReadTextDataArrayFilter::k_DataArrayKey, std::make_any<DataPath>(DataPath{{Constants::k_Confidence_Index.str()}}));

    IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Apply Preset
  const GenerateColorTableFilter filter;
  Arguments args;
  fs::path presetFilePath;

  SECTION(k_BlackBlueWhitePresetName)
  {
    REQUIRE(!presetsMap[k_BlackBlueWhitePresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackBlueWhitePresetName));
    presetFilePath = k_BlackBlueWhitePresetPath;
  }
  SECTION(k_BlackOrangeWhitePresetName)
  {
    REQUIRE(!presetsMap[k_BlackOrangeWhitePresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackOrangeWhitePresetName));
    presetFilePath = k_BlackOrangeWhitePresetPath;
  }
  SECTION(k_BlackBodyRadiationPresetName)
  {
    REQUIRE(!presetsMap[k_BlackBodyRadiationPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlackBodyRadiationPresetName));
    presetFilePath = k_BlackBodyRadiationPresetPath;
  }
  SECTION(k_BlueToYellowPresetName)
  {
    REQUIRE(!presetsMap[k_BlueToYellowPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_BlueToYellowPresetName));
    presetFilePath = k_BlueToYellowPresetPath;
  }
  SECTION(k_ColdAndHotPresetName)
  {
    REQUIRE(!presetsMap[k_ColdAndHotPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_ColdAndHotPresetName));
    presetFilePath = k_ColdAndHotPresetPath;
  }
  SECTION(k_GrayscalePresetName)
  {
    REQUIRE(!presetsMap[k_GrayscalePresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_GrayscalePresetName));
    presetFilePath = k_GrayscalePresetPath;
  }
  SECTION(k_HazePresetName)
  {
    REQUIRE(!presetsMap[k_HazePresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_HazePresetName));
    presetFilePath = k_HazePresetPath;
  }
  SECTION(k_HSVPresetName)
  {
    REQUIRE(!presetsMap[k_HSVPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_HSVPresetName));
    presetFilePath = k_HSVPresetPath;
  }
  SECTION(k_JetPresetName)
  {
    REQUIRE(!presetsMap[k_JetPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_JetPresetName));
    presetFilePath = k_JetPresetPath;
  }
  SECTION(k_RainbowBlendedBlackPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedBlackPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedBlackPresetName));
    presetFilePath = k_RainbowBlendedBlackPresetPath;
  }
  SECTION(k_RainbowBlendedGreyPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedGreyPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedGreyPresetName));
    presetFilePath = k_RainbowBlendedGreyPresetPath;
  }
  SECTION(k_RainbowBlendedWhitePresetName)
  {
    REQUIRE(!presetsMap[k_RainbowBlendedWhitePresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowBlendedWhitePresetName));
    presetFilePath = k_RainbowBlendedWhitePresetPath;
  }
  SECTION(k_RainbowDesaturatedPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowDesaturatedPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowDesaturatedPresetName));
    presetFilePath = k_RainbowDesaturatedPresetPath;
  }
  SECTION(k_RainbowPresetName)
  {
    REQUIRE(!presetsMap[k_RainbowPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_RainbowPresetName));
    presetFilePath = k_RainbowPresetPath;
  }
  SECTION(k_XRayPresetName)
  {
    REQUIRE(!presetsMap[k_XRayPresetName].empty());
    args.insertOrAssign(GenerateColorTableFilter::k_SelectedPreset_Key, std::make_any<std::string>(k_XRayPresetName));
    presetFilePath = k_XRayPresetPath;
  }

  args.insertOrAssign(GenerateColorTableFilter::k_SelectedDataArrayPath_Key, std::make_any<DataPath>(DataPath{{Constants::k_Confidence_Index.str()}}));
  args.insertOrAssign(GenerateColorTableFilter::k_RgbArrayPath_Key, std::make_any<std::string>("CI_RGB"));

  IFilter::ExecuteResult executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Validate Results
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
