#include "ExampleFilter1.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include <string>

using namespace std::string_literals;

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
} // namespace

namespace complex
{
std::string ExampleFilter1::name() const
{
  return FilterTraits<ExampleFilter1>::name;
}

std::string ExampleFilter1::className() const
{
  return FilterTraits<ExampleFilter1>::className;
}

Uuid ExampleFilter1::uuid() const
{
  return FilterTraits<ExampleFilter1>::uuid;
}

std::string ExampleFilter1::humanName() const
{
  return "Example Filter 1";
}

Parameters ExampleFilter1::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0.1234f));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param2, "Use Value", "The 2nd parameter", true));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_Param3, "Parameter 3", "MultiDataArraySelection Parameter", MultiArraySelectionParameter::ValueType{}));

  params.insert(std::make_unique<VectorInt32Parameter>("Vec2_Key", "2D Dimensions", "", std::vector<int32_t>{10, 20}, std::vector<std::string>{"X"s, "Y"s}));
  params.insert(std::make_unique<VectorInt32Parameter>("Vec3_Key", "3D Dimensions", "", std::vector<int32_t>{-19, -100, 456}, std::vector<std::string>{"X"s, "Y"s, "Z"s}));
  params.insert(std::make_unique<VectorUInt8Parameter>("Vec4_Key", "RGBA", "", std::vector<uint8_t>{0, 255, 128, 255}, std::vector<std::string>{"R"s, "G"s, "B"s, "A"s}));
  params.insert(std::make_unique<VectorFloat32Parameter>("Vec4F_Key", "Quaternion", "", std::vector<float>{0.0F, 84.98F, 234.12F, 985.98F}, std::vector<std::string>{"U"s, "V"s, "W"s, "X"s}));
  params.insert(std::make_unique<VectorFloat32Parameter>("Vec6F_Key", "Tensor?", "", std::vector<float>(6), std::vector<std::string>{"U"s, "V"s, "W"s, "X"s, "B"s, "A"s}));

  params.linkParameters(k_Param2, k_Param1, true);
  return params;
}

IFilter::UniquePointer ExampleFilter1::clone() const
{
  return std::make_unique<ExampleFilter1>();
}

IFilter::PreflightResult ExampleFilter1::preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto vec2 = filterArgs.value<VectorInt32Parameter::ValueType>("Vec2_Key");
  auto vec3 = filterArgs.value<VectorInt32Parameter::ValueType>("Vec3_Key");
  auto vec4 = filterArgs.value<VectorUInt8Parameter::ValueType>("Vec4_Key");
  auto vec4f = filterArgs.value<VectorFloat32Parameter::ValueType>("Vec4F_Key");
  auto vec6 = filterArgs.value<VectorFloat32Parameter::ValueType>("Vec6F_Key");

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

Result<> ExampleFilter1::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
