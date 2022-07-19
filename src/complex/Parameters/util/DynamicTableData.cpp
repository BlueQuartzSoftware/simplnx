#include "DynamicTableData.hpp"

#include <cstdlib>
#include <sstream>

using namespace complex;

DynamicTableData::DynamicTableData()

{
}

DynamicTableData::DynamicTableData(int32 nRows, int32 nCols)

{
  TableDataType data(nRows, std::vector<double>(nCols, 0));
  m_TableData = data;

  HeadersListType rHeaders, cHeaders;

  for(int i = 0; i < nRows; i++)
  {
    rHeaders.push_back(std::to_string(i));
  }

  for(int i = 0; i < nCols; i++)
  {
    cHeaders.push_back(std::to_string(i));
  }

  m_RowHeaders = rHeaders;
  m_ColHeaders = cHeaders;
}

DynamicTableData::DynamicTableData(int32 nRows, int32 nCols, const HeadersListType& rHeaders, const HeadersListType& cHeaders)

{
  TableDataType data(nRows, std::vector<double>(nCols, 0));
  m_TableData = data;

  m_RowHeaders = rHeaders;
  m_ColHeaders = cHeaders;

  // Adjust dimensions if they are not all the same
  checkAndAdjustDimensions();
}

DynamicTableData::DynamicTableData(const TableDataType& data, const HeadersListType& rHeaders, const HeadersListType& cHeaders)

{
  m_TableData = data;
  m_RowHeaders = rHeaders;
  m_ColHeaders = cHeaders;

  // Adjust dimensions if they are not all the same
  checkAndAdjustDimensions();
}

DynamicTableData DynamicTableData::Create(const TableDataType& dims, const HeadersListType& rHeaders, const HeadersListType& cHeaders)
{
  HeadersListType rowHdrs;
  for(const auto& rowHdr : rHeaders)
  {
    rowHdrs.push_back(rowHdr);
  }
  HeadersListType colHdrs;
  for(const auto& colHdr : cHeaders)
  {
    colHdrs.push_back(colHdr);
  }
  return DynamicTableData(dims, rowHdrs, colHdrs);
}

DynamicTableData::~DynamicTableData() = default;

bool DynamicTableData::isEmpty() const
{
  return !(!m_TableData.empty() || !m_RowHeaders.empty() || m_ColHeaders.empty());
}

void DynamicTableData::checkAndAdjustDimensions()
{
  using SizeType = std::pair<usize, usize>;
  SizeType dataSize = {m_TableData.size(), 0};
  SizeType headerSize = {m_RowHeaders.size(), m_ColHeaders.size()};

  if(!m_TableData.empty())
  {
    dataSize.second = m_TableData[0].size();
  }

  if(dataSize == headerSize)
  {
    return;
  }

  /* The header dimensions do not equal the data dimensions.
     The data dimensions will be used and will overwrite the current header dimensions.
     This may result in data loss.
  */
  int nRows = dataSize.second;
  int nCols = dataSize.first;

  // If row header dimension is greater than default row dimension, remove the extra headers
  if(m_RowHeaders.size() > nRows)
  {
    while(m_RowHeaders.size() > nRows)
    {
      m_RowHeaders.pop_back();
    }
  }
  // If column header dimension is greater than default column dimension, remove the extra headers
  if(m_ColHeaders.size() > nCols)
  {
    while(m_ColHeaders.size() > nCols)
    {
      m_ColHeaders.pop_back();
    }
  }
}

std::string DynamicTableData::serializeData(char delimiter) const
{
  std::stringstream ss;

  for(const auto& row : m_TableData)
  {
    for(int col = 0; col < row.size(); col++)
    {
      ss << row[col] << delimiter;
    }
  }
  std::string str = ss.str();
  str.pop_back(); // Get rid of the last, unnecessary delimiter

  return str;
}

DynamicTableData::TableDataType DynamicTableData::DeserializeData(const std::string& dataStr, int32 nRows, int32 nCols, char delimiter)
{
  TableDataType data(nRows, std::vector<double>(nCols));
  int row = 0, col = 0;

  if(dataStr.empty())
  {
    return data;
  }

  int start = 0;
  int tokenIndex = 0;

  while(tokenIndex >= 0)
  {
    tokenIndex = dataStr.find(delimiter, start);
    std::string valueStr = dataStr.substr(start, tokenIndex - start);
    double value = std::stod(valueStr.c_str());
    data[row][col] = value;
    start = tokenIndex + 1;

    col++;
    if(col == nCols)
    {
      row++;
      col = 0;
    }
  }

  return data;
}

std::string DynamicTableData::serializeRowHeaders(char delimiter) const
{
  std::stringstream ss;

  for(const auto& m_RowHeader : m_RowHeaders)
  {
    ss << m_RowHeader;
    ss << delimiter;
  }
  std::string str = ss.str();
  str.pop_back(); // Get rid of the last, unnecessary delimiter

  return str;
}

std::string DynamicTableData::serializeColumnHeaders(char delimiter) const
{
  std::stringstream ss;

  for(const auto& m_ColHeader : m_ColHeaders)
  {
    ss << m_ColHeader;
    ss << delimiter;
  }
  std::string str = ss.str();
  str.pop_back(); // Get rid of the last, unnecessary delimiter

  return str;
}

DynamicTableData::HeadersListType DynamicTableData::DeserializeHeaders(const std::string& headersStr, char delimiter)
{
  HeadersListType headers;

  if(headersStr.empty())
  {
    return headers;
  }

  int start = 0;
  int tokenIndex = 0;

  while(tokenIndex >= 0)
  {
    tokenIndex = headersStr.find(delimiter, start);
    std::string header = headersStr.substr(start, tokenIndex);
    headers.push_back(header);
    start = tokenIndex + 1;
  }

  return headers;
}

std::vector<DynamicTableData::DataType> DynamicTableData::getFlattenedData() const
{
  int numRows = getNumberOfRows();
  int numCols = getNumberOfColumns();

  std::vector<DataType> flat(numRows * numCols);
  for(int row = 0; row < numRows; row++)
  {
    for(int col = 0; col < numCols; col++)
    {
      flat[row * numCols + col] = m_TableData[row][col];
    }
  }

  return flat;
}

DynamicTableData::TableDataType DynamicTableData::ExpandData(const std::vector<DataType>& orig, usize nRows, usize nCols)
{
  TableDataType expand(nRows, std::vector<DataType>(nCols));

  if(orig.size() != nRows * nCols)
  {
    // Something went wrong
    return expand;
  }

  for(usize row = 0; row < nRows; row++)
  {
    for(usize col = 0; col < nCols; col++)
    {
      expand[row][col] = orig[row * nCols + col];
    }
  }

  return expand;
}

void DynamicTableData::writeJson(nlohmann::json& json) const
{
  writeData(json);

  nlohmann::json rHeaders;
  for(const auto& header : m_RowHeaders)
  {
    rHeaders.push_back(header);
  }
  json["Row Headers"] = rHeaders;

  nlohmann::json cHeaders;
  for(const auto& header : m_ColHeaders)
  {
    cHeaders.push_back(header);
  }
  json["Column Headers"] = cHeaders;

  json["HasDynamicRows"] = m_DynamicRows;
  json["HasDynamicCols"] = m_DynamicCols;
  json["MinRowCount"] = m_MinRows;
  json["MinColCount"] = m_MinCols;
  json["DefaultRowCount"] = m_DefaultRowCount;
  json["DefaultColCount"] = m_DefaultColCount;
}

bool DynamicTableData::readJson(const nlohmann::json& json)
{
  if(json.contains("Dynamic Table Data"))
  {
    nlohmann::json obj = json["Dynamic Table Data"].object();
    m_TableData = readData(obj);
  }
  else
  {
    m_TableData = readData(json);
  }

  nlohmann::json rHeaders = json["Row Headers"];
  for(const auto& val : rHeaders)
  {
    if(val.is_string())
    {
      m_RowHeaders.push_back(val);
    }
  }

  nlohmann::json cHeaders = json["Column Headers"];
  for(const auto& val : cHeaders)
  {
    if(val.is_string())
    {
      m_ColHeaders.push_back(val);
    }
  }

  m_DynamicRows = json["HasDynamicRows"];
  m_DynamicCols = json["HasDynamicCols"];
  m_MinRows = json["MinRowCount"];
  m_MinCols = json["MinColCount"];
  m_DefaultRowCount = json["DefaultRowCount"];
  m_DefaultColCount = json["DefaultColCount"];

  return true;
}

void DynamicTableData::writeData(nlohmann::json& object) const
{
  nlohmann::json rows;
  for(const std::vector<DataType>& vector : m_TableData)
  {
    nlohmann::json cols;
    for(const auto& val : vector)
    {
      cols.push_back(val);
    }
    rows.push_back(cols);
  }

  object["Table Data"] = rows;
}

DynamicTableData::TableDataType DynamicTableData::readData(const nlohmann::json& object)
{
  TableDataType data;
  if(object["Table Data"].is_array())
  {
    nlohmann::json rowArray = object["Table Data"];
    data.resize(rowArray.size());

    for(int row = 0; row < rowArray.size(); row++)
    {
      nlohmann::json rowObj = rowArray.at(row);
      if(rowObj.is_array())
      {
        nlohmann::json colArray = rowObj;
        data[row].resize(colArray.size());

        for(int col = 0; col < colArray.size(); col++)
        {
          nlohmann::json colObj = colArray.at(col);

          if(colObj.is_number_float())
          {
            data[row][col] = colObj;
          }
        }
      }
    }
    return data;
  }
  return TableDataType();
}

DynamicTableData::TableDataType DynamicTableData::getTableData() const
{
  return m_TableData;
}

void DynamicTableData::setTableData(const TableDataType& data)
{
  m_TableData = data;

  // Adjust dimensions
  checkAndAdjustDimensions();
}

int DynamicTableData::getNumberOfRows() const
{
  return static_cast<int>(m_TableData.size());
}

int DynamicTableData::getNumberOfColumns() const
{
  if(!m_TableData.empty())
  {
    return static_cast<int>(m_TableData[0].size());
  }
  return 0;
}

DynamicTableData::DynamicTableData(const DynamicTableData& rhs)
{
  m_TableData = rhs.m_TableData;
  m_RowHeaders = rhs.m_RowHeaders;
  m_ColHeaders = rhs.m_ColHeaders;
  m_DynamicRows = rhs.m_DynamicRows;
  m_DynamicCols = rhs.m_DynamicCols;
  m_MinRows = rhs.m_MinRows;
  m_MinCols = rhs.m_MinCols;
  m_DefaultRowCount = rhs.m_DefaultRowCount;
  m_DefaultColCount = rhs.m_DefaultColCount;
}

DynamicTableData& DynamicTableData::operator=(const DynamicTableData& rhs)
{
  m_TableData = rhs.m_TableData;
  m_RowHeaders = rhs.m_RowHeaders;
  m_ColHeaders = rhs.m_ColHeaders;
  m_DynamicRows = rhs.m_DynamicRows;
  m_DynamicCols = rhs.m_DynamicCols;
  m_MinRows = rhs.m_MinRows;
  m_MinCols = rhs.m_MinCols;
  m_DefaultRowCount = rhs.m_DefaultRowCount;
  m_DefaultColCount = rhs.m_DefaultColCount;
  return *this;
}

bool DynamicTableData::operator==(const DynamicTableData& rhs) const
{
  if(m_RowHeaders == rhs.m_RowHeaders && m_ColHeaders == rhs.m_ColHeaders && m_DynamicRows == rhs.m_DynamicRows && m_DynamicCols == rhs.m_DynamicCols && m_MinRows == rhs.m_MinRows &&
     m_MinCols == rhs.m_MinCols && m_DefaultRowCount == rhs.m_DefaultRowCount && m_DefaultColCount == rhs.m_DefaultColCount)
  {
    for(int i = 0; i < m_TableData.size(); i++)
    {
      for(int j = 0; j < m_TableData[i].size(); j++)
      {
        if(m_TableData[i][j] != rhs.m_TableData[i][j])
        {
          return false;
        }
      }
    }
    return true;
  }

  return false;
}

bool DynamicTableData::operator!=(const DynamicTableData& rhs) const
{
  if(m_RowHeaders == rhs.m_RowHeaders && m_ColHeaders == rhs.m_ColHeaders && m_DynamicRows == rhs.m_DynamicRows && m_DynamicCols == rhs.m_DynamicCols && m_MinRows == rhs.m_MinRows &&
     m_MinCols == rhs.m_MinCols && m_DefaultRowCount == rhs.m_DefaultRowCount && m_DefaultColCount == rhs.m_DefaultColCount)
  {
    for(int i = 0; i < m_TableData.size(); i++)
    {
      for(int j = 0; j < m_TableData[i].size(); j++)
      {
        if(m_TableData[i][j] != rhs.m_TableData[i][j])
        {
          return true;
        }
      }
    }
  }
  else
  {
    return true;
  }

  return false;
}

void DynamicTableData::setColumnHeaders(const HeadersListType& value)
{
  m_ColHeaders = value;
}

DynamicTableData::HeadersListType DynamicTableData::getColumnHeaders() const
{
  return m_ColHeaders;
}

void DynamicTableData::setRowHeaders(const HeadersListType& value)
{
  m_RowHeaders = value;
}

DynamicTableData::HeadersListType DynamicTableData::getRowHeaders() const
{
  return m_RowHeaders;
}

void DynamicTableData::setDynamicRows(bool value)
{
  m_DynamicRows = value;
}

bool DynamicTableData::getDynamicRows() const
{
  return m_DynamicRows;
}

void DynamicTableData::setDynamicCols(bool value)
{
  m_DynamicCols = value;
}

bool DynamicTableData::getDynamicCols() const
{
  return m_DynamicCols;
}

void DynamicTableData::setMinRows(int32 value)
{
  m_MinRows = value;
}

int32 DynamicTableData::getMinRows() const
{
  return m_MinRows;
}

void DynamicTableData::setMinCols(int32 value)
{
  m_MinCols = value;
}

int32 DynamicTableData::getMinCols() const
{
  return m_MinCols;
}

void DynamicTableData::setDefaultRowCount(int32 value)
{
  m_DefaultRowCount = value;
}

int32 DynamicTableData::getDefaultRowCount() const
{
  return m_DefaultRowCount;
}

void DynamicTableData::setDefaultColCount(int32 value)
{
  m_DefaultColCount = value;
}

int32 DynamicTableData::getDefaultColCount() const
{
  return m_DefaultColCount;
}
