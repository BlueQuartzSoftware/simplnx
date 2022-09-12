#include "DynamicTableExample.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_Param1 = "param1";
constexpr StringLiteral k_Param2 = "param2";
constexpr StringLiteral k_Param3 = "param3";
constexpr StringLiteral k_Param4 = "param4";
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
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Col 1", "Col 2"}));
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Row 1", "Row 2"}));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param1, "Fixed Columns | Fixed Rows", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }
  params.insertSeparator({"Fixed Column | Dynamic Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"Col 1", "Col 2"}));
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Row {}"));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param2, "Fixed Coumn | Dynamic Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }

  params.insertSeparator({"Dynamic Column | Fixed Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Col {}"));
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"Row 1", "Row 2"}));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param3, "Dynamic Coumn | Fixed Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
  }

  params.insertSeparator({"Dynamic Column | Dynamic Row"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Col {}"));
    tableInfo.setRowsInfo(DynamicTableInfo::DynamicVectorInfo(2, "Row {}"));
    DynamicTableInfo::TableDataType defaultTable{{{10, 20}, {30, 40}}};
    params.insert(std::make_unique<DynamicTableParameter>(k_Param4, "Dynamic Coumn | Dynamic Row", "DynamicTableParameter Example Help Text", defaultTable, tableInfo));
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
