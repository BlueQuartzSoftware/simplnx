#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace fs = std::filesystem;

namespace nx::core
{
/**
 * @class ReadCSVFile

 */
class SIMPLNXCORE_EXPORT ReadCSVFile
{
public:
  enum class IssueCodes
  {
    EMPTY_FILE = -100,
    EMPTY_NEW_DG = -102,
    EMPTY_EXISTING_DG = -103,
    DUPLICATE_NAMES = -105,
    ILLEGAL_NAMES = -107,
    FILE_NOT_OPEN = -108,
    INCORRECT_DATATYPE_COUNT = -109,
    INCORRECT_MASK_COUNT = -110,
    INCORRECT_TUPLES = -113,
    NEW_DG_EXISTS = -114,
    CANNOT_SKIP_TO_LINE = -115,
    EMPTY_NAMES = -116,
    HEADER_LINE_OUT_OF_RANGE = -120,
    START_IMPORT_ROW_OUT_OF_RANGE = -121,
    EMPTY_HEADERS = -122,
    IGNORED_TUPLE_DIMS = -200
  };

  ReadCSVFile();
  ~ReadCSVFile() noexcept;

  ReadCSVFile(const ReadCSVFile&) = delete;
  ReadCSVFile(ReadCSVFile&&) noexcept = delete;
  ReadCSVFile& operator=(const ReadCSVFile&) = delete;
  ReadCSVFile& operator=(ReadCSVFile&&) noexcept = delete;

  Result<> readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, const std::vector<std::string>& columnHeaders,
                    const std::vector<DataType>& columnDataTypes, const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const std::vector<usize>& tupleDims,
                    const std::vector<char>& delimiters, bool consecutiveDelimiters, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler);

  Result<> readFile(DataStructure& dataStructure, const std::string& inputFilePath, usize importStartingRow, usize headersLineNumber, const std::vector<DataType>& columnDataTypes,
                    const std::vector<bool>& columnsSkipped, const DataPath& groupPath, const std::vector<usize>& tupleDims, const std::vector<char>& delimiters, bool consecutiveDelimiters,
                    const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& msgHandler);
};
} // namespace nx::core
