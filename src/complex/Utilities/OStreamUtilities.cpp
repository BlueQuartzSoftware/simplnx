#include "OStreamUtilities.hpp"
#include "StringUtilities.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using namespace complex;

void OStreamUtilities::OutputFunctions::neighborListImplWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, bool hasIndex, bool hasHeaders)
{
  if(type == DataObject::Type::INeighborList) // unique because its loaded horizontally no dataAlg use
  {
    auto dataType = dataStructure.getDataAs<INeighborList>(path)->getDataType();
    switch(dataType)
    {
    case DataType::int8: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<int8>>(path), hasIndex, hasHeaders).loadToMatrix(0, dataStructure.getDataAs<NeighborList<int8>>(path)->getSize());
      break;
    }
    case DataType::int16: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<int16>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<int16>>(path)->getSize());
      break;
    }
    case DataType::int32: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<int32>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<int32>>(path)->getSize());
      break;
    }
    case DataType::int64: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<int64>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<int64>>(path)->getSize());
      break;
    }
    case DataType::uint8: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<uint8>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<uint8>>(path)->getSize());
      break;
    }
    case DataType::uint16: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<uint16>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<uint16>>(path)->getSize());
      break;
    }
    case DataType::uint32: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<uint32>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<uint32>>(path)->getSize());
      break;
    }
    case DataType::uint64: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<uint64>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<uint64>>(path)->getSize());
      break;
    }
    case DataType::float32: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<float32>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<float32>>(path)->getSize());
      break;
    }
    case DataType::float64: {
      LoadNeighborListToMatrixImpl(matrixRef, dataStructure.getDataRefAs<NeighborList<float64>>(path), hasIndex, hasHeaders)
          .loadToMatrix(0, dataStructure.getDataAs<NeighborList<float64>>(path)->getSize());
      break;
    }
    default: {
      throw std::runtime_error("Cannot boolean in this data structure type");
      break;
    }
    }
  }
  else
  {
    throw std::runtime_error("Function can only process neighborLists");
  }
}

void OStreamUtilities::OutputFunctions::dataAlgParallelWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, const size_t& column,
                                                               bool hasHeaders)
{
  ParallelDataAlgorithm dataAlg;
  if(type == DataObject::Type::StringArray)
  {
    if(hasHeaders)
    {
      matrixRef.getValue(0, column) = dataStructure.getDataAs<StringArray>(path)->getName();
    }
    dataAlg.setRange(0, dataStructure.getDataAs<StringArray>(path)->getSize());
    dataAlg.execute(LoadStringArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<StringArray>(path), column, hasHeaders));
  }
  else if(type == DataObject::Type::DataArray) // Data array of unknown typing
  {
    if(hasHeaders)
    {
      matrixRef.getValue(0, column) = dataStructure.getDataAs<IDataArray>(path)->getName();
    }
    dataAlg.setRange(0, dataStructure.getDataAs<IDataArray>(path)->getSize());
    auto dataType = dataStructure.getDataAs<IDataArray>(path)->getDataType();
    switch(dataType)
    {
    case DataType::int8: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<int8>>(path), column, hasHeaders));
      break;
    }
    case DataType::int16: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<int16>>(path), column, hasHeaders));
      break;
    }
    case DataType::int32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<int32>>(path), column, hasHeaders));
      break;
    }
    case DataType::int64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<int64>>(path), column, hasHeaders));
      break;
    }
    case DataType::uint8: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<uint8>>(path), column, hasHeaders));
      break;
    }
    case DataType::uint16: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<uint16>>(path), column, hasHeaders));
      break;
    }
    case DataType::uint32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<uint32>>(path), column, hasHeaders));
      break;
    }
    case DataType::uint64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<uint64>>(path), column, hasHeaders));
      break;
    }
    case DataType::float32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<float32>>(path), column, hasHeaders));
      break;
    }
    case DataType::float64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dataStructure.getDataRefAs<DataArray<float64>>(path), column, hasHeaders));
      break;
    }
    default: {
      throw std::runtime_error("Cannot process boolean in this data structure type");
      break;
    }
    }
  }
  else
  {
    throw std::runtime_error("Cannot process print data for this type");
  }
}

template <typename T>
size_t getMaxNeighborlistElements(NeighborList<T> list)
{
  size_t max = 0;
  for(size_t i = 0; i < list.getNumberOfLists(); i++)
  {
    auto grain = list.getListReference(i);
    for(size_t index = 0; index < grain.size(); index++)
    {
      if(grain[index] > max)
      {
        max = grain[index];
      }
    }
  }
  return max;
}

std::vector<std::shared_ptr<OStreamUtilities::PrintMatrix2D>> OStreamUtilities::OutputFunctions::createNeighborListPrintStringMatrices(DataStructure& dataStructure, const std::vector<DataPath>& paths,
                                                                                                                                       bool addIndexValue, bool includeHeaders)
{
  std::vector<std::shared_ptr<PrintMatrix2D>> outputPtrs;
  for(const auto& path : paths)
  {
    size_t columns = 0;
    if(addIndexValue)
    {
      columns++;
    }
    size_t rows = 0;
    if(includeHeaders)
    {
      rows++;
    }
    auto list = dataStructure.getDataAs<INeighborList>(path);
    rows += list->getNumberOfTuples();

    auto dataType = dataStructure.getDataAs<INeighborList>(path)->getDataType();
    switch(dataType)
    {
    case DataType::int8: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int8>>(path));
      break;
    }
    case DataType::int16: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int16>>(path));
      break;
    }
    case DataType::int32: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int32>>(path));
      break;
    }
    case DataType::int64: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int64>>(path));
      break;
    }
    case DataType::uint8: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint8>>(path));
      break;
    }
    case DataType::uint16: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint16>>(path));
      break;
    }
    case DataType::uint32: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint32>>(path));
      break;
    }
    case DataType::uint64: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint64>>(path));
      break;
    }
    case DataType::float32: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<float32>>(path));
      break;
    }
    case DataType::float64: {
      columns += getMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<float64>>(path));
      break;
    }
    default: {
      throw std::runtime_error("Cannot process boolean in this data structure type");
      break;
    }
    }

    auto matrixPtr = std::make_shared<PrintMatrix2D>(rows, columns);
    // multi-thread element access
    std::vector<std::string> headers;
    if(addIndexValue)
    {
      headers.emplace_back("Feature_IDs");
      if(includeHeaders)
      {
        headers.emplace_back("Element_Count");
        headers.emplace_back("NeighborList");
        for(size_t i = 0; i < (rows - 1); i++)
        {
          matrixPtr->getValue((i + 1), 0) = StringUtilities::number((i + 1));
        }
      }
      else
      {
        for(size_t i = 0; i < rows; i++)
        {
          matrixPtr->getValue(i, 0) = StringUtilities::number((i + 1));
        }
      }
    }
    else
    {
      if(includeHeaders)
      {
        headers.emplace_back("Element_Count");
        headers.emplace_back("NeighborList");
        for(size_t i = 0; i < (rows - 1); i++)
        {
          matrixPtr->getValue((i + 1), 0) = StringUtilities::number((i + 1));
        }
      }
      else
      {
        for(size_t i = 0; i < rows; i++)
        {
          matrixPtr->getValue(i, 0) = StringUtilities::number((i + 1));
        }
      }
    }
    for(int32 i = 0; i < headers.size(); i++)
    {
      matrixPtr->getValue(i) = headers[i];
    }
    neighborListImplWrapper(dataStructure, *matrixPtr, DataObject::Type::INeighborList, path, addIndexValue, includeHeaders);
    outputPtrs.push_back(matrixPtr);
  }
  return outputPtrs;
}

std::shared_ptr<OStreamUtilities::PrintMatrix2D> OStreamUtilities::OutputFunctions::createSingleTypePrintStringMatrix(DataStructure& dataStructure, const std::vector<DataPath>& paths,
                                                                                                                      const DataObject::Type& typing, bool addIndexValue, bool includeHeaders)
// implicitly runs checkBalance [except on neighborLists]
{
  bool checkBalance = false;
  size_t columns = 0;
  if(addIndexValue)
  {
    columns++;
  }
  size_t rows = 0;
  if(includeHeaders)
  {
    rows++;
  }
  std::vector<size_t> compCounts;

  if(typing == DataObject::Type::StringArray)
  {
    columns += paths.size();
    for(const auto& path : paths)
    {
      compCounts.push_back(dataStructure.getDataAs<StringArray>(path)->getSize());
    }
  }
  else if(typing == DataObject::Type::DataArray)
  {
    columns += paths.size();
    for(const auto& path : paths)
    {
      compCounts.push_back(dataStructure.getDataAs<IDataArray>(path)->getSize());
    }
  }

  /* find rows */
  if(compCounts.size() == 0)
  {
    throw std::runtime_error("Cannot create matrix from empty paths");
  }
  // clear dupes
  std::sort(compCounts.begin(), compCounts.end());
  auto dupes = std::unique(compCounts.begin(), compCounts.end());
  compCounts.erase(dupes, compCounts.end());

  if(compCounts.size() == 1) // is uniform in size
  {
    rows += compCounts[0];
    checkBalance = true;
  }
  else
  {
    rows += *std::max_element(compCounts.begin(), compCounts.end());
  }

  /* work with matrix */
  auto matrixPtr = std::make_shared<PrintMatrix2D>(rows, columns);
  // multi-thread element access
  if(addIndexValue)
  {
    if(includeHeaders)
    {
      matrixPtr->getValue(0) = "Feature_IDs";
      for(size_t i = 0; i < (rows - 1); i++)
      {
        matrixPtr->getValue((i + 1), 0) = StringUtilities::number((i + 1));
      }
    }
    else
    {
      for(size_t i = 0; i < rows; i++)
      {
        matrixPtr->getValue(i, 0) = StringUtilities::number((i + 1));
      }
    }
    for(size_t i = 0; i < paths.size(); i++)
    {
      dataAlgParallelWrapper(dataStructure, *matrixPtr, typing, paths[i], (i + 1), includeHeaders);
    }
  }
  else
  {
    for(size_t i = 0; i < paths.size(); i++)
    {
      dataAlgParallelWrapper(dataStructure, *matrixPtr, typing, paths[i], i, includeHeaders);
    }
  }
  if(checkBalance)
  {
    matrixPtr->checkBalanceType();
  }
  return matrixPtr;
}

std::shared_ptr<OStreamUtilities::PrintMatrix2D>
OStreamUtilities::OutputFunctions::createMultipleTypePrintStringMatrix(DataStructure& dataStructure, std::map<DataPath, DataObject::Type>& printMapping, bool addIndexValue, bool includeHeaders)
{
  bool checkBalance = false;
  size_t rows = 0;
  if(includeHeaders)
  {
    rows++;
  }
  size_t columns = printMapping.size();
  if(addIndexValue)
  {
    columns++;
  }
  std::vector<DataPath> paths;
  std::vector<DataObject::Type> types;
  std::vector<size_t> compCounts;
  for(auto& [key, value] : printMapping)
  {
    switch(value)
    {
    case DataObject::Type::StringArray:
      compCounts.push_back(dataStructure.getDataAs<StringArray>(key)->getSize());
      break;
    case DataObject::Type::DataArray:
      compCounts.push_back(dataStructure.getDataAs<IDataArray>(key)->getSize());
      break;
    default:
      throw std::runtime_error("Cannot process this data structure type");
      break;
    }
    paths.push_back(key);
    types.push_back(value);
  }

  /* find rows */
  if(compCounts.size() == 0)
  {
    throw std::runtime_error("Cannot create matrix from empty paths");
  }
  // clear dupes
  std::sort(compCounts.begin(), compCounts.end());
  auto dupes = std::unique(compCounts.begin(), compCounts.end());
  compCounts.erase(dupes, compCounts.end());

  if(compCounts.size() == 1) // is uniform in size
  {
    rows += compCounts[0];
    checkBalance = true;
  }
  else
  {
    rows += *std::max_element(compCounts.begin(), compCounts.end());
  }

  auto matrixPtr = std::make_shared<PrintMatrix2D>(rows, columns);
  // multi-thread element access
  if(addIndexValue)
  {
    if(includeHeaders)
    {
      matrixPtr->getValue(0) = "Feature_IDs";
      for(size_t i = 0; i < (rows - 1); i++)
      {
        matrixPtr->getValue((i + 1), 0) = StringUtilities::number((i + 1));
      }
    }
    else
    {
      for(size_t i = 0; i < rows; i++)
      {
        matrixPtr->getValue(i, 0) = StringUtilities::number((i + 1));
      }
    }
    for(size_t i = 0; i < printMapping.size(); i++)
    {
      dataAlgParallelWrapper(dataStructure, *matrixPtr, types[i], paths[i], (i + 1), includeHeaders);
    }
  }
  else
  {
    for(size_t i = 0; i < printMapping.size(); i++)
    {
      dataAlgParallelWrapper(dataStructure, *matrixPtr, types[i], paths[i], i, includeHeaders);
    }
  }
  if(checkBalance)
  {
    matrixPtr->checkBalanceType();
  }
  return matrixPtr;
}

template <typename T>
void OStreamUtilities::OutputFunctions::writeOut(const std::string& outStr, T& outputStrm, bool isBinary)
{
  if((isBinary) && ((std::is_same<T, std::ofstream>::value) || (std::is_same<T, std::ostringstream>::value))) // endianess should be considered before this point
  {
    outputStrm.write(outStr.c_str(), outStr.length());
  }
  else
  {
    outputStrm << outStr;
  }
}

template <typename T>
void OStreamUtilities::OutputFunctions::writeOutWrapper(std::map<size_t, std::string>& stringStore, T& outputStrm, bool isBinary)
// requires unique stringStore for each Print2DMatrix to function properly
{
  for(auto& [key, value] : stringStore) // take advantage of maps automatic descending order sort
  {
    writeOut(value, outputStrm, isBinary);
  }
}

void OStreamUtilities::OutputFunctions::multiFileWriteOutWrapper(std::map<std::string, std::map<size_t, std::string>>& printMap, bool isBinary)
// requires unique stringStore for each Print2DMatrix to function properly
{
  std::ofstream outputStrm;
  for(auto& [pathKey, nestedMap] : printMap)
  {
    const auto& path = pathKey;
    if(isBinary)
    {
      outputStrm = std::ofstream(path, std::ios_base::app | std::ios_base::binary);
    }
    else
    {
      outputStrm = std::ofstream(path, std::ios_base::app);
    }
    for(auto& [key, value] : nestedMap) // take advantage of maps automatic descending order sort
    {
      writeOut(value, outputStrm, isBinary);
    }
  }
}

DataObject::Type OStreamUtilities::OutputFunctions::getDataType(const DataPath& objectPath, DataStructure& dataStructure)
{
  return dataStructure.getDataAs<DataObject>(objectPath)->getDataObjectType();
}

std::vector<DataObject::Type> OStreamUtilities::OutputFunctions::getDataTypesWrapper(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure)
{
  std::vector<DataObject::Type> types;
  for(const auto& path : objectPaths)
  {
    types.push_back(std::move(getDataType(path, dataStructure)));
  }
  return types;
}

template <typename T, typename U>
void OStreamUtilities::OutputFunctions::loadMapfromVectors(const std::vector<T>& keys, const std::vector<U>& values, std::map<T, U>& mapToLoad) // vectors must be same size
{
  std::transform(keys.begin(), keys.end(), values.begin(), std::inserter(mapToLoad, mapToLoad.end()), [](T a, U b) { return std::make_pair(a, b); });
}

template <typename T, typename U>
std::map<T, U> OStreamUtilities::OutputFunctions::createHomogeneousMapfromVector(const std::vector<T>& keys, const U& value)
{
  std::map<T, U> homogMap;
  for(const auto& key : keys)
  {
    homogMap.emplace(key, value);
  }
  return homogMap;
}

template <typename T, typename U>
std::map<U, std::vector<T>> OStreamUtilities::OutputFunctions::createSortedMapbyType(const std::vector<T>& uniqueValuesVec, const std::vector<U>& parallelSortingVec)
{
  if(uniqueValuesVec.size() != parallelSortingVec.size())
  {
    throw std::runtime_error("Vectors are not equivalent!");
  }

  auto uniqueTypes = parallelSortingVec;
  std::sort(uniqueTypes.begin(), uniqueTypes.end());
  auto dupes = std::unique(uniqueTypes.begin(), uniqueTypes.end());
  uniqueTypes.erase(dupes, uniqueTypes.end());

  std::map<U, std::vector<T>> sortedMap;

  for(const auto& type : uniqueTypes)
  {
    std::vector<T> uniquesTemp;
    for(size_t i = 0; i < parallelSortingVec.size(); i++)
    {
      if(parallelSortingVec[i] == type)
      {
        uniquesTemp.push_back(uniqueValuesVec[i]);
      }
    }
    sortedMap.emplace(type, uniquesTemp);
  }
  return sortedMap;
}

std::vector<std::shared_ptr<OStreamUtilities::PrintMatrix2D>> OStreamUtilities::OutputFunctions::unpackSortedMapIntoMatricies(std::map<DataObject::Type, std::vector<DataPath>>& sortedMap,
                                                                                                                              DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                                                                                                              bool includeIndex, bool includeHeaders, bool includeNeighborLists)
{
  mesgHandler(IFilter::Message::Type::Info, "Sorting data...");
  std::vector<std::shared_ptr<PrintMatrix2D>> outputPtrs;
  std::map<DataPath, DataObject::Type> arrayMap = {};
  std::vector<DataPath> neighborLists = {};
  // concatenate similar maps
  for(auto& [type, paths] : sortedMap)
  {
    if((paths.size() == 1) && (type != DataObject::Type::NeighborList))
    {
      mesgHandler(IFilter::Message::Type::Info, fmt::format("Assembling single {} print object", type));
      outputPtrs.emplace_back(createSingleTypePrintStringMatrix(dataStructure, paths, type, includeIndex, includeHeaders));
      return outputPtrs;
    }
    switch(type)
    {
    case DataObject::Type::DataArray: {
      mesgHandler(IFilter::Message::Type::Info, "DataArrays identifed");
      arrayMap.merge(createHomogeneousMapfromVector(paths, type));
      break;
    }
    case DataObject::Type::StringArray: {
      mesgHandler(IFilter::Message::Type::Info, "StringArrays Identified");
      arrayMap.merge(createHomogeneousMapfromVector(paths, type));
      break;
    }
    case DataObject::Type::NeighborList: {
      mesgHandler(IFilter::Message::Type::Info, "NeighborLists Identified");
      neighborLists = paths;
      break;
    }
    default: {
      throw std::runtime_error("Cannot process this data structure type");
      break;
    }
    }
  }

  mesgHandler(IFilter::Message::Type::Info, "Assembling primary print object");
  outputPtrs.emplace_back(createMultipleTypePrintStringMatrix(dataStructure, arrayMap, includeIndex, includeHeaders));

  if((neighborLists.size() != 0) && (includeNeighborLists))
  {
    mesgHandler(IFilter::Message::Type::Info, "Assembling NeighborList print objects");
    auto neighborPtrs = createNeighborListPrintStringMatrices(dataStructure, neighborLists, includeIndex, includeHeaders);
    outputPtrs.insert(outputPtrs.end(), neighborPtrs.begin(), neighborPtrs.end());
  }

  return outputPtrs;
}

std::map<size_t, std::string> OStreamUtilities::OutputFunctions::createStringMapFromVector(const std::vector<std::string>& strings)
{
  std::map<size_t, std::string> outputMap;
  for(size_t i = 0; i < strings.size(); i++)
  {
    if(strings[i].find("UNINITIALIZED") == std::string::npos) // if unintialized not found
    {
      outputMap.emplace(i, strings[i]);
    }
  }
  return outputMap;
}

// multiple datapaths, Creates OFStream from filepath [BINARY CAPABLE, unless neighborlist] // endianess must be determined in calling class
void OStreamUtilities::OutputFunctions::printDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::directory_entry& directoryPath,
                                                                     const IFilter::MessageHandler& mesgHandler, std::string fileExtension, bool exportToBinary, const std::string& delimiter,
                                                                     bool includeIndex, bool includeHeaders, size_t componentsPerLine)
// binary bool must come after fileExtension to force caller to input a fileExtension that could support binary
{
  if(!directoryPath.is_directory())
  {
    throw std::runtime_error("directoryPath must be a directory");
  }
  bool hasNeighborLists = false;
  if(exportToBinary)
  {
    includeHeaders = false;
    includeIndex = false;
    componentsPerLine = 0;
  }
  auto objTypes = getDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
      if(objTypes.size() == 1)
      {
        throw std::runtime_error("This function doesn't support singular neighbor lists use printSingleDataObject() instead");
      }
    }
  }
  auto sortedMap = createSortedMapbyType(objectPaths, objTypes); // used later
  auto matrices = unpackSortedMapIntoMatricies(sortedMap, dataStructure, mesgHandler, includeIndex, true);
  ParallelDataAlgorithm dataAlg;
  auto& matrix = matrices[0];             // first store never neighborlist so safe to parse vertically implicitly
  dataAlg.setRange(1, matrix->getRows()); // skip 0 index because headers created implicitly
  size_t arrayIndex = 0;
  mesgHandler(IFilter::Message::Type::Progress, fmt::format("Now processing print object 1 of {}", matrices.size()));
  if(includeIndex)
  {
    arrayIndex++;
  }

  std::vector<size_t> componentCountOrder;
  if(componentsPerLine == 0)
  {
    for(auto& [type, paths] : sortedMap) // this gets parse order correct for naming scheme
    {
      for(const auto& path : paths)
      {
        componentCountOrder.emplace_back(dataStructure.getDataAs<IDataArray>(path)->getNumberOfComponents());
      }
    }
  }
  else
  {
    for(size_t i = 0; i < objectPaths.size(); i++)
    {
      componentCountOrder.emplace_back(componentsPerLine);
    }
  }

  std::map<std::string, std::map<size_t, std::string>> printMap;
  std::ofstream fout;
  auto index = 0;
  while(arrayIndex < matrix->getColumns())
  {
    std::vector<std::string> stringStore(matrix->getRows(), "UNINITIALIZED"); // 1 per array
    dataAlg.execute(OStreamUtilities::AssembleVerticalStringFromIndex(*matrix, stringStore, arrayIndex, delimiter, componentCountOrder[index], exportToBinary));
    auto path = directoryPath.path().string() + "/" + matrix->getValue(arrayIndex) + fileExtension;
    if(exportToBinary)
    {
      fout = std::ofstream(path, std::ofstream::out | std::ios_base::binary);
    }
    else
    {
      fout = std::ofstream(path, std::ofstream::out);
    }
    if(!fout.is_open())
    {
      throw std::runtime_error("Error creating the file");
    }
    printMap.emplace(path, std::map<size_t, std::string>(std::move(createStringMapFromVector(stringStore)))); // try to make as resource efficient as possible
    arrayIndex++;
  }
  if(hasNeighborLists) // cant be binary
  {
    std::vector<std::string> neighborNames;
    for(auto& [type, paths] : sortedMap) // this gets parse order correct for naming scheme
    {
      if(type == DataObject::Type::NeighborList)
      {
        for(const auto& path : paths)
        {
          neighborNames.emplace_back(dataStructure.getDataAs<INeighborList>(path)->getName());
        }
      }
    }
    for(size_t i = 1; i < matrices.size(); i++) // neighborLists automatically stored at the end
    {
      mesgHandler(IFilter::Message::Type::Progress, fmt::format("Now processing print object {} of {}", i, matrices.size()));
      std::vector<std::string> stringStore(matrices[i]->getSize(), "UNINITIALIZED"); // 1 per matrix
      dataAlg.execute(OStreamUtilities::AssembleHorizontalStringFromIndex(*matrices[i], stringStore, delimiter, componentsPerLine));
      auto path = directoryPath.path().string() + "/" + neighborNames[(i - 1)] + fileExtension;
      fout = std::ofstream(path, std::ofstream::out);
      if(!fout.is_open())
      {
        throw std::runtime_error("Error creating the file");
      }
      printMap.emplace(path, std::map<size_t, std::string>(std::move(createStringMapFromVector(stringStore)))); // try to make as resource efficient as possible
    }
  }
  mesgHandler(IFilter::Message::Type::Info, "Printing out to file");
  if(hasNeighborLists)
  {
    multiFileWriteOutWrapper(printMap, false);
  }
  else
  {
    multiFileWriteOutWrapper(printMap, exportToBinary);
  }
}

// single path, custom OStream [BINARY CAPABLE] // endianess must be determined in calling class
void OStreamUtilities::OutputFunctions::printSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                                                              bool exportToBinary, const std::string delimiter, size_t componentsPerLine)
{
  bool hasNeighborLists = false;
  if(exportToBinary)
  {
    componentsPerLine = 0;
  }
  // wrap DataPath in vector to take advantage of existing framework
  const std::vector<DataPath> objectPaths = {objectPath};
  auto objTypes = getDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
    }
  }
  auto matrices = unpackSortedMapIntoMatricies(std::move(createSortedMapbyType(objectPaths, objTypes)), dataStructure, mesgHandler, false);
  // unpack matrix from vector
  auto matrix = matrices[0];

  ParallelDataAlgorithm dataAlg;
  std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
  dataAlg.setRange(0, matrix->getRows());
  mesgHandler(IFilter::Message::Type::Progress, "Now processing print object");
  dataAlg.execute(OStreamUtilities::AssembleVerticalStringFromIndex(*matrix, stringStore, 0, delimiter, componentsPerLine));

  mesgHandler(IFilter::Message::Type::Info, "Printing out");
  if(hasNeighborLists)
  {
    writeOutWrapper(std::move(createStringMapFromVector(stringStore)), outputStrm, false);
  }
  else
  {
    writeOutWrapper(std::move(createStringMapFromVector(stringStore)), outputStrm, exportToBinary);
  }
}

// single path, Creates OFStream from filepath [BINARY CAPABLE] // endianess must be determined in calling class
void OStreamUtilities::OutputFunctions::printSingleDataObject(const DataPath& objectPath, DataStructure& dataStructure, std::filesystem::path& filePath, const IFilter::MessageHandler& mesgHandler,
                                                              bool exportToBinary, const std::string delimiter, size_t componentsPerLine)
{
  bool hasNeighborLists = false;
  if(exportToBinary)
  {
    componentsPerLine = 0;
  }
  // wrap DataPath in vector to take advantage of existing framework
  const std::vector<DataPath> objectPaths = {objectPath};
  auto objTypes = getDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
    }
  }
  auto matrices = unpackSortedMapIntoMatricies(std::move(createSortedMapbyType(objectPaths, objTypes)), dataStructure, mesgHandler, false);
  // unpack matrix from vector
  auto matrix = matrices[0];

  ParallelDataAlgorithm dataAlg;
  std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
  dataAlg.setRange(0, matrix->getSize());
  mesgHandler(IFilter::Message::Type::Progress, "Now processing print object");
  dataAlg.execute(OStreamUtilities::AssembleVerticalStringFromIndex(*matrix, stringStore, 0, delimiter, componentsPerLine));

  mesgHandler(IFilter::Message::Type::Info, "Printing out to file");
  std::ofstream outputStrm;
  if(exportToBinary)
  {
    outputStrm = std::ofstream(filePath.string(), std::ofstream::out | std::ios_base::binary);
  }
  else
  {
    outputStrm = std::ofstream(filePath.string(), std::ofstream::out);
  }
  if(!outputStrm.is_open())
  {
    throw std::runtime_error("Invalid file path");
  }
  if(hasNeighborLists)
  {
    writeOutWrapper(std::move(createStringMapFromVector(stringStore)), outputStrm, false);
  }
  else
  {
    writeOutWrapper(std::move(createStringMapFromVector(stringStore)), outputStrm, exportToBinary);
  }
}

// custom OStream [NO BINARY SUPPORT]
void OStreamUtilities::OutputFunctions::printDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure,
                                                                  const IFilter::MessageHandler& mesgHandler, const std::string& delimiter, bool includeIndex, size_t componentsPerLine,
                                                                  bool includeHeaders, bool includeNeighborLists)
{
  bool hasNeighborLists = false;
  auto objTypes = getDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
    }
  }
  auto matrices = unpackSortedMapIntoMatricies(std::move(createSortedMapbyType(objectPaths, objTypes)), dataStructure, mesgHandler, includeIndex, includeHeaders, includeNeighborLists);

  ParallelDataAlgorithm dataAlg;
  std::vector<std::map<size_t, std::string>> stringStoreList;
  auto count = 0;
  for(auto& matrix : matrices) // neighborLists automatically stored at the end
  {
    mesgHandler(IFilter::Message::Type::Progress, fmt::format("Now processing print object {} of {}", count, matrices.size()));
    std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
    dataAlg.setRange(0, matrix->getSize());
    dataAlg.execute(OStreamUtilities::AssembleHorizontalStringFromIndex(*matrix, stringStore, delimiter, componentsPerLine));
    stringStoreList.emplace_back(createStringMapFromVector(stringStore));
    count++;
  }

  mesgHandler(IFilter::Message::Type::Info, "Printing out");
  for(size_t i = 0; i < stringStoreList.size(); i++)
  {
    writeOutWrapper(stringStoreList[i], outputStrm);
    if(i < (stringStoreList.size() - 1))
    {
      outputStrm << "\n"; // seperate neighborlists
    }
  }
}

// Creates OFStream from filepath [NO BINARY SUPPORT]
void OStreamUtilities::OutputFunctions::printDataSetsToSingleFile(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::path& filePath,
                                                                  const IFilter::MessageHandler& mesgHandler, const std::string& delimiter, bool includeIndex, size_t componentsPerLine,
                                                                  bool includeHeaders, bool includeNeighborLists)
{
  bool hasNeighborLists = false;
  auto objTypes = getDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
    }
  }
  auto matrices = unpackSortedMapIntoMatricies(std::move(createSortedMapbyType(objectPaths, objTypes)), dataStructure, mesgHandler, includeIndex, includeHeaders, includeNeighborLists);

  ParallelDataAlgorithm dataAlg;

  std::vector<std::map<size_t, std::string>> stringStoreList;
  auto count = 0;
  for(auto& matrix : matrices) // neighborLists automatically stored at the end
  {
    mesgHandler(IFilter::Message::Type::Progress, fmt::format("Now processing print object {} of {}", count, matrices.size()));
    std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
    dataAlg.setRange(0, matrix->getSize());
    dataAlg.execute(OStreamUtilities::AssembleHorizontalStringFromIndex(*matrix, stringStore, delimiter, componentsPerLine));
    stringStoreList.emplace_back(createStringMapFromVector(stringStore));
    count++;
  }

  std::ofstream outputStrm(filePath.string(), std::ofstream::out); // overwrite old file just in case
  if(!outputStrm.is_open())
  {
    throw std::runtime_error("Invalid file path");
  }

  mesgHandler(IFilter::Message::Type::Info, "Printing out");
  for(size_t i = 0; i < stringStoreList.size(); i++)
  {
    writeOutWrapper(stringStoreList[i], outputStrm);
    if(i < (stringStoreList.size() - 1))
    {
      outputStrm << "\n"; // seperate neighborlists
    }
  }
}
