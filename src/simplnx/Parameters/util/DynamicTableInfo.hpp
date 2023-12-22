#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace nx::core
{
/**
 * @brief DynamicTableInfo contains the description of a 2D table which may
 * have dynamic or static rows and columns.
 */
class SIMPLNX_EXPORT DynamicTableInfo
{
public:
  using HeadersListType = std::vector<std::string>;
  using ValueType = float64;
  using RowType = std::vector<ValueType>;
  using TableDataType = std::vector<RowType>;

  /**
   * @brief TableDims is a simple struct to hold named
   * values for the number of rows and columns
   */
  struct SIMPLNX_EXPORT TableDims
  {
    usize numRows = 0;
    usize numCols = 0;
  };

  /**
   * @brief DynamicVectorInfo contains the description of
   * a dynamically sized row or column.
   */
  class SIMPLNX_EXPORT DynamicVectorInfo
  {
  public:
    /**
     * @brief Default constructor.
     */
    DynamicVectorInfo();

    /**
     * @brief Contructor a DynamicVectorInfo where default size must be greater than min size.
     * @param minSize
     * @param defaultSize
     * @param headerTemplate
     */
    DynamicVectorInfo(usize minSize, usize defaultSize, std::string headerTemplate);

    /**
     * @brief Contructs a DynamicVectorInfo with min size equal to default size.
     * @param minSize
     * @param headerTemplate
     */
    DynamicVectorInfo(usize minSize, std::string headerTemplate);

    /**
     * @brief Returns true if num is greater than min size.
     * @param num
     * @return
     */
    bool validate(usize num) const;

    /**
     * @brief Returns the min size.
     * @return
     */
    usize getMinSize() const;

    /**
     * @brief Returns the default size.
     * @return
     */
    usize getDefaultSize() const;

    /**
     * @brief Returns the header template. Uses fmtlib formatting rules.
     * @return
     */
    const std::string& getHeaderTemplate() const;

  private:
    usize m_MinSize = 0;
    usize m_DefaultSize = 0;
    std::string m_HeaderTemplate = "{}";
  };

  /**
   * @brief StaticVectorInfo contains the description of
   * a statically sized row or column.
   */
  class SIMPLNX_EXPORT StaticVectorInfo
  {
  public:
    /**
     * @brief Default constructor.
     */
    StaticVectorInfo();

    /**
     * @brief Constructs a StaticVectorInfo with headers set to the index number.
     * @param size
     */
    StaticVectorInfo(usize size);

    /**
     * @brief Constructs a StaticVectorInfo with the number of elements equal
     * to the number of headers.
     * @param headers
     */
    StaticVectorInfo(HeadersListType headers);

    /**
     * @brief Returns the number of elements.
     * @return
     */
    usize getSize() const;

    /**
     * @brief Returns the list of headers.
     * @return
     */
    const HeadersListType& getHeaders() const;

    /**
     * @brief Returns true if num is equal to size.
     * @param num
     * @return
     */
    bool validate(usize num) const;

    /**
     * @brief Returns the default size. Same as size.
     * @return
     */
    usize getDefaultSize() const;

    /**
     * @brief Returns the min size. Same as size.
     * @return
     */
    usize getMinSize() const;

  private:
    usize m_Size = 0;
    HeadersListType m_Headers = {};
  };

  /**
   * @brief VectorInfo contains the description of
   * a dynamic or static row or column.
   */
  class SIMPLNX_EXPORT VectorInfo
  {
  public:
    /**
     * @brief Deleted default constructor.
     */
    VectorInfo() = delete;

    /**
     * @brief Constructs a VectorInfo from a static vector info.
     * @param vectorInfo
     */
    VectorInfo(StaticVectorInfo vectorInfo);

    /**
     * @brief Constructs a VectorInfo from a dynamic vector info.
     * @param vectorInfo
     */
    VectorInfo(DynamicVectorInfo vectorInfo);

    /**
     * @brief Returns true if the given size is valid for the
     * underlying vector info.
     * @param num
     * @return
     */
    bool validate(usize num) const;

    /**
     * @brief Returns true if this contains a DynamicVectorInfo.
     * @return
     */
    bool isDynamic() const;

    /**
     * @brief Returns the underlying StaticVectorInfo/DynamicVectorInfo.
     * @tparam T
     * @return
     */
    template <class T>
    const T& get() const
    {
      static_assert(std::is_same_v<T, StaticVectorInfo> || std::is_same_v<T, DynamicVectorInfo>);
      return std::get<T>(m_VectorInfo);
    }

    /**
     * @brief Sets the underlying info to a StaticVectorInfo.
     * @param info
     */
    void set(StaticVectorInfo info);

    /**
     * @brief Sets the underlying info to a DynamicVectorInfo.
     * @param info
     */
    void set(DynamicVectorInfo info);

    /**
     * @brief Returns the default size.
     * @return
     */
    usize getDefaultSize() const;

    /**
     * @brief Returns the min size.
     * @return
     */
    usize getMinSize() const;

  private:
    std::variant<StaticVectorInfo, DynamicVectorInfo> m_VectorInfo;
  };

  /**
   * @brief Default constructor with rows/columns set to static
   * vectors of size 0.
   */
  DynamicTableInfo();

  /**
   * @brief Contructs a DynamicTableInfo with row and column info.
   * @param rowInfo
   * @param colInfo
   */
  DynamicTableInfo(VectorInfo rowsInfo, VectorInfo colsInfo);

  /**
   * @brief Returns the rows info.
   * @return
   */
  const VectorInfo& getRowsInfo() const;

  /**
   * @brief Returns the cols info.
   * @return
   */
  const VectorInfo& getColsInfo() const;

  /**
   * @brief Sets the rows info.
   * @param info
   */
  void setRowsInfo(VectorInfo info);

  /**
   * @brief Sets the cols info.
   * @param info
   */
  void setColsInfo(VectorInfo info);

  /**
   * @brief Validates the given 2D table fits the description
   * from the rows and cols info.
   * @param table
   * @return
   */
  Result<> validate(const TableDataType& table) const;

  /**
   * @brief Creates a 2D table based on the default size.
   * @return
   */
  TableDataType createDefault() const;

  /**
   * @brief Returns the table dims from a vector of vectors. If the given table,
   * is not valid (i.e. M x N), returns nullopt optional.
   * @param table
   * @return
   */
  static std::optional<TableDims> GetTableDims(const TableDataType& table);

  /**
   * @brief Writes a table to json in row major order.
   * @param table
   * @return
   */
  static nlohmann::json WriteData(const TableDataType& table);

  /**
   * @brief Reads a table from json.
   * @param json
   * @return
   */
  static Result<TableDataType> ReadData(const nlohmann::json& json);

  /**
   * @brief Flattens a 2D table to a 1D vector in row major order.
   * @param table
   * @return
   */
  static std::vector<ValueType> FlattenData(const TableDataType& table);

private:
  VectorInfo m_RowsInfo;
  VectorInfo m_ColsInfo;
};
} // namespace nx::core
