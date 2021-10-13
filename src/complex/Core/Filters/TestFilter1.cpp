#include "TestFilter1.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Core/Parameters/BoolParameter.hpp"
#include "complex/Core/Parameters/ChoicesParameter.hpp"
#include "complex/Core/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Core/Parameters/NumberParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";

constexpr StringLiteral k_Param4 = "Parent_Bool";
constexpr StringLiteral k_Param5 = "Child_Param";

constexpr StringLiteral k_Param6 = "Parent_Choice";
constexpr StringLiteral k_Param7 = "Child_Choice_1";
constexpr StringLiteral k_Param8 = "Child_Choice_2";
constexpr StringLiteral k_Param9 = "Child_Choice_3";

} // namespace

namespace complex
{
std::string TestFilter1::name() const
{
  return FilterTraits<TestFilter1>::name;
}

Uuid TestFilter1::uuid() const
{
  return FilterTraits<TestFilter1>::uuid;
}

std::string TestFilter1::humanName() const
{
  return "Test Filter 1";
}

Parameters TestFilter1::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<Float32Parameter>(k_Param1, "Parameter 1", "The 1st parameter", 0.1234f));
  params.insert(std::make_unique<BoolParameter>(k_Param2, "Parameter 2", "The 2nd parameter", false));
  params.insert(std::make_unique<GeneratedFileListParameter>(k_Param3, "Input File List", "Data needed to generate the input file list", GeneratedFileListParameter::ValueType{}));

  params.insertSeparator({"Linked Boolean Parameter Example"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param4, "Some Boolean", "Parent Boolean", false));
  params.insert(std::make_unique<StringParameter>(k_Param5, "Linked String Parameter", "Test parameter", "default"));
  params.linkParameters(k_Param4, k_Param5, true);

  params.insertSeparator({"Linked Choices Parameter Example"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Param6, "Linked Choices Parameter", "The 3rd parameter", 0, ChoicesParameter::Choices{"Vert", "Image", "Triangle"}));
  params.insert(std::make_unique<Float32Parameter>(k_Param7, "Parameter 6", "The 1st parameter", 11.983));
  params.insert(std::make_unique<Float32Parameter>(k_Param8, "Parameter 7", "The 1st parameter", 22.324));
  params.insert(std::make_unique<Float32Parameter>(k_Param9, "Parameter 8", "The 1st parameter", 77.729));
  params.linkParameters(k_Param6, k_Param7, 0);
  params.linkParameters(k_Param6, k_Param8, 1);
  params.linkParameters(k_Param6, k_Param9, 2);

  return params;
}

IFilter::UniquePointer TestFilter1::clone() const
{
  return std::make_unique<TestFilter1>();
}

Result<OutputActions> TestFilter1::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}

Result<> TestFilter1::executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
