#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
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
COMPLEX_EXPORT std::string DelimiterToString(uint64 delim);

/**
 * @brief [BINARY CAPABLE, unless neighborlist][Multiple File Output] | Writes out to multiple files | !!!!endianess must be addressed in calling class!!!!
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The complex datastructure where *objectPaths* datacontainers are stored
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
COMPLEX_EXPORT void PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                                 const std::atomic_bool& shouldCancel, std::string fileExtension = ".txt", bool exportToBinary = false, const std::string& delimiter = "",
                                                 bool includeIndex = false, bool includeHeaders = false, size_t componentsPerLine = 0);

/**
 * @brief [Single Output][Custom OStream] | Writes one IArray child to some OStream
 * @param outputStrm The already opened output string to write to
 * @param objectPath The datapath for respective dataObject to be written out
 * @param dataStructure The complex datastructure where *objectPath* datacontainer is stored
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param includeHeaders The boolean that determines if headers are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
COMPLEX_EXPORT void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                          const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                          size_t componentsPerLine = 0);

/**
 * @brief [Single Output][Custom OStream] | writes out multiple arrays to ostream
 * @param outputStrm The already opened output string to write to
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The complex datastructure where *objectPaths* datacontainers are stored
 * @param mesgHandler The handler to send progress updates to
 * @param shouldCancel The atomic boolean that determines cancel
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param includeHeaders The boolean that determines if headers are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 * @param neighborLists The list of dataPaths of neighborlists to include
 */
COMPLEX_EXPORT void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                              const std::atomic_bool& shouldCancel, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false,
                                              size_t componentsPerLine = 0, const std::vector<DataPath>& neighborLists = {});
} // namespace OStreamUtilities

} // namespace complex
