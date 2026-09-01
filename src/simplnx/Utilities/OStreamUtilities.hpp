#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core::OStreamUtilities
{
/**
 * @enum Delimiter
 * @brief Selects one supported text delimiter.
 *
 * The numeric values index the delimiter lookup table. Keep their order stable.
 */
enum class Delimiter : uint64
{
  Space = 0,     ///< Selects one space.
  Semicolon = 1, ///< Selects a semicolon.
  Comma = 2,     ///< Selects a comma.
  Colon = 3,     ///< Selects a colon.
  Tab = 4        ///< Selects one tab.
};

/**
 * @brief Converts a delimiter value to text.
 * @param delim Specifies a Delimiter underlying value.
 * @return Selected delimiter string.
 * @pre delim is not greater than the Tab underlying value.
 */
SIMPLNX_EXPORT std::string DelimiterToString(uint64 delim);

/**
 * @brief Writes each selected data object to its own text or binary file.
 * @param objectPaths Identifies numeric arrays, string arrays, or neighbor lists.
 * @param dataStructure Supplies the selected data objects.
 * @param directoryPath Identifies an existing output directory.
 * @param mesgHandler Receives progress messages.
 * @param shouldCancel Supplies the cancellation flag.
 * @param fileExtension Specifies the suffix for each generated file.
 * @param exportToBinary True to write numeric arrays as raw binary values.
 * @param delimiter Specifies text output separation.
 * @param includeIndex True to include neighbor-list indices.
 * @param includeHeaders True to include neighbor-list headers.
 * @param componentsPerLine Specifies numeric-array tuples per text line. Zero selects one.
 * @param swapEndian True to byte-swap temporary numeric pages for binary output.
 * @return First numeric storage, binary stream, atomic-file, or commit error.
 * @throws std::runtime_error If directoryPath is not a directory or binary output selects a neighbor list.
 *
 * Each output uses AtomicFile and commits only after its stream closes. Numeric
 * arrays use bounded pages, and byte swapping never modifies the source. Cancellation
 * returns a valid result without committing the current temporary file.
 * @pre componentsPerLine fits int32.
 *
 * Text stream failures are not reported. Binary stream failures return an error.
 */
SIMPLNX_EXPORT Result<> PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& directoryPath,
                                                     const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const std::string& fileExtension = ".txt",
                                                     bool exportToBinary = false, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                                     size_t componentsPerLine = 0, bool swapEndian = false);

/**
 * @brief Writes one supported data object to a caller-owned text stream.
 * @param outputStrm Receives output and must already be open.
 * @param objectPath Identifies a numeric array, string array, or neighbor list.
 * @param dataStructure Supplies the selected object.
 * @param mesgHandler Receives progress messages.
 * @param shouldCancel Supplies the cancellation flag.
 * @param delimiter Specifies value separation.
 * @param includeIndex True to include neighbor-list indices.
 * @param includeHeaders True to include neighbor-list headers.
 * @param componentsPerLine Specifies numeric-array tuples per line. Zero selects one.
 *
 * This void API does not report store or stream failures. Cancellation can leave
 * partial output in outputStrm.
 * @pre componentsPerLine fits int32.
 */
SIMPLNX_EXPORT void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                          const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                          size_t componentsPerLine = 0);

/**
 * @brief Interleaves selected arrays into one caller-owned text stream.
 * @param outputStrm Receives output and must already be open.
 * @param objectPaths Identifies numeric and string arrays to interleave.
 * @param dataStructure Supplies all selected objects.
 * @param mesgHandler Receives progress messages.
 * @param shouldCancel Supplies the cancellation flag.
 * @param delimiter Specifies column separation.
 * @param includeIndex True to write a tuple-index column.
 * @param includeHeaders True to write array and component column names.
 * @param writeFirstIndex True to include tuple zero.
 * @param indexName Specifies the tuple-index header.
 * @param neighborLists Identifies neighbor lists to append after the table.
 * @param writeNumOfFeatures True to write the exported tuple count first.
 * @pre objectPaths is nonempty. All selected arrays have at least the first array's tuple count.
 * @pre If tuple zero is omitted and writeNumOfFeatures is true, the first array is nonempty.
 *
 * Numeric writers use bounded forward caches. A numeric store-read failure throws
 * std::runtime_error. This void API does not report stream failures. Cancellation
 * can leave partial output in outputStrm.
 */
SIMPLNX_EXPORT void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                              const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                              bool writeFirstIndex = true, const std::string& indexName = "Index", const std::vector<DataPath>& neighborLists = {}, bool writeNumOfFeatures = false);
} // namespace nx::core::OStreamUtilities
