#include "DynamicTableInfo.hpp"

#include <fmt/core.h>

using namespace nx::core;

DynamicTableInfo::DynamicVectorInfo::DynamicVectorInfo() = default;

DynamicTableInfo::DynamicVectorInfo::DynamicVectorInfo(usize minSize, usize defaultSize, std::string headerTemplate)
: m_MinSize(minSize)
, m_DefaultSize(defaultSize)
, m_HeaderTemplate(std::move(headerTemplate))
{
  if(m_DefaultSize < m_MinSize)
  {
    throw std::runtime_error("DynamicTableInfo::DynamicVectorInfo: Invalid default size");
  }
}

DynamicTableInfo::DynamicVectorInfo::DynamicVectorInfo(usize minSize, std::string headerTemplate)
: DynamicVectorInfo(minSize, minSize, std::move(headerTemplate))
{
}

bool DynamicTableInfo::DynamicVectorInfo::validate(usize num) const
{
  return num >= m_MinSize;
}

usize DynamicTableInfo::DynamicVectorInfo::getMinSize() const
{
  return m_MinSize;
}

usize DynamicTableInfo::DynamicVectorInfo::getDefaultSize() const
{
  return m_DefaultSize;
}

const std::string& DynamicTableInfo::DynamicVectorInfo::getHeaderTemplate() const
{
  return m_HeaderTemplate;
}

DynamicTableInfo::StaticVectorInfo::StaticVectorInfo() = default;

DynamicTableInfo::StaticVectorInfo::StaticVectorInfo(usize size)
: m_Size(size)
{
  for(usize i = 0; i < size; i++)
  {
    m_Headers.push_back(std::to_string(i));
  }
}

DynamicTableInfo::StaticVectorInfo::StaticVectorInfo(HeadersListType headers)
: m_Headers(std::move(headers))
{
  m_Size = m_Headers.size();
}

usize DynamicTableInfo::StaticVectorInfo::getSize() const
{
  return m_Size;
}

const DynamicTableInfo::HeadersListType& DynamicTableInfo::StaticVectorInfo::getHeaders() const
{
  return m_Headers;
}

bool DynamicTableInfo::StaticVectorInfo::validate(usize num) const
{
  return num == m_Size;
}

usize DynamicTableInfo::StaticVectorInfo::getDefaultSize() const
{
  return m_Size;
}

usize DynamicTableInfo::StaticVectorInfo::getMinSize() const
{
  return m_Size;
}

DynamicTableInfo::VectorInfo::VectorInfo(StaticVectorInfo vectorInfo)
: m_VectorInfo(std::move(vectorInfo))
{
}

DynamicTableInfo::VectorInfo::VectorInfo(DynamicVectorInfo vectorInfo)
: m_VectorInfo(std::move(vectorInfo))
{
}

bool DynamicTableInfo::VectorInfo::validate(usize num) const
{
  return std::visit([num](auto&& info) { return info.validate(num); }, m_VectorInfo);
}

bool DynamicTableInfo::VectorInfo::isDynamic() const
{
  return std::holds_alternative<DynamicVectorInfo>(m_VectorInfo);
}

void DynamicTableInfo::VectorInfo::set(StaticVectorInfo info)
{
  m_VectorInfo = std::move(info);
}

void DynamicTableInfo::VectorInfo::set(DynamicVectorInfo info)
{
  m_VectorInfo = std::move(info);
}

usize DynamicTableInfo::VectorInfo::getDefaultSize() const
{
  return std::visit([](auto&& info) { return info.getDefaultSize(); }, m_VectorInfo);
}

usize DynamicTableInfo::VectorInfo::getMinSize() const
{
  return std::visit([](auto&& info) { return info.getMinSize(); }, m_VectorInfo);
}

DynamicTableInfo::DynamicTableInfo()
: m_RowsInfo(StaticVectorInfo(0))
, m_ColsInfo(StaticVectorInfo(0))
{
}

DynamicTableInfo::DynamicTableInfo(VectorInfo rowsInfo, VectorInfo colsInfo)
: m_RowsInfo(std::move(rowsInfo))
, m_ColsInfo(std::move(colsInfo))
{
}

const DynamicTableInfo::VectorInfo& DynamicTableInfo::getRowsInfo() const
{
  return m_RowsInfo;
}

const DynamicTableInfo::VectorInfo& DynamicTableInfo::getColsInfo() const
{
  return m_ColsInfo;
}

void DynamicTableInfo::setRowsInfo(VectorInfo info)
{
  m_RowsInfo = std::move(info);
}

void DynamicTableInfo::setColsInfo(VectorInfo info)
{
  m_ColsInfo = std::move(info);
}

std::optional<DynamicTableInfo::TableDims> DynamicTableInfo::GetTableDims(const TableDataType& table)
{
  usize numRows = table.size();
  if(numRows == 0)
  {
    return {{0, 0}};
  }
  usize lastNumCols = table.front().size();
  for(usize i = 1; i < numRows; i++)
  {
    const auto& row = table[i];
    usize numCols = row.size();
    if(numCols != lastNumCols)
    {
      return {};
    }
  }
  return {{numRows, lastNumCols}};
}

Result<> DynamicTableInfo::validate(const TableDataType& table) const
{
  auto dims = GetTableDims(table);
  if(!dims.has_value())
  {
    return MakeErrorResult(-1, "Data does not have the same row lengths");
  }

  if(!m_RowsInfo.validate(dims->numRows))
  {
    return MakeErrorResult(-2, "Invalid number of rows");
  }

  if(!m_ColsInfo.validate(dims->numCols))
  {
    return MakeErrorResult(-3, "Invalid number of columns");
  }

  return {};
}

DynamicTableInfo::TableDataType DynamicTableInfo::createDefault() const
{
  usize numRows = m_RowsInfo.getDefaultSize();
  usize numCols = m_ColsInfo.getDefaultSize();
  return TableDataType(numRows, RowType(numCols));
}

nlohmann::json DynamicTableInfo::WriteData(const TableDataType& table)
{
  nlohmann::json rows;
  for(const auto& vector : table)
  {
    nlohmann::json cols;
    for(auto value : vector)
    {
      cols.push_back(value);
    }
    rows.push_back(cols);
  }
  return rows;
}

Result<DynamicTableInfo::TableDataType> DynamicTableInfo::ReadData(const nlohmann::json& json)
{
  if(!json.is_array())
  {
    return MakeErrorResult<TableDataType>(-1, "Json is not an array");
  }

  usize numRows = json.size();

  TableDataType table;
  table.reserve(numRows);

  for(usize i = 0; i < json.size(); i++)
  {
    const auto& rowJson = json[i];
    if(!rowJson.is_array())
    {
      return MakeErrorResult<TableDataType>(-2, fmt::format("Row {} json is not an array", i));
    }

    usize numCols = rowJson.size();
    RowType row;
    row.reserve(numCols);

    for(usize j = 0; j < rowJson.size(); j++)
    {
      const auto& value = rowJson[j];
      if(!value.is_number())
      {
        return MakeErrorResult<TableDataType>(-3, fmt::format("Element ({}, {}) is not a number", i, j));
      }

      row.push_back(value.get<ValueType>());
    }

    table.push_back(std::move(row));
  }

  return {std::move(table)};
}

std::vector<DynamicTableInfo::ValueType> DynamicTableInfo::FlattenData(const TableDataType& table)
{
  std::vector<ValueType> flattenedData;

  for(const auto& row : table)
  {
    for(auto value : row)
    {
      flattenedData.push_back(value);
    }
  }

  return flattenedData;
}
