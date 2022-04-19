#include "DynamicTableExample.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

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
constexpr StringLiteral k_Param11 = "param11";
constexpr StringLiteral k_Param12 = "param12";
constexpr StringLiteral k_Param13 = "param13";
constexpr StringLiteral k_Param14 = "param14";
constexpr StringLiteral k_Param15 = "param15";
constexpr StringLiteral k_Param16 = "param16";
} // namespace

namespace complex
{
std::string DynamicTableExample::name() const
{
  return FilterTraits<DynamicTableExample>::name;
}

std::string DynamicTableExample::className() const
{
  return FilterTraits<DynamicTableExample>::className;
}

Uuid DynamicTableExample::uuid() const
{
  return FilterTraits<DynamicTableExample>::uuid;
}

std::string DynamicTableExample::humanName() const
{
  return "Dynamic Table Examples";
}

Parameters DynamicTableExample::parameters() const
{
  Parameters params;
  params.insertSeparator({"Fixed Columns | Fixed Rows"});
  {
    DynamicTableParameter::ValueType dynamicTable{{{10, 20}, {30, 40}}, {"Row 1", "Row2"}, {"Col 1", "Col 2"}};
    dynamicTable.setMinCols(2);
    dynamicTable.setDynamicCols(false);
    dynamicTable.setDynamicRows(false);
    params.insert(std::make_unique<DynamicTableParameter>(k_Param1, "Fixed Columns | Fixed Rows", "DynamicTableParameter Example Help Text", dynamicTable));
  }
  params.insertSeparator({"Fixed Column | Dynamic Row"});
  {
    DynamicTableParameter::ValueType dynamicTable{{{10, 20}, {30, 40}}, {"Row 1", "Row2"}, {"Col 1", "Col 2"}};
    dynamicTable.setMinCols(2);
    dynamicTable.setDynamicCols(false);
    dynamicTable.setDynamicRows(true);
    params.insert(std::make_unique<DynamicTableParameter>(k_Param2, "Fixed Coumn | Dynamic Row", "DynamicTableParameter Example Help Text", dynamicTable));
  }

  params.insertSeparator({"Dynamic Coumn | Fixed Row"});
  {
    DynamicTableParameter::ValueType dynamicTable{{{10, 20}, {30, 40}}, {"Row 1", "Row2"}, {"Col 1", "Col 2"}};
    dynamicTable.setMinCols(2);
    dynamicTable.setDynamicCols(true);
    dynamicTable.setDynamicRows(false);
    params.insert(std::make_unique<DynamicTableParameter>(k_Param3, "Dynamic Coumn | Fixed Row", "DynamicTableParameter Example Help Text", dynamicTable));
  }

  params.insertSeparator({"Dynamic Coumn | Dynamic Row"});
  {
    DynamicTableParameter::ValueType dynamicTable{{{10, 20}, {30, 40}}, {"Row 1", "Row2"}, {"Col 1", "Col 2"}};
    dynamicTable.setMinCols(2);
    dynamicTable.setDynamicCols(true);
    dynamicTable.setDynamicRows(true);
    params.insert(std::make_unique<DynamicTableParameter>(k_Param4, "Dynamic Coumn | Dynamic Row", "DynamicTableParameter Example Help Text", dynamicTable));
  }
  return params;
}

IFilter::UniquePointer DynamicTableExample::clone() const
{
  return std::make_unique<DynamicTableExample>();
}

IFilter::PreflightResult DynamicTableExample::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}

Result<> DynamicTableExample::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
