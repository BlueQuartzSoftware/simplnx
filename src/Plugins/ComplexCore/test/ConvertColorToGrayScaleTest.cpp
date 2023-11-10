#include "ComplexCore/Filters/Algorithms/ConvertColorToGrayScale.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ConvertColorToGrayScaleFilter.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace complex;
using namespace complex::types;

const std::string m_GeomName = "VertexGeom";
const std::string m_DataArrayName = "DataArray";
const std::string m_outputArrayPrefix = "grayTestImage";
const size_t numTuples = 16;
const std::vector<std::vector<uint8_t>> testColors{
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
const std::vector<uint8_t> checkDefaultLuminosityColors{
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
const std::vector<uint8_t> checkCustomLuminosityColors{
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
const std::vector<uint8_t> checkLessZeroLuminosityColors{
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
const std::vector<uint8_t> checkGreaterOneLuminosityColors{
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
const std::vector<uint8_t> checkLessNegativeOneLuminosityColors{
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
const std::vector<uint8_t> checkAverageColors{
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
const std::vector<uint8_t> checkLightnessColors{
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
const std::vector<uint8_t> checkRChannelColors{
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
const std::vector<uint8_t> checkGChannelColors{
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
const std::vector<uint8_t> checkBChannelColors{
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
const std::vector<std::vector<uint8_t>> algorithmMap{checkDefaultLuminosityColors,
                                                     checkCustomLuminosityColors,
                                                     checkLessZeroLuminosityColors,
                                                     checkGreaterOneLuminosityColors,
                                                     checkLessNegativeOneLuminosityColors,
                                                     checkAverageColors,
                                                     checkLightnessColors,
                                                     checkRChannelColors,
                                                     checkGChannelColors,
                                                     checkBChannelColors};

//------------------------------------------------------------------------------
void SetDataArrayTestValues(UInt8Array* aa)
{
  aa->fill(0);
  size_t index = 0;
  for(const auto& eachColor : testColors)
  {
    aa->getDataStore()->setTuple(index++, eachColor);
  }
}

//------------------------------------------------------------------------------
static VertexGeom* createVertexGeometry(DataStructure& dataStructure, const std::vector<size_t>& tDims, const std::vector<size_t>& cDims)
{
  VertexGeom* vertexGeo = VertexGeom::Create(dataStructure, "VertexGeom");
  auto* vertexArray = Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Vertices", tDims, cDims, vertexGeo->getId());
  vertexGeo->setVertices(*vertexArray);
  auto& vertices = vertexArray->getDataStoreRef();
  for(usize i = 0; i < numTuples; ++i)
  {
    vertices[i * 3 + 0] = i;
    vertices[i * 3 + 1] = i;
    vertices[i * 3 + 2] = i;
  }
  return vertexGeo;
}

//------------------------------------------------------------------------------
void CompareResults(const uint8& algoMapIndex, const DataStructure& dataStructure)
{
  DataPath arrayName({m_GeomName});
  arrayName = arrayName.createChildPath(m_outputArrayPrefix + m_DataArrayName);
  auto* testArray = dataStructure.getDataAs<UInt8Array>(arrayName);
  REQUIRE(testArray != nullptr);
  auto& testData = testArray->getDataStoreRef();

  std::vector<uint8> colorArray{algorithmMap[algoMapIndex]};

  for(size_t index = 0; index < colorArray.size(); ++index)
  {
    uint8 testVal = testData[index];
    uint8 exemplarVal = colorArray[index];
    REQUIRE(testVal == exemplarVal);
  }
}

//------------------------------------------------------------------------------
void RunTest(const uint8& algoMapIndex, const ConvertColorToGrayScale::ConversionType& algorithm, const std::vector<float32>& colorWeights, const int32& colorChannel, bool shouldFail)
{
  DataStructure dataStruct;
  const std::vector<size_t> tDims{numTuples};
  const std::vector<size_t> cDims{3, 1, 1};

  // NOTE: This filter has no geometry requirements
  VertexGeom* vertexGeo = createVertexGeometry(dataStruct, tDims, cDims);
  auto* testAA = UInt8Array::CreateWithStore<DataStore<uint8>>(dataStruct, m_DataArrayName, tDims, cDims, vertexGeo->getId());
  SetDataArrayTestValues(testAA);

  std::vector<DataPath> daps = {DataPath({m_GeomName, m_DataArrayName})};

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ConvertColorToGrayScaleFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ConversionAlgorithm_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(algorithm)));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(colorWeights));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_ColorChannel_Key, std::make_any<int32>(colorChannel));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_InputDataArrayVector_Key, std::make_any<MultiArraySelectionParameter::ValueType>(daps));
  args.insertOrAssign(ConvertColorToGrayScaleFilter::k_OutputArrayPrefix_Key, std::make_any<StringParameter::ValueType>(m_outputArrayPrefix));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStruct, args);
  if(shouldFail)
  {
    COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  else
  {
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStruct, args);
  if(shouldFail)
  {
    COMPLEX_RESULT_REQUIRE_INVALID(executeResult.result);
  }
  else
  {
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);
  }
  if(shouldFail)
  {
    return;
  }

  CompareResults(algoMapIndex, dataStruct);
}

TEST_CASE("ComplexCore::ConvertColorToGrayScale: Valid Execution", "[ComplexCore][ConvertColorToGrayScaleFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Luminosity Algorithm testing
  // Test defaults
  std::cout << "Testing luminosity algorithm..." << std::endl;
  std::cout << "Default weights (0.2125, 0.7154, 0.0721)...";
  RunTest(0, ConvertColorToGrayScale::ConversionType::Luminosity, {0.2125f, 0.7154f, 0.0721f}, 0, false);

  // Test custom
  std::vector<float32> colorWeights{0.75, 0.75, 0.75};
  std::cout << "Custom weights (0.75, 0.75, 0.75)..." << std::endl;
  RunTest(1, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test <0
  colorWeights = {-0.75, -0.75, -0.75};
  std::cout << "Testing weights < 0 (-0.75, -0.75, -0.75)..." << std::endl;
  RunTest(2, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test >1
  colorWeights = {1.75, 1.75, 1.75};
  std::cout << "Testing weights > 1 (1.75, 1.75, 1.75)..." << std::endl;
  RunTest(3, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Test <-1
  colorWeights = {-1.75, -1.75, -1.75};
  std::cout << "Testing weights < -1 (-1.75, -1.75, -1.75)..." << std::endl;
  RunTest(4, ConvertColorToGrayScale::ConversionType::Luminosity, colorWeights, 0, true);

  // Average Algorithm testing
  std::cout << "Testing average algorithm..." << std::endl;
  RunTest(5, ConvertColorToGrayScale::ConversionType::Average, {0.2125f, 0.7154f, 0.0721f}, 0, false);

  // Lightness Algorithm testing
  std::cout << "Testing lightness algorithm..." << std::endl;
  RunTest(6, ConvertColorToGrayScale::ConversionType::Lightness, {0.2125f, 0.7154f, 0.0721f}, 0, false);

  // Single Channel Algorithm testing
  // Red channel
  std::cout << "Testing red channel algorithm..." << std::endl;
  RunTest(7, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 0, false);

  // Green channel
  std::cout << "Testing green channel algorithm..." << std::endl;
  RunTest(8, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 1, false);

  // Blue channel
  std::cout << "Testing blue channel algorithm..." << std::endl;
  RunTest(9, ConvertColorToGrayScale::ConversionType::SingleChannel, {0.2125f, 0.7154f, 0.0721f}, 2, false);
}
