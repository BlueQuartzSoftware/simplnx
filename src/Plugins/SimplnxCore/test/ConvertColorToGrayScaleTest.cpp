#include "SimplnxCore/Filters/ConvertColorToGrayScaleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

#include <array>
#include <atomic>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::types;
namespace fs = std::filesystem;

const std::string k_MGeomName = "VertexGeom";
const std::string k_MDataArrayName = "DataArray";
const std::string k_MOutputArrayPrefix = "grayTestImage";
const size_t numTuples = 16;
const std::vector<std::vector<uint8_t>> k_TestColors{
    {0, 0, 0},       // black
    {0, 0, 128},     // navy
    {0, 0, 255},     // blue
    {128, 0, 0},     // maroon
    {128, 0, 128},   // purple
    {255, 0, 0},     // red
    {255, 0, 255},   // fuchsia
    {0, 128, 0},     // green
    {0, 128, 128},   // teal
    {128, 128, 0},   // olive
    {128, 128, 128}, // gray
    {0, 255, 0},     // lime
    {192, 192, 192}, // silver
    {0, 255, 255},   // aqua
    {255, 255, 0},   // yellow
    {255, 255, 255}, // white
};
const std::vector<uint8_t> k_CheckDefaultLuminosityColors{
    0,   // black
    9,   // navy
    18,  // blue
    27,  // maroon
    36,  // purple
    54,  // red
    73,  // fuchsia
    92,  // green
    101, // teal
    119, // olive
    128, // gray
    182, // lime
    192, // silver
    201, // aqua
    237, // yellow
    255, // white
};
const std::vector<uint8_t> k_CheckCustomLuminosityColors{
    0,   // black
    96,  // navy
    191, // blue
    96,  // maroon
    192, // purple
    191, // red
    127, // fuchsia
    96,  // green
    192, // teal
    192, // olive
    32,  // gray
    191, // lime
    176, // silver
    127, // aqua
    127, // yellow
    62,  // white
};
const std::vector<uint8_t> k_CheckLessZeroLuminosityColors{
    0,   // black
    160, // navy
    65,  // blue
    160, // maroon
    64,  // purple
    65,  // red
    129, // fuchsia
    160, // green
    64,  // teal
    64,  // olive
    224, // gray
    65,  // lime
    80,  // silver
    129, // aqua
    129, // yellow
    194, // white
};
const std::vector<uint8_t> k_CheckGreaterOneLuminosityColors{
    0,   // black
    224, // navy
    190, // blue
    224, // maroon
    192, // purple
    190, // red
    125, // fuchsia
    224, // green
    192, // teal
    192, // olive
    160, // gray
    190, // lime
    240, // silver
    125, // aqua
    125, // yellow
    59,  // white
};
const std::vector<uint8_t> k_CheckLessNegativeOneLuminosityColors{
    0,   // black
    32,  // navy
    66,  // blue
    32,  // maroon
    64,  // purple
    66,  // red
    131, // fuchsia
    32,  // green
    64,  // teal
    64,  // olive
    96,  // gray
    66,  // lime
    16,  // silver
    131, // aqua
    131, // yellow
    197, // white
};
const std::vector<uint8_t> k_CheckAverageColors{
    0,   // black
    43,  // navy
    85,  // blue
    43,  // maroon
    85,  // purple
    85,  // red
    170, // fuchsia
    43,  // green
    85,  // teal
    85,  // olive
    128, // gray
    85,  // lime
    192, // silver
    170, // aqua
    170, // yellow
    255, // white
};
const std::vector<uint8_t> k_CheckLightnessColors{
    0,   // black
    64,  // navy
    128, // blue
    64,  // maroon
    64,  // purple
    128, // red
    128, // fuchsia
    64,  // green
    64,  // teal
    64,  // olive
    128, // gray
    128, // lime
    192, // silver
    128, // aqua
    128, // yellow
    255, // white
};
const std::vector<uint8_t> k_CheckRChannelColors{
    0,   // black
    0,   // navy
    0,   // blue
    128, // maroon
    128, // purple
    255, // red
    255, // fuchsia
    0,   // green
    0,   // teal
    128, // olive
    128, // gray
    0,   // lime
    192, // silver
    0,   // aqua
    255, // yellow
    255, // white
};
const std::vector<uint8_t> k_CheckGChannelColors{
    0,   // black
    0,   // navy
    0,   // blue
    0,   // maroon
    0,   // purple
    0,   // red
    0,   // fuchsia
    128, // green
    128, // teal
    128, // olive
    128, // gray
    255, // lime
    192, // silver
    255, // aqua
    255, // yellow
    255, // white
};
const std::vector<uint8_t> k_CheckBChannelColors{
    0,   // black
    128, // navy
    255, // blue
    0,   // maroon
    128, // purple
    0,   // red
    255, // fuchsia
    0,   // green
    128, // teal
    0,   // olive
    128, // gray
    0,   // lime
    192, // silver
    255, // aqua
    0,   // yellow
    255, // white
};
const std::vector<std::vector<uint8_t>> k_AlgorithmMap{k_CheckDefaultLuminosityColors,
                                                       k_CheckCustomLuminosityColors,
                                                       k_CheckLessZeroLuminosityColors,
                                                       k_CheckGreaterOneLuminosityColors,
                                                       k_CheckLessNegativeOneLuminosityColors,
                                                       k_CheckAverageColors,
                                                       k_CheckLightnessColors,
                                                       k_CheckRChannelColors,
                                                       k_CheckGChannelColors,
                                                       k_CheckBChannelColors};

//------------------------------------------------------------------------------
static void SetDataArrayTestValues(UInt8ArrayType* aa)
{
  aa->fill(0);
  size_t index = 0;
  for(const auto& eachColor : k_TestColors)
  {
    aa->getDataStore()->setTuple(index++, eachColor);
  }
}

//------------------------------------------------------------------------------
static VertexGeom* CreateVertexGeometry(DataStructure& dataStructure, const std::vector<size_t>& tDims, const std::vector<size_t>& cDims)
{
  VertexGeom* vertexGeoPtr = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArrayPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, "Vertices", tDims, cDims, vertexGeoPtr->getId());
  vertexGeoPtr->setVertices(*vertexArrayPtr);
  auto& vertices = vertexArrayPtr->getDataStoreRef();
  for(usize i = 0; i < numTuples; ++i)
  {
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }
  return vertexGeoPtr;
}

//------------------------------------------------------------------------------
static void CompareResults(const uint8& algoMapIndex, const DataStructure& dataStructure)
{
  DataPath arrayName({k_MGeomName});
  arrayName = arrayName.createChildPath(k_MOutputArrayPrefix + k_MDataArrayName);
  const auto* testArrayPtr = dataStructure.getDataAs<UInt8ArrayType>(arrayName);
  REQUIRE(testArrayPtr != nullptr);
  const auto& testData = testArrayPtr->getDataStoreRef();

  const std::vector<uint8>& colorArray{k_AlgorithmMap[algoMapIndex]};

  for(size_t index = 0; index < colorArray.size(); ++index)
  {
    uint8 testVal = testData[index];
    uint8 exemplarVal = colorArray[index];
    REQUIRE(testVal == exemplarVal);
  }
}

//------------------------------------------------------------------------------
static void RunTest(const uint8& algoMapIndex, const ConvertColorToGrayScale::ConversionType& algorithm, const std::vector<float32>& colorWeights, const int32& colorChannel, bool shouldFail)
{
  DataStructure dataStruct;
  const std::vector<size_t> tDims{numTuples};
  const std::vector<size_t> cDims{3};

  // NOTE: This filter has no geometry requirements
  VertexGeom* vertexGeoPtr = CreateVertexGeometry(dataStruct, tDims, cDims);
  auto* testAaPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStruct, k_MDataArrayName, tDims, cDims, vertexGeoPtr->getId());
  SetDataArrayTestValues(testAaPtr);

  std::vector<DataPath> daps = {DataPath({k_MGeomName, k_MDataArrayName})};

  // Configure the filter arguments.
  ConvertColorToGrayScaleFilter filter;
  Arguments args;

  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(algorithm)));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32ParameterType::ValueType>(colorWeights));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorChannel_Key, std::make_any<int32>(colorChannel));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayPath_Key, std::make_any<MultiArraySelectionParameter::ValueType>(daps));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>(k_MOutputArrayPrefix));

  auto preflightResult = filter.preflight(dataStruct, args);
  if(shouldFail)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  }

  auto executeResult = filter.execute(dataStruct, args);
  if(shouldFail)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  if(shouldFail)
  {
    return;
  }

  CompareResults(algoMapIndex, dataStruct);

  unit_test::CheckArraysInheritTupleDims(dataStruct);
}

TEST_CASE("SimplnxCore::ConvertColorToGrayScale: Valid Execution", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);
  unit_test::LoadPlugins();

  // Luminosity Algorithm testing
  // Test defaults
  std::cout << "Testing luminosity algorithm..." << '\n';
  std::cout << "Default weights (0.2125, 0.7154, 0.0721)...";
  scope.execute([&] { RunTest(0, ConvertColorToGrayScale::ConversionType::Luminosity, {0.2125f, 0.7154f, 0.0721f}, 0, false); });

  // Test custom
  std::vector<float32> colorWeights{0.75, 0.75, 0.75};
  std::cout << "Custom weights (0.75, 0.75, 0.75)..." << '\n';
  RunTest(1, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test <0
  colorWeights = {-0.75, -0.75, -0.75};
  std::cout << "Testing weights < 0 (-0.75, -0.75, -0.75)..." << '\n';
  RunTest(2, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test >1
  colorWeights = {1.75, 1.75, 1.75};
  std::cout << "Testing weights > 1 (1.75, 1.75, 1.75)..." << '\n';
  RunTest(3, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test <-1
  colorWeights = {-1.75, -1.75, -1.75};
  std::cout << "Testing weights < -1 (-1.75, -1.75, -1.75)..." << '\n';
  RunTest(4, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Average Algorithm testing
  std::cout << "Testing average algorithm..." << '\n';
  scope.execute([&] { RunTest(5, ConvertColorToGrayScale::ConversionType::Average, {0.2125f, 0.7154f, 0.0721f}, 0, false); });

  // Lightness Algorithm testing
  std::cout << "Testing lightness algorithm..." << '\n';
  scope.execute([&] { RunTest(6, ConvertColorToGrayScale::ConversionType::Lightness, {0.2125f, 0.7154f, 0.0721f}, 0, false); });

  // Single Channel Algorithm testing
  // Red channel
  std::cout << "Testing red channel algorithm..." << '\n';
  scope.execute([&] { RunTest(7, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 0, false); });

  // Green channel
  std::cout << "Testing green channel algorithm..." << '\n';
  scope.execute([&] { RunTest(8, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 1, false); });

  // Blue channel
  std::cout << "Testing blue channel algorithm..." << '\n';
  scope.execute([&] { RunTest(9, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 2, false); });
}

TEST_CASE("SimplnxCore::ConvertColorToGrayScale: Multiple input arrays", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  unit_test::LoadPlugins();
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;
  const std::array<std::string, 2> matrixNames = {"ColorsA", "ColorsB"};
  const std::array<std::vector<uint8>, 2> colors = {{{100, 0, 0, 0, 100, 0}, {0, 0, 100, 100, 100, 100, 200, 100, 0}}};
  // Round 0.2125*R + 0.7154*G + 0.0721*B to the nearest integer:
  // A: 21.25 -> 21, 71.54 -> 72. B: 7.21 -> 7, 100 -> 100, 114.04 -> 114.
  const std::array<std::vector<uint8>, 2> expectedGray = {{{21, 72}, {7, 100, 114}}};
  MultiArraySelectionParameter::ValueType inputPaths;
  for(usize arrayIndex = 0; arrayIndex < colors.size(); ++arrayIndex)
  {
    const usize tupleCount = expectedGray[arrayIndex].size();
    auto* matrixPtr = AttributeMatrix::Create(dataStructure, matrixNames[arrayIndex], {tupleCount});
    REQUIRE(matrixPtr != nullptr);
    auto* inputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "RGB", {tupleCount}, {3}, matrixPtr->getId());
    REQUIRE(inputPtr != nullptr);
    const auto writeResult = inputPtr->getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(colors[arrayIndex].data(), colors[arrayIndex].size()));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    inputPaths.emplace_back(DataPath({matrixNames[arrayIndex], "RGB"}));
  }

  ConvertColorToGrayScaleFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32ParameterType::ValueType>(std::vector<float32>{0.2125F, 0.7154F, 0.0721F}));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayPath_Key, std::make_any<MultiArraySelectionParameter::ValueType>(inputPaths));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<std::string>("Gray_"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.output_actions);
  auto execution = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execution.result);

  for(usize arrayIndex = 0; arrayIndex < inputPaths.size(); ++arrayIndex)
  {
    const DataPath outputPath({matrixNames[arrayIndex], "Gray_RGB"});
    CAPTURE(outputPath.toString());
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(outputPath));
    const auto& output = dataStructure.getDataRefAs<UInt8ArrayType>(outputPath);
    REQUIRE(output.getNumberOfTuples() == expectedGray[arrayIndex].size());
    REQUIRE(output.getNumberOfComponents() == 1);
    for(usize tuple = 0; tuple < expectedGray[arrayIndex].size(); ++tuple)
    {
      CHECK(output[tuple] == expectedGray[arrayIndex][tuple]);
    }
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(inputPaths[arrayIndex]));
    const auto& input = dataStructure.getDataRefAs<UInt8ArrayType>(inputPaths[arrayIndex]);
    for(usize value = 0; value < colors[arrayIndex].size(); ++value)
    {
      CHECK(input[value] == colors[arrayIndex][value]);
    }
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConvertColorToGrayScale: Mismatched output path count", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  unit_test::LoadPlugins();
  const usize outputCount = GENERATE(1, 3);
  CAPTURE(outputCount);
  DataStructure dataStructure;
  for(const auto& name : {"A", "B"})
  {
    auto* inputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, name, {1}, {3});
    REQUIRE(inputPtr != nullptr);
    inputPtr->fill(11);
  }
  auto* outputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Gray", {1}, {1});
  REQUIRE(outputPtr != nullptr);
  outputPtr->fill(199);

  ConvertColorToGrayScaleInputValues inputValues{};
  inputValues.conversion_algorithm = 3;
  inputValues.color_weights = {0.2125F, 0.7154F, 0.0721F};
  inputValues.input_data_array_paths = {DataPath({"A"}), DataPath({"B"})};
  inputValues.output_data_array_paths.assign(outputCount, DataPath({"Gray"}));
  // Invalid counts must be rejected before selecting an algorithm or writing any output.
  const auto result = ConvertColorToGrayScaleArrays(dataStructure, inputValues, {}, std::atomic_bool{false});
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -10706);
  CHECK(result.errors()[0].message == fmt::format("Grayscale conversion needs one output path for each input path. Input paths: 2. Output paths: {}.", outputCount));
  CHECK((*outputPtr)[0] == 199);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ConvertColorToGrayScaleFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ConvertColorToGrayScaleFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ConvertColorToGrayScaleFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ConvertColorToGrayScaleFilter.json"},
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
      REQUIRE(filterPtr->uuid() == FilterTraits<ConvertColorToGrayScaleFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key) == 0);
      // Successful pipeline loading verifies the FloatVec3FilterParameterConverter value.
      CHECK(args.value<int32>(ConvertColorToGrayScaleFilter::k_ColorChannel_Key) == 5);
      // Successful pipeline loading verifies the MultiDataArraySelectionFilterParameterConverter value.
      CHECK(args.value<std::string>(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::PreflightColorToGrayScale: rejects invalid component relationships", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  struct RejectionCase
  {
    ChoicesParameter::ValueType Algorithm;
    usize components;
    int32 channel;
    int32 error;
  };
  const auto testCase = GENERATE(from_range(std::vector<RejectionCase>{{0, 1, 0, -10710},
                                                                       {0, 2, 0, -10710},
                                                                       {1, 1, 0, -10710},
                                                                       {1, 2, 0, -10710},
                                                                       {2, 1, 0, -10710},
                                                                       {2, 2, 0, -10710},
                                                                       {3, 1, -1, -10711},
                                                                       {3, 1, 1, -10711},
                                                                       {3, 2, -1, -10711},
                                                                       {3, 2, 2, -10711},
                                                                       {3, 3, 3, -10711},
                                                                       {3, 4, 4, -10711}}));
  DataStructure dataStructure;
  auto* inputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {2}, {testCase.components});
  REQUIRE(inputPtr != nullptr);
  ConvertColorToGrayScaleInputValues values{};
  values.conversion_algorithm = testCase.Algorithm;
  values.color_weights = {0.25f, 0.5f, 0.25f};
  values.color_channel = testCase.channel;
  values.input_data_array_paths = {DataPath({"Input"})};
  values.output_array_prefix = "gray";
  auto result = PreflightColorToGrayScale(dataStructure, values);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.output_actions);
  REQUIRE_FALSE(result.output_actions.errors().empty());
  REQUIRE(result.output_actions.errors().front().code == testCase.error);
}

TEST_CASE("SimplnxCore::PreflightColorToGrayScale: accepts valid component relationships", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const auto algorithm = GENERATE(0, 1, 2, 3);
  const auto components = GENERATE(3, 4);
  DataStructure dataStructure;
  auto* inputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {1}, {static_cast<usize>(components)});
  REQUIRE(inputPtr != nullptr);
  (*inputPtr)[0] = 200;
  (*inputPtr)[1] = 100;
  (*inputPtr)[2] = 50;
  if(components == 4)
  {
    (*inputPtr)[3] = 3;
  }
  ConvertColorToGrayScaleInputValues values{};
  values.conversion_algorithm = algorithm;
  values.color_weights = {0.25f, 0.5f, 0.25f};
  values.color_channel = components - 1;
  values.input_data_array_paths = {DataPath({"Input"})};
  values.output_array_prefix = "gray";
  auto pre = PreflightColorToGrayScale(dataStructure, values);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.output_actions);
  auto actionResult = pre.output_actions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actionResult);
  const std::atomic_bool cancel = false;
  scope.execute([&] {
    auto conversionResult = ConvertColorToGrayScaleArrays(dataStructure, values, {}, cancel);
    SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
  });
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(DataPath({"grayInput"})));
  const auto& output = dataStructure.getDataRefAs<UInt8ArrayType>(DataPath({"grayInput"}));
  const std::vector<uint8> expected = components == 3 ? std::vector<uint8>{113, 117, 125, 50} : std::vector<uint8>{113, 117, 125, 3};
  REQUIRE(output[0] == expected[algorithm]);
}

TEST_CASE("SimplnxCore::ConvertColorToGrayScale: single channel accepts scalar and two component input", "[SimplnxCore][ConvertColorToGrayScaleFilter]")
{
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  unit_test::AlgorithmTestScope scope(scenario);
  const usize components = GENERATE(1, 2);
  DataStructure dataStructure;
  auto* inputPtr = UInt8ArrayType::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {1}, {components});
  REQUIRE(inputPtr != nullptr);
  (*inputPtr)[0] = 64;
  if(components == 2)
  {
    (*inputPtr)[1] = 50;
  }
  ConvertColorToGrayScaleInputValues values{};
  values.conversion_algorithm = 3;
  values.color_weights = {0.25f, 0.5f, 0.25f};
  values.color_channel = static_cast<int32>(components - 1);
  values.input_data_array_paths = {DataPath({"Input"})};
  values.output_array_prefix = "gray";
  auto pre = PreflightColorToGrayScale(dataStructure, values);
  SIMPLNX_RESULT_REQUIRE_VALID(pre.output_actions);
  auto actionResult = pre.output_actions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actionResult);
  const std::atomic_bool cancel = false;
  scope.execute([&] {
    auto conversionResult = ConvertColorToGrayScaleArrays(dataStructure, values, {}, cancel);
    SIMPLNX_RESULT_REQUIRE_VALID(conversionResult);
  });
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8ArrayType>(DataPath({"grayInput"})));
  REQUIRE(dataStructure.getDataRefAs<UInt8ArrayType>(DataPath({"grayInput"}))[0] == (components == 1 ? 64 : 50));
}
