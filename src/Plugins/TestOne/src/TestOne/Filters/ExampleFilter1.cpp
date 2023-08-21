#include "ExampleFilter1.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/ArrayThresholdsParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/NumericTypeParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <string>

using namespace std::string_literals;

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
constexpr StringLiteral k_Param4 = "param4";
constexpr StringLiteral k_Param5 = "param5";
constexpr StringLiteral k_Param6 = "param6";
constexpr StringLiteral k_Param7 = "param7";
constexpr StringLiteral k_Param8 = "param8";
constexpr StringLiteral k_Param9 = "param9";
constexpr StringLiteral k_Param10 = "param10";
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ExampleFilter1::name() const
{
  return FilterTraits<ExampleFilter1>::name;
}

//------------------------------------------------------------------------------
std::string ExampleFilter1::className() const
{
  return FilterTraits<ExampleFilter1>::className;
}

//------------------------------------------------------------------------------
Uuid ExampleFilter1::uuid() const
{
  return FilterTraits<ExampleFilter1>::uuid;
}

//------------------------------------------------------------------------------
std::string ExampleFilter1::humanName() const
{
  return "Example Filter 1";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExampleFilter1::defaultTags() const
{
  return {className(), "Example", "Test"};
}

//------------------------------------------------------------------------------
Parameters ExampleFilter1::parameters() const
{
  Parameters params;
  params.insertSeparator({"FileSystem Selections"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputDir_Key, "Input Directory", "Example input directory help text", "Data", FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputDir));
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "Example input file help text", "/opt/local/bin/ninja", FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile, true));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputDir_Key, "Ouptut Directory", "Example output directory help text", "Output Data", FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputDir));
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output File", "Example output file help text", "", FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::OutputFile));

  params.insertSeparator({"Linked Parameter"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param2, "BoolParameter", "The 2nd parameter", true));
  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Float32Parameter", "The 1st parameter", 0.1234f));
  params.linkParameters(k_Param2, k_Param1, true);
  params.insert(std::make_unique<Int32Parameter>(k_Param3, "Int32Parameter", "The 1st parameter", 0));

  params.insertSeparator({"Vector Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>("Vec2_Key", "Vect<int,2>", "Example int32 vector help text", std::vector<int32_t>{10, 20}, std::vector<std::string>{"X"s, "Y"s}));
  //  params.insert(std::make_unique<VectorInt32Parameter>("Vec3_Key", "3D Dimensions", "", std::vector<int32_t>{-19, -100, 456}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(std::make_unique<VectorUInt8Parameter>("Vec4_Key", "RGBA", "Example uint8 vector help text", std::vector<uint8_t>{0, 255, 128, 255}, std::vector<std::string>{"R"s, "G"s, "B"s, "A"s}));
  //  params.insert(std::make_unique<VectorFloat32Parameter>("Vec4F_Key", "Quaternion", "", std::vector<float>{0.0F, 84.98F, 234.12F, 985.98F}, std::vector<std::string>{"U"s, "V"s, "W"s, "X"s}));
  //  params.insert(std::make_unique<VectorFloat32Parameter>("Vec6F_Key", "Tensor?", "", std::vector<float>(6), std::vector<std::string>{"U"s, "V"s, "W"s, "X"s, "B"s, "A"s}));

  params.insertSeparator({"Other Parameters"});
  params.insert(std::make_unique<StringParameter>(k_Param5, "StringParameter", "Example string help text", "test string"));
  params.insert(std::make_unique<NumericTypeParameter>(k_Param6, "Numeric Type", "Example numeric type help text", NumericType::int32));

  params.insertSeparator({"Big Parameters"});
  params.insert(std::make_unique<GeneratedFileListParameter>(k_Param4, "Input File List", "Data needed to generate the input file list", GeneratedFileListParameter::ValueType{}));

  params.insert(std::make_unique<ArrayThresholdsParameter>(k_Param7, "Data Thresholds", "DataArray thresholds to mask", ArrayThresholdSet{}));

  // param10 should be active if either param8 OR param9 are the correct value
  params.insertSeparator({"Multiple Linked Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param8, "Bool Parameter", "", true));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Param9, "Choices Parameter", "", 0, ChoicesParameter::Choices{"0", "1", "2"}));
  params.insert(std::make_unique<Int32Parameter>(k_Param10, "Int32 Parameter", "", 42));
  params.linkParameters(k_Param8, k_Param10, true);
  params.linkParameters(k_Param9, k_Param10, std::make_any<ChoicesParameter::ValueType>(1));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExampleFilter1::clone() const
{
  return std::make_unique<ExampleFilter1>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExampleFilter1::preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  //  auto inputDir = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputDir_Key);
  //  std::cout << "[ExampleFilter1::PreflightImpl] inputDir=" << inputDir << std::endl;
  //  auto inputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  //  std::cout << "[ExampleFilter1::PreflightImpl] inputFile=" << inputFile << std::endl;

  //  auto outputDir = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputDir_Key);
  //  std::cout << "[ExampleFilter1::PreflightImpl] outputDir=" << outputDir << std::endl;
  //  auto outputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  //  std::cout << "[ExampleFilter1::PreflightImpl] outputFile=" << outputFile << std::endl;

  //  auto vec2 = filterArgs.value<VectorInt32Parameter::ValueType>("Vec2_Key");
  //  auto vec3 = filterArgs.value<VectorInt32Parameter::ValueType>("Vec3_Key");
  //  auto vec4 = filterArgs.value<VectorUInt8Parameter::ValueType>("Vec4_Key");
  //  auto vec4f = filterArgs.value<VectorFloat32Parameter::ValueType>("Vec4F_Key");
  //  auto vec6 = filterArgs.value<VectorFloat32Parameter::ValueType>("Vec6F_Key");

#if 0
  for(const auto& value : vec2)
  {
    std::cout << value << ", ";
  }
  std::cout << std::endl;

  for(const auto& value : vec3)
  {
    std::cout << value << ", ";
  }
  std::cout << std::endl;

  for(const auto& value : vec4)
  {
    std::cout << value << ", ";
  }
  std::cout << std::endl;

  for(const auto& value : vec4f)
  {
    std::cout << value << ", ";
  }
  std::cout << std::endl;

  for(const auto& value : vec6)
  {
    std::cout << value << ", ";
  }
  std::cout << std::endl;
#endif
  return {};
}

//------------------------------------------------------------------------------
Result<> ExampleFilter1::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return MakeWarningVoidResult(-100, "Example Warning from within an execute message");
}
} // namespace complex
