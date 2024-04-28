#include "DynamicTableExampleFilter.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
constexpr StringLiteral k_Param4 = "param4";
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string DynamicTableExampleFilter::name() const
{
  return FilterTraits<DynamicTableExampleFilter>::name;
}

//------------------------------------------------------------------------------
std::string DynamicTableExampleFilter::className() const
{
  return FilterTraits<DynamicTableExampleFilter>::className;
}

//------------------------------------------------------------------------------
Uuid DynamicTableExampleFilter::uuid() const
{
  return FilterTraits<DynamicTableExampleFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string DynamicTableExampleFilter::humanName() const
{
  return "Dynamic Table Examples";
}

//------------------------------------------------------------------------------
std::vector<std::string> DynamicTableExampleFilter::defaultTags() const
{
  return {className(), "Example", "Test"};
}

//------------------------------------------------------------------------------
Parameters DynamicTableExampleFilter::parameters() const
{
  Parameters params;
  params.insertSeparator({"Fixed Columns - Fixed Rows"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Col 1", "Col 2"}));
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Row 1", "Row 2"}));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param1, "Fixed Columns - Fixed Rows", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }
  params.insertSeparator({"Fixed Column - Dynamic Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Col 1", "Col 2"}));
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Row {}"));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param2, "Fixed Columns - Dynamic Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }

  params.insertSeparator({"Dynamic Column - Fixed Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Col {}"));
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Row 1", "Row 2"}));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param3, "Dynamic Columns - Fixed Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }

  params.insertSeparator({"Dynamic Column - Dynamic Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Col {}"));
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Row {}"));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param4, "Dynamic Columns - Dynamic Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer DynamicTableExampleFilter::clone() const
{
  return std::make_unique<DynamicTableExampleFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult DynamicTableExampleFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}

//------------------------------------------------------------------------------
Result<> DynamicTableExampleFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineFilter, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace nx::core
