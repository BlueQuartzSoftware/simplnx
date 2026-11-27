#include "ReadTextDataArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
/**
 * @struct CSVReadFileFunctor
 * @brief Dispatches the destination type to the paged text parser.
 */
struct CSVReadFileFunctor
{
  /**
   * @brief Parses typed text values into the destination store.
   * @tparam T Specifies the destination numeric type.
   * @param inputIDataArray Receives parsed values.
   * @param inputFilePath Identifies the input text file.
   * @param skipLines Specifies leading lines to ignore.
   * @param delimiter Separates values in each data line.
   * @return Parser or checked destination-write error, or success.
   *
   * CsvParser uses bounded pages. This call does not inspect the algorithm cancellation flag.
   */
  template <typename T>
  Result<> operator()(IDataArray* inputIDataArray, const fs::path& inputFilePath, uint64 skipLines, char delimiter)
  {
    auto& store = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    return csv_parser::ReadFile<T>(inputFilePath, store, skipLines, delimiter);
  }
};
} // namespace

ReadTextDataArray::ReadTextDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadTextDataArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> ReadTextDataArray::operator()()
{
  const auto& inputFilePath = m_InputValues->input_file;
  auto skipLines = m_InputValues->skip_line_count;
  auto choiceIndex = m_InputValues->delimiter_index;
  const auto& path = m_InputValues->output_data_array_path;

  char delimiter = nx::core::csv_parser::IndexToDelimiter(choiceIndex);

  auto* iDataArrayPtr = m_DataStructure.getDataAs<IDataArray>(path);
  return ExecuteDataFunction(CSVReadFileFunctor{}, iDataArrayPtr->getDataType(), iDataArrayPtr, inputFilePath, skipLines, delimiter);
}
