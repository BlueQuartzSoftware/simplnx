#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT DynamicTableData
{
public:
  using HeadersListType = std::vector<std::string>;
  using DataType = float64;
  using TableDataType = std::vector<std::vector<DataType>>;

  DynamicTableData();
  DynamicTableData(int32 numRows, int32 numCols);
  DynamicTableData(int32 numRows, int32 numCols, const HeadersListType& rowHeaders, const HeadersListType& colHeaders);
  DynamicTableData(const TableDataType& data, const HeadersListType& rowHeaders, const HeadersListType& colHeaders);
  virtual ~DynamicTableData();

  static DynamicTableData Create(const TableDataType& data, const HeadersListType& rowHeaders, const HeadersListType& colHeaders);

  void setColumnHeaders(const HeadersListType& columnHeaders);
  void setRowHeaders(const HeadersListType& rowHeaders);

  HeadersListType getColumnHeaders() const;
  HeadersListType getRowHeaders() const;

  void setDynamicRows(bool value);
  bool getDynamicRows() const;

  void setDynamicCols(bool value);
  bool getDynamicCols() const;

  void setMinRows(int32 minRows);
  void setMinCols(int32 minCols);

  int32 getMinRows() const;
  int32 getMinCols() const;

  void setDefaultRowCount(int32 value);
  void setDefaultColCount(int32 value);

  int32 getDefaultRowCount() const;
  int32 getDefaultColCount() const;

  static TableDataType DeserializeData(const std::string& dataStr, int32 numRows, int32 numCols, char delimiter);
  static TableDataType ExpandData(const std::vector<DataType>& orig, usize numRows, usize numCols);

  static std::vector<std::string> DeserializeHeaders(const std::string& headersString, char delimiter);

  std::string serializeData(char delimiter) const;
  std::string serializeRowHeaders(char delimiter) const;
  std::string serializeColumnHeaders(char delimiter) const;

  std::vector<DataType> getFlattenedData() const;

  void writeJson(nlohmann::json& json) const;
  bool readJson(const nlohmann::json& json);

  TableDataType getTableData() const;
  void setTableData(const TableDataType& data);

  int32 getNumberOfRows() const;
  int32 getNumberOfColumns() const;

  bool isEmpty() const;

  DynamicTableData(const DynamicTableData& rhs);
  DynamicTableData& operator=(const DynamicTableData& rhs);
  bool operator==(const DynamicTableData& rhs) const;
  bool operator!=(const DynamicTableData& rhs) const;

private:
  HeadersListType m_ColHeaders = {};
  HeadersListType m_RowHeaders = {};
  bool m_DynamicRows = {};
  bool m_DynamicCols = {};
  int32 m_MinRows = 0;
  int32 m_MinCols = 0;
  int32 m_DefaultRowCount = 0;
  int32 m_DefaultColCount = 0;

  TableDataType m_TableData;

  /**
   * @brief Writes the contents of the data to a json object.
   * @param data
   */
  void writeData(nlohmann::json& object) const;

  /**
   * @brief Reads the contents of the json object into a 2D array.
   * @param object
   * @return TableDataType
   */
  TableDataType readData(const nlohmann::json& object);

  /**
   * @brief Checks that the dimensions between all variables are the same.
   * If not, adjusts dimensions to match numRows and numCols.
   */
  void checkAndAdjustDimensions();
};
} // namespace complex
