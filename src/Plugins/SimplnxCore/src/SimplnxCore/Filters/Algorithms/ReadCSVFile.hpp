#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/util/ReadCSVData.hpp"

namespace fs = std::filesystem;

namespace nx::core
{
/**
 * @class ReadCSVFile
 * @brief Parses delimited columns into preallocated DataArrays.
 *
 * Column parsers buffer destination writes. Cancellation and parse errors flush
 * completed buffered values before the method returns.
 */
class SIMPLNXCORE_EXPORT ReadCSVFile
{
public:
  /**
   * @enum IssueCodes
   * @brief Defines CSV configuration and input errors.
   */
  enum class IssueCodes
  {
    EMPTY_FILE = -100,                    ///< Reports an input with no data.
    EMPTY_NEW_DG = -102,                  ///< Reports an empty new group path.
    EMPTY_EXISTING_DG = -103,             ///< Reports an empty existing group path.
    DUPLICATE_NAMES = -105,               ///< Reports repeated output column names.
    ILLEGAL_NAMES = -107,                 ///< Reports output names with invalid characters.
    FILE_NOT_OPEN = -108,                 ///< Reports failure to open the input file.
    INCORRECT_DATATYPE_COUNT = -109,      ///< Reports a type count that differs from column count.
    INCORRECT_MASK_COUNT = -110,          ///< Reports a skip-mask count that differs from column count.
    INCORRECT_TUPLES = -113,              ///< Reports a row count that differs from destination tuples.
    NEW_DG_EXISTS = -114,                 ///< Reports a new group path that already exists.
    CANNOT_SKIP_TO_LINE = -115,           ///< Reports failure to reach the first import row.
    EMPTY_NAMES = -116,                   ///< Reports an empty output column name.
    HEADER_LINE_OUT_OF_RANGE = -120,      ///< Reports a header row outside the file.
    START_IMPORT_ROW_OUT_OF_RANGE = -121, ///< Reports an import row outside the file.
    EMPTY_HEADERS = -122,                 ///< Reports an empty header set.
    IGNORED_TUPLE_DIMS = -200             ///< Reports tuple dimensions ignored by an existing group.
  };

  /**
   * @brief Creates a stateless CSV reader.
   */
  ReadCSVFile();
  /**
   * @brief Destroys the stateless CSV reader.
   */
  ~ReadCSVFile() noexcept;

  ReadCSVFile(const ReadCSVFile&) = delete;
  ReadCSVFile(ReadCSVFile&&) noexcept = delete;
  ReadCSVFile& operator=(const ReadCSVFile&) = delete;
  ReadCSVFile& operator=(ReadCSVFile&&) noexcept = delete;

  /**
   * @brief Parses rows using caller-supplied column headers.
   * @param dataStructure Receives parsed columns.
   * @param inputFilePath Identifies the input file.
   * @param importStartingRow Specifies the first data row.
   * @param columnHeaders Specifies output names.
   * @param columnDataTypes Specifies one parser type per column.
   * @param columnsSkipped Selects columns to ignore.
   * @param groupPath Identifies the destination group.
   * @param tupleDims Specifies destination tuple dimensions.
   * @param delimiters Specifies accepted field separators.
   * @param consecutiveDelimiters Treats adjacent delimiters as one separator when true.
   * @param shouldCancel Stops before later rows when true.
   * @param msgHandler Receives five-percent progress updates.
   * @return Open, seek, parse, flush, or destination-write error, or success after cancellation.
   */
  Result<> readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, const std::vector<std::string>& columnHeaders, const std::vector<CSVType>& columnDataTypes,
                    const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler);

  /**
   * @brief Reads column headers from one file row, then parses data rows.
   * @param dataStructure Receives parsed columns.
   * @param inputFilePath Identifies the input file.
   * @param importStartingRow Specifies the first data row.
   * @param headersLineNumber Specifies the header row.
   * @param columnDataTypes Specifies one parser type per column.
   * @param columnsSkipped Selects columns to ignore.
   * @param groupPath Identifies the destination group.
   * @param tupleDims Specifies destination tuple dimensions.
   * @param delimiters Specifies accepted field separators.
   * @param consecutiveDelimiters Treats adjacent delimiters as one separator when true.
   * @param shouldCancel Stops before later rows when true.
   * @param msgHandler Receives five-percent progress updates.
   * @return Header, open, seek, parse, flush, or destination-write error, or success after cancellation.
   */
  Result<> readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, usize headersLineNumber, const std::vector<CSVType>& columnDataTypes,
                    const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const ShapeType& tupleDims, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler);
};
} // namespace nx::core
