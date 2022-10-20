#pragma once

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace fs = std::filesystem;

namespace complex
{
namespace OStreamUtilities
{
/**
 * @brief [BINARY CAPABLE, unless neighborlist][Multiple File Output] | writes out to multiple files | !!!!endianess must be addressed in calling class!!!!
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The complex datastructure where *objectPaths* datacontainers are stored
 * @param directoryPath The path to directory to write files to | used to create file paths for ofstream
 * //params with defaults
 * @param fileExtension The extension to create and write to files with
 * @param exportToBinary The boolean that determines if it writes out binary
 * @param delimiter The delimiter to be inserted into string | leave blank if binary is end output
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed | leave blank if binary is end output
 * @param includeHeaders The boolean that determines if headers are printed | leave blank if binary is end output
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
COMPLEX_EXPORT void PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, fs::directory_entry& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                                 std::string fileExtension = ".txt", bool exportToBinary = false, const std::string& delimiter = "", bool includeIndex = false,
                                                 bool includeHeaders = false, size_t componentsPerLine = 0, bool includeNeighborLists = false);

/**
 * @brief [BINARY CAPABLE, unless neighborlist][Single Output][Custom OStream] | writes one IArray child to some ostream | !!!!endianess must be addressed in calling class!!!!
 * @param outputStrm The already opened output string to write to | binary only supported with ofstream and ostringstream
 * @param objectPath The datapath for respective dataObject to be written out
 * @param dataStructure The complex datastructure where *objectPath* datacontainer is stored
 * //params with defaults
 * @param exportToBinary The boolean that determines if it writes out binary
 * @param delimiter The delimiter to be inserted into string | leave blank if binary is end output
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
COMPLEX_EXPORT void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, bool exportToBinary = false,
                                          const std::string& delimiter = "", size_t componentsPerLine = 0);

/**
 * @brief [Single Output][Custom OStream] | writes out multiple arrays to ostream
 * @param outputStrm The already opened output string to write to
 * @param objectPaths The vector of datapaths for respective dataObjects to be written out
 * @param dataStructure The complex datastructure where *objectPaths* datacontainers are stored
 * //params with defaults
 * @param delimiter The delimiter to be inserted into string
 * @param includeIndex The boolean that determines if "Feature_IDs" are printed
 * @param componentsPerLine The amount of elements to be inserted before newline character
 * @param includeHeaders The boolean that determines if headers are printed
 * @param IncludeNeighborLists The boolean that determines if NeighborLists are printed at the bottom of file
 */
COMPLEX_EXPORT void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                              const std::string& delimiter = "", bool includeIndex = false, size_t componentsPerLine = 0, bool includeHeaders = false,
                                              bool includeNeighborLists = false);
} // namespace OStreamUtilities

} // namespace complex
