#include "ExampleFilter2Filter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include <any>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3_index";
constexpr StringLiteral k_Param4 = "param4";
constexpr StringLiteral k_Param5 = "param5_path";
constexpr StringLiteral k_Param6 = "param6_path";
constexpr StringLiteral k_Param7 = "param7";
constexpr StringLiteral k_Param8 = "param8_path";
constexpr StringLiteral k_Param9 = "param9_path";
constexpr StringLiteral k_Param10 = "param10_path";
constexpr StringLiteral k_Param11 = "param11_path";
constexpr StringLiteral k_Param12 = "param12s";
constexpr StringLiteral k_Param13 = "param13";

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExampleFilter2Filter::name() const
{
  return FilterTraits<ExampleFilter2Filter>::name;
}

//------------------------------------------------------------------------------
std::string ExampleFilter2Filter::className() const
{
  return FilterTraits<ExampleFilter2Filter>::className;
}

//------------------------------------------------------------------------------
Uuid ExampleFilter2Filter::uuid() const
{
  return FilterTraits<ExampleFilter2Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExampleFilter2Filter::humanName() const
{
  return "Example Filter 2";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExampleFilter2Filter::defaultTags() const
{
  return {className(), "Example", "Test"};
}

//------------------------------------------------------------------------------
Parameters ExampleFilter2Filter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"1rst Group of Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Param7, "Bool Parameter", "Example bool help text", true));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_Param3, "Choices Parameter", "Example choices help text", 0, ChoicesParameter::Choices{"foo", "bar", "baz"}));

  params.insertSeparator(Parameters::Separator{"2nd Group of Parameters"});
  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Col {}"));
  tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(0, "Row {}"));
  DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
  params.insert(std::make_unique<DynamicTableParameter>(k_Param13, "Dynamic Table Parameter", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));

  // These should show up under the "Required Objects" Section in the GUI
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_Param9, "DataGroup Selection Parameter", "Example data group selection help text", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::DataGroup}));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_Param10, "DataPath Selection Parameter", "Example data path selection help text", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_Param6, "Array Selection", "Example array selection help text", ArraySelectionParameter::ValueType{}, nx::core::GetAllDataTypes()));
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_Param11, "Geometry Selection Parameter", "Example geometry selection help text", DataPath{}, GeometrySelectionParameter::AllowedTypes{}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_Param12, "MultiArray Selection Parameter", "Example multiarray selection help text", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::Any}, nx::core::GetAllDataTypes()));

  params.linkParameters(k_Param7, k_Param9, std::make_any<BoolParameter::ValueType>(true));

  params.linkParameters(k_Param3, k_Param10, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_Param3, k_Param6, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_Param3, k_Param11, std::make_any<ChoicesParameter::ValueType>(2));

  // These should show up under the "Created Objects" section in the GUI
  params.insert(std::make_unique<DataGroupCreationParameter>(k_Param8, "DataGroup Creation Parameter", "Example data group creation help text", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_Param5, "Array Creation", "Example array creation help text", ArrayCreationParameter::ValueType{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExampleFilter2Filter::clone() const
{
  return std::make_unique<ExampleFilter2Filter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExampleFilter2Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
Result<> ExampleFilter2Filter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace nx::core
