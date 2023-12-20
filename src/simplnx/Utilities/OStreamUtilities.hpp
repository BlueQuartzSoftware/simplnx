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

namespace nx::core
{
namespace OStreamUtilities
{
/**
 * @brief enum for accepted output delimiters in DREAM3D
 */
enum class Delimiter : uint64
{
  Space = 0,
  Semicolon = 1,
  Comma = 2,
  Colon = 3,
  Tab = 4
}; // Don't reorder

/**
 * @brief turns the enum in this API to respective character as a string
 * @param delim the underlying value of the enum type
 */
SIMPLNX_EXPORT std::string DelimiterToString(uint64 delim);

/**
 * @brief [BINARY CAPABLE, unless neighborlist][Multiple File Output] | Writes out to multiple files | !!!!endianess must be addressed in calling class!!!!
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The simplnx datastructure where *objectPaths* datacontainers are stored
 * @param directoryPath The path to the directory to write files to | used to create outputStrm paths for ofstream
 * @param mesgHandler The handler to send progress updates to
 * @param shouldCancel The atomic boolean that determines cancel
 * //params with defaults
 * @param fileExtension The extension to create and write to files with
 * @param exportToBinary The boolean that determines if it writes out binary
 * @param delimiter The delimiter to be inserted into string | leave blank if binary is end output
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed | leave blank if binary is end output
 * @param includeHeaders The boolean that determines if headers are printed | leave blank if binary is end output
 * @param componentsPerLine The amount of elements to be inserted before newline character | leave blank if binary is end output
 */
SIMPLNX_EXPORT void PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                                 const std::atomic_bool& shouldCancel, const std::string& fileExtension = ".txt", bool exportToBinary = false, const std::string& delimiter = "",
                                                 bool includeIndex = false, bool includeHeaders = false, size_t componentsPerLine = 0);

/**
 * @brief [Single Output][Custom OStream] | Writes one IArray child to some OStream
 * @param outputStrm The already opened output string to write to
 * @param objectPath The datapath for respective dataObject to be written out
 * @param dataStructure The simplnx datastructure where *objectPath* datacontainer is stored
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param includeHeaders The boolean that determines if headers are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
SIMPLNX_EXPORT void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                          const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                          size_t componentsPerLine = 0);

/**
 * @brief [Single Output][Custom OStream] | writes out multiple arrays to ostream
 * @param outputStrm The already opened output string to write to
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The simplnx datastructure where *objectPaths* datacontainers are stored
 * @param mesgHandler The handler to send progress updates to
 * @param shouldCancel The atomic boolean that determines cancel
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param writeFirstIndex Write the first tuple of the arrays. For Feature based data, this would be FeatureId = 0.
 * @paragraph indexName The name of the "Index" column
 * @param includeHeaders The boolean that determines if headers are printed
 * @param neighborLists The list of dataPaths of neighborlists to include
 * @param writeNumOfFeatures The amount of elements per tuple printed at top
 */
SIMPLNX_EXPORT void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                              const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                              bool writeFirstIndex = true, const std::string& indexName = "Index", const std::vector<DataPath>& neighborLists = {}, bool writeNumOfFeatures = false);
} // namespace OStreamUtilities

} // namespace nx::core
