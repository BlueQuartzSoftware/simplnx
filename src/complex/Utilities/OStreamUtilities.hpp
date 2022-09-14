#pragma once

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
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using namespace complex;

namespace complex
{
namespace OStreamUtilities
{
struct PrintMatrix2D
{
  // visual representation of string matrix
  /*
  * ****NOTE functionality for storing neighborlist below has been removed to preserve simplicity of use****
  *
    ID | DataArray1 | StringArray | DataArray2  //Note the ID column is optional this is just an example (**view note next to neightborlist**)
  --------------------------------------------
  | "1"   "value"      "String"      "value" |  In this example matrix, "value" represents any numeric type
  | "2"   "value"      "String"      "value" |  in string representation based off parent dataArray typing.
  | "3"   "value"      "String"      "value" |  [ie if DataArray1 was of type float64 then respective column
  | "4"   "value"      "String"      "value" |  will be string represtentation of doubles to the end of array's index]
  | "5"   "value"      "String"      "value" |  However, its important to note that these columns are not explicitly
  | "6"   "value"      "String"      "value" |  numerically typed, so if you pull index of column below end of
  | ...     ...          ...           ...   |  dataArray it can give odd values such as UNINITIALIZED or unexpected values(if stored below).

  NeighborList Representation (must be processed horizontally)
    ID   |  NumNeighbor  |       Values           ...        //Just an example usage
  -----------------------------------------------------------
  | "1"         "2"              "num"           "num"      |  In this case, UNITIALIZED are essentially empty values and will be ignored in
  | "2"         "1"              "num"      "UNINITAILIZED" |  print if isBalanced bool is false (default)
  | "3"         "0"         "UNINITAILIZED" "UNINITAILIZED" |  **checkBalanceType() should be run for uniform component arrays to speed up
  | "4"         "2"              "num"           "num"      |  string assembly, but is not required [Neighbor lists and dataGroups will
  | ...          ...              ...             ...       |  [Neighbor lists and dataGroups will almost always be unbalanced so its not
  -----------------------------------------------------------  worth preparsing time, unless you know component count is uniform(Attribute Matrix)]**
  */
private:
  size_t rows, columns;
  // since its being allocated to heap anyways no significant advantage to array over vector (small sizing difference)
  std::vector<std::string> matrix;
  bool isBalanced = false;

public:
  PrintMatrix2D(size_t numrows, size_t numcols)
  : rows(numrows)
  , columns(numcols)
  , matrix(numrows * numcols, std::string("UNINITIALIZED"))
  {
  }
  ~PrintMatrix2D() = default;

  PrintMatrix2D(PrintMatrix2D&) = default;
  PrintMatrix2D(PrintMatrix2D&&) noexcept = default;

  PrintMatrix2D& operator=(const PrintMatrix2D&) = default;
  PrintMatrix2D& operator=(PrintMatrix2D&&) noexcept = default;

  std::string& getValue(size_t row, size_t column) // no error checking for walking off edge
  {
    return matrix[row * columns + column];
  }

  std::string& getValue(size_t index) // no error checking for walking off edge
  {
    return matrix[index];
  }

  const std::string& getValue(size_t row, size_t column) const // no error checking for walking off edge
  {
    return matrix[row * columns + column];
  }

  const std::string& getValue(size_t index) const // no error checking for walking off edge
  {
    return matrix[index];
  }

  std::string& operator()(size_t row, size_t column) // no error checking for walking off edge
  {
    return matrix[row * columns + column];
  }

  std::string& operator()(size_t index) // no error checking for walking off edge
  {
    return matrix[index];
  }

  const std::string& operator()(size_t row, size_t column) const // no error checking for walking off edge
  {
    return matrix[row * columns + column];
  }

  const std::string& operator()(size_t index) const // no error checking for walking off edge
  {
    return matrix[index];
  }

  size_t getSize() const
  {
    return rows * columns;
  }

  size_t getRows() const
  {
    return rows;
  }
  size_t getColumns() const
  {
    return columns;
  }
  bool getBalance() const
  {
    return isBalanced;
  }
  void checkBalanceType() // processor heavy, avoid running if you know its unbalanced
  {
    bool balanced = true;
    for(const auto& value : matrix)
    {
      if(value.find("UNINITIALIZED") != std::string::npos)
      {
        balanced = false;
      }
    }
    isBalanced = balanced;
  }
};

template <typename T>
struct LoadDataArrayToMatrixImpl
{
  LoadDataArrayToMatrixImpl(PrintMatrix2D& outputMatrix, const DataArray<T>& inputArray, const size_t& columnToWrite, bool hasHeaders = false)
  : m_OutputMatrix(outputMatrix)
  , m_ColumnToWrite(columnToWrite)
  , m_InputArray(inputArray)
  , m_HasHeaders(hasHeaders)
  {
  }
  ~LoadDataArrayToMatrixImpl() = default;

  void loadToMatrix(size_t start, size_t end) const
  {
    if(m_HasHeaders)
    {
      size_t row;
      for(size_t i = start; i < end; i++)
      {
        row = i + 1;
        m_OutputMatrix.getValue(row, m_ColumnToWrite) = StringUtilities::number(m_InputArray[i]);
      }
    }
    else
    {
      for(size_t i = start; i < end; i++)
      {
        m_OutputMatrix.getValue(i, m_ColumnToWrite) = StringUtilities::number(m_InputArray[i]);
      }
    }
  }
  void operator()(const ComplexRange& range) const
  {
    loadToMatrix(range.min(), range.max());
  }

private:
  PrintMatrix2D& m_OutputMatrix;
  const size_t& m_ColumnToWrite;
  const DataArray<T>& m_InputArray;
  bool m_HasHeaders = false;
};

template <typename T>
struct LoadNeighborListToMatrixImpl // adds component count implicitly
{
  LoadNeighborListToMatrixImpl(PrintMatrix2D& outputMatrix, const NeighborList<T>& inputArray, bool hasIndex = false, bool hasHeader = false)
  : m_OutputMatrix(outputMatrix)
  , m_InputArray(inputArray)
  , m_HasIndex(hasIndex)
  , m_HasHeader(hasHeader)
  {
  }
  ~LoadNeighborListToMatrixImpl() = default;

  void loadToMatrix(size_t start, size_t end) const // use initial logic checking to avoid repeating ifs in loops
  {
    size_t row, column;
    if(m_HasHeader)
    {
      if(m_HasIndex) // yes index yes header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          row = i + 1;
          auto grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(row, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            column = index + 2;
            m_OutputMatrix.getValue(row, column) = StringUtilities::number(grain[index]);
          }
        }
      }
      else // no index yes header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          row = i + 1;
          auto grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(row, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            column = index + 1;
            m_OutputMatrix.getValue(row, index) = StringUtilities::number(grain[index]);
          }
        }
      }
    }
    else // no header
    {
      if(m_HasIndex) // yes index no header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          auto grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(i, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            column = index + 2;
            m_OutputMatrix.getValue(i, column) = StringUtilities::number(grain[index]);
          }
        }
      }
      else // no index no header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          auto grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(i, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            column = index + 1;
            m_OutputMatrix.getValue(i, index) = StringUtilities::number(grain[index]);
          }
        }
      }
    }
  }

  // void operator()(const ComplexRange& range) const // to complex to implement reasonably
  //{
  //     loadToMatrix(range.min(), range.max());
  // }

private:
  PrintMatrix2D& m_OutputMatrix;
  const NeighborList<T>& m_InputArray;
  bool m_HasIndex = false;
  bool m_HasHeader = false;
};

struct LoadStringArrayToMatrixImpl
{
public:
  LoadStringArrayToMatrixImpl(PrintMatrix2D& outputMatrix, const StringArray& inputArray, const size_t& columnToWrite, bool hasHeaders = false)
  : m_OutputMatrix(outputMatrix)
  , m_ColumnToWrite(columnToWrite)
  , m_InputArray(inputArray)
  , m_HasHeaders(hasHeaders)
  {
  }
  ~LoadStringArrayToMatrixImpl() = default;
  void loadToMatrix(size_t start, size_t end) const
  {
    if(m_HasHeaders)
    {
      size_t row;
      for(size_t i = start; i < end; i++)
      {
        row = i + 1;
        m_OutputMatrix.getValue(row, m_ColumnToWrite) = m_InputArray[i];
      }
    }
    else
    {
      for(size_t i = start; i < end; i++)
      {
        m_OutputMatrix.getValue(i, m_ColumnToWrite) = m_InputArray[i];
      }
    }
  }
  void operator()(const ComplexRange& range) const
  {
    loadToMatrix(range.min(), range.max());
  }

private:
  PrintMatrix2D& m_OutputMatrix;
  const size_t& m_ColumnToWrite;
  const StringArray& m_InputArray;
  bool m_HasHeaders = false;
};

void neighborListImplWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, bool hasIndex = false, bool hasHeaders = false)
{
  if(type == DataObject::Type::NeighborList) // unique because its loaded horizontally no dataAlg use
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
    }
  }
  else
  {
    throw std::runtime_error("Function can only process neighborLists");
  }
}

void dataAlgParallelWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, const size_t& column, bool hasHeaders = false)
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
    }
  }
  else
  {
    throw std::runtime_error("Cannot process print data for this type");
  }
}

std::vector<std::shared_ptr<PrintMatrix2D>> createNeighborListPrintStringMatrices(DataStructure& dataStructure, const std::vector<DataPath>& paths, bool addIndexValue, bool includeHeaders = false)
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
    columns += list->getNumberOfComponents();

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
        for(size_t i = 0; i < rows; i++)
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
        for(size_t i = 0; i < rows; i++)
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

std::shared_ptr<PrintMatrix2D> createSingleTypePrintStringMatrix(DataStructure& dataStructure, const std::vector<DataPath>& paths, const DataObject::Type& typing, bool addIndexValue,
                                                                 bool includeHeaders = false)
// implicitly runs checkBalance [except on neighborLists]
{ // double check that virtual typing is not neccessary
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
      for(size_t i = 0; i < rows; i++)
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

std::shared_ptr<PrintMatrix2D> createMultipleTypePrintStringMatrix(DataStructure& dataStructure, std::map<DataPath, DataObject::Type>& printMapping, bool addIndexValue, bool includeHeaders = false)
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
    case DataObject::Type::StringArray: {
      compCounts.push_back(dataStructure.getDataAs<StringArray>(key)->getSize());
      break;
    }
    case DataObject::Type::DataArray: {
      compCounts.push_back(dataStructure.getDataAs<IDataArray>(key)->getSize());
      break;
    }
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
      for(size_t i = 0; i < rows; i++)
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
  return matrixPtr;
}

struct AssembleVerticalStringFromIndex
{
  AssembleVerticalStringFromIndex(std::shared_ptr<PrintMatrix2D> matrix, std::map<size_t, std::string>& stringStore, const int32 verticalColumnToPrint, const std::string delimiter = "",
                                  const size_t componentsPerLine = 0)
  : m_Matrix(matrix)
  , m_StringStore(stringStore)
  , m_MaxComp(componentsPerLine)
  , m_Delim(delimiter)
  , m_VertColumn(verticalColumnToPrint)
  {
  }
  ~AssembleVerticalStringFromIndex() = default;

  void verticalPrint(const size_t start, const size_t end) const
  {
    std::stringstream sstrm;
    size_t length = m_Matrix->getRows(); // will print entire array on one line if no componentsPerLine provided
    size_t count = 0;
    size_t elementCount = 0;
    size_t totalCount = 0;
    size_t stringDumpCount = end - start; // get estimated buffer length based on parallelization calculations
    if(m_MaxComp != 0)                    // if a custom print length provided
    {
      length = m_MaxComp;
    }
    if((start != 0) && (m_MaxComp != 0)) // if start not index 0, then figure out position relative to current column
    {
      count = static_cast<size_t>(std::round(((start + 1) % length) * length)); // plus one converts index to component count
    }
    else // printing full array so it doesnt matter if start is zero or not for newline char
    {
      count = start;
    }
    if(m_Matrix->getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix->getValue(i, m_VertColumn) << m_Delim;
          count++;
          elementCount++;
          totalCount += 2;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
          }
          if(totalCount >= stringDumpCount)
          {
            m_StringStore.emplace((i - elementCount), sstrm.str());
            sstrm.flush();
            totalCount = elementCount = 0;
          }
        }
      }
      else // no delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix->getValue(i, m_VertColumn);
          count++;
          elementCount++;
          totalCount++;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
          }
          if(totalCount >= stringDumpCount)
          {
            m_StringStore.emplace((i - elementCount), sstrm.str());
            sstrm.flush();
            totalCount = elementCount = 0;
          }
        }
      }
    }
    else // unbalanced data arrays (must search for uninitialized)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          const auto& selected = m_Matrix->getValue(i, m_VertColumn);
          if(selected.find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
          else // not empty matrix slot
          {
            sstrm << selected << m_Delim;
            count++;
            elementCount++;
            totalCount += 2;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
              totalCount++;
            }
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
        }
      }
      else // empty delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          const auto& selected = m_Matrix->getValue(i, m_VertColumn);
          if(selected.find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
          else // not empty matrix slot
          {
            sstrm << selected;
            count++;
            elementCount++;
            totalCount++;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
              totalCount++;
            }
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
        }
      }
    }
  }
  void operator()(ComplexRange range) const // for this parallelization range should go to size of largest data array to avoid hitting possible neighborlists for default printing
  {
    verticalPrint(range.min(), range.max());
  }

private:
  std::shared_ptr<PrintMatrix2D> m_Matrix;
  std::map<size_t, std::string>& m_StringStore;
  const size_t m_MaxComp;
  const std::string m_Delim = "";
  const int32 m_VertColumn = 0;
};

struct AssembleHorizontalStringFromIndex
{
  AssembleHorizontalStringFromIndex(std::shared_ptr<PrintMatrix2D> matrix, std::map<size_t, std::string>& stringStore, const std::string delimiter = "", const size_t componentsPerLine = 0)
  : m_Matrix(matrix)
  , m_StringStore(stringStore)
  , m_MaxComp(componentsPerLine)
  , m_Delim(delimiter)
  {
  }
  ~AssembleHorizontalStringFromIndex() = default;

  void horizontalPrint(const size_t start, const size_t end) const // mostly single file use
  {
    std::stringstream sstrm;
    size_t length = m_Matrix->getColumns(); // default is one tuple per line
    size_t count = 0;
    size_t elementCount = 0;
    size_t totalCount = 0;
    size_t stringDumpCount = end - start; // get estimated buffer length based on parallelization calculations
    if(m_MaxComp != 0)                    // if a custom print length provided
    {
      length = m_MaxComp;
    }
    if(start != 0) // if start not index 0, then figure out position relative to current row for newline char
    {
      count = static_cast<size_t>(std::round(((start + 1) % length) * length)); // plus one converts index to component count
    }
    if(m_Matrix->getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix->getValue(i) << m_Delim;
          count++;
          elementCount++;
          totalCount += 2;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
          }
          if(totalCount >= stringDumpCount)
          {
            m_StringStore.emplace((i - elementCount), sstrm.str());
            sstrm.flush();
            totalCount = elementCount = 0;
          }
        }
      }
      else // no delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix->getValue(i);
          count++;
          elementCount++;
          totalCount++;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
          }
          if(totalCount >= stringDumpCount)
          {
            m_StringStore.emplace((i - elementCount), sstrm.str());
            sstrm.flush();
            totalCount = elementCount = 0;
          }
        }
      }
    }
    else // unbalanced data arrays (must search for uninitialized)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          if(m_Matrix->getValue(i).find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
          else // not empty matrix slot
          {
            sstrm << m_Matrix->getValue(i) << m_Delim;
            count++;
            elementCount++;
            totalCount += 2;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
              totalCount++;
            }
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
        }
      }
      else // empty delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          if(m_Matrix->getValue(i).find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
          {
            sstrm << "\n";
            count = 0;
            totalCount++;
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              totalCount = elementCount = 0;
            }
          }
          else // not empty matrix slot
          {
            sstrm << m_Matrix->getValue(i);
            count++;
            elementCount++;
            totalCount++;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
              totalCount++;
            }
            if(totalCount >= stringDumpCount)
            {
              m_StringStore.emplace((i - elementCount), sstrm.str());
              sstrm.flush();
              totalCount = elementCount = 0;
            }
          }
        }
      }
    }
  }

  void operator()(ComplexRange range) const // this parallelization should feed the size of the entire matrix into the upper range limit for default printing
  {
    horizontalPrint(range.min(), range.max());
  }

private:
  std::shared_ptr<PrintMatrix2D> m_Matrix;
  std::map<size_t, std::string>& m_StringStore;
  const size_t m_MaxComp;
  const std::string m_Delim = "";
};

template <typename T>
void writeOut(const std::string& outStr, T& outputStrm, bool isBinary = false)
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
void writeOutWrapper(std::map<size_t, std::string>& stringStore, T& outputStrm, bool isBinary = false)
// requires unique stringStore for each Print2DMatrix to function properly
{
  for(auto& [key, value] : stringStore) // take advantage of maps automatic descending order sort
  {
    writeOut(value, outputStrm, isBinary);
  }
}

void multiFileWriteOutWrapper(std::map<std::string, std::map<size_t, std::string>>& printMap, bool isBinary = false)
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

DataObject::Type getDataType(const DataPath& objectPath, DataStructure& dataStructure)
{
  return dataStructure.getDataAs<DataObject>(objectPath)->getDataObjectType();
}

std::vector<DataObject::Type> getDataTypesWrapper(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure)
{
  std::vector<DataObject::Type> types;
  for(const auto& path : objectPaths)
  {
    types.push_back(std::move(getDataType(path, dataStructure)));
  }
  return types;
}

template <typename T, typename U>
void loadMapfromVectors(const std::vector<T>& keys, const std::vector<U>& values, std::map<T, U>& mapToLoad) // vectors must be same size
{
  std::transform(keys.begin(), keys.end(), values.begin(), std::inserter(mapToLoad, mapToLoad.end()), [](T a, U b) { return std::make_pair(a, b); });
}

template <typename T, typename U>
std::map<T, U> createHomogeneousMapfromVector(const std::vector<T>& keys, const U& value)
{
  std::map<T, U> homogMap;
  for(const auto& key : keys)
  {
    homogMap.emplace(key, value);
  }
  return homogMap;
}

template <typename T, typename U>
std::map<U, std::vector<T>> createSortedMapbyType(const std::vector<T>& uniqueValuesVec, const std::vector<U>& parallelSortingVec)
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

std::vector<std::shared_ptr<PrintMatrix2D>> unpackSortedMapIntoMatricies(std::map<DataObject::Type, std::vector<DataPath>>& sortedMap, DataStructure& dataStructure, bool includeIndex = false,
                                                                         bool includeHeaders = false)
{
  std::vector<std::shared_ptr<PrintMatrix2D>> outputPtrs;
  std::map<DataPath, DataObject::Type> arrayMap = {};
  std::vector<DataPath> neighborLists = {};
  // concatenate similar maps
  for(auto& [type, paths] : sortedMap)
  {
    if((sortedMap.size() == 1) && (type != DataObject::Type::NeighborList))
    {
      outputPtrs.emplace_back(createSingleTypePrintStringMatrix(dataStructure, paths, type, includeIndex, includeHeaders));
      return outputPtrs;
    }
    switch(type)
    {
    case DataObject::Type::DataArray: {
      arrayMap.merge(createHomogeneousMapfromVector(paths, type));
      break;
    }
    case DataObject::Type::StringArray: {
      arrayMap.merge(createHomogeneousMapfromVector(paths, type));
      break;
    }
    case DataObject::Type::NeighborList: {
      neighborLists = paths;
      break;
    }
    }
  }

  outputPtrs.emplace_back(createMultipleTypePrintStringMatrix(dataStructure, arrayMap, includeIndex, includeHeaders));

  if(neighborLists.size() != 0)
  {
    auto neighborPtrs = createNeighborListPrintStringMatrices(dataStructure, neighborLists, includeIndex, includeHeaders);
    outputPtrs.insert(outputPtrs.end(), neighborPtrs.begin(), neighborPtrs.end());
  }

  return outputPtrs;
}

namespace OutputFunctions
{
// multiple datapaths, Creates OFStream from filepath [BINARY CAPABLE, unless neighborlist] // endianess must be determined in calling class
void printDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::directory_entry& directoryPath, std::string fileExtension = ".txt",
                                  bool exportToBinary = false, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false, size_t componentsPerLine = 0)
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
  auto matrices = unpackSortedMapIntoMatricies(sortedMap, dataStructure, includeIndex, true);
  ParallelDataAlgorithm dataAlg;
  auto& matrix = matrices[0];             // first store never neighborlist so safe to parse vertically implicitly
  dataAlg.setRange(1, matrix->getRows()); // skip 0 index because headers created implicitly
  size_t arrayIndex = 0;
  if(includeIndex)
  {
    arrayIndex++;
  }
  std::map<std::string, std::map<size_t, std::string>> printMap;
  std::ofstream fout;
  while(arrayIndex < matrix->getColumns())
  {
    std::map<size_t, std::string> stringStore; // 1 per array
    dataAlg.execute(AssembleVerticalStringFromIndex(matrix, stringStore, arrayIndex, delimiter, componentsPerLine));
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
    printMap.emplace(path, std::map<size_t, std::string>(std::move(stringStore))); // try to make as resource efficient as possible
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
      std::map<size_t, std::string> stringStore; // 1 per matrix
      dataAlg.execute(AssembleHorizontalStringFromIndex(matrix, stringStore, delimiter, componentsPerLine));
      auto path = directoryPath.path().string() + "/" + neighborNames[(i - 1)] + fileExtension;
      fout = std::ofstream(path, std::ofstream::out);
      if(!fout.is_open())
      {
        throw std::runtime_error("Error creating the file");
      }
      printMap.emplace(path, std::map<size_t, std::string>(std::move(stringStore))); // try to make as resource efficient as possible
    }
  }

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
void printSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const std::string delimiter = "", bool exportToBinary = false,
                           size_t componentsPerLine = 0)
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
  auto matrices = unpackSortedMapIntoMatricies(createSortedMapbyType(objectPaths, objTypes), dataStructure, false);
  // unpack matrix from vector
  auto matrix = matrices[0];

  ParallelDataAlgorithm dataAlg;
  std::map<size_t, std::string> stringStore; // 1 per matrix
  dataAlg.setRange(0, matrix->getRows());
  dataAlg.execute(AssembleVerticalStringFromIndex(matrix, stringStore, 0, delimiter, componentsPerLine));

  if(hasNeighborLists)
  {
    writeOutWrapper(stringStore, outputStrm, false);
  }
  else
  {
    writeOutWrapper(stringStore, outputStrm, exportToBinary);
  }
}

// single path, Creates OFStream from filepath [BINARY CAPABLE] // endianess must be determined in calling class
void printSingleDataObject(const DataPath& objectPath, DataStructure& dataStructure, std::filesystem::path& filePath, const std::string delimiter = "", bool exportToBinary = false,
                           size_t componentsPerLine = 0)
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
  auto matrices = unpackSortedMapIntoMatricies(createSortedMapbyType(objectPaths, objTypes), dataStructure, false);
  // unpack matrix from vector
  auto matrix = matrices[0];

  ParallelDataAlgorithm dataAlg;
  std::map<size_t, std::string> stringStore; // 1 per matrix
  dataAlg.setRange(0, matrix->getSize());
  dataAlg.execute(AssembleVerticalStringFromIndex(matrix, stringStore, 0, delimiter, componentsPerLine));

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
    writeOutWrapper(stringStore, outputStrm, false);
  }
  else
  {
    writeOutWrapper(stringStore, outputStrm, exportToBinary);
  }
}

// custom OStream [NO BINARY SUPPORT]
void printDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& delimiter = "", bool includeIndex = false,
                               size_t componentsPerLine = 0, bool includeHeaders = false)
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
  auto matrices = unpackSortedMapIntoMatricies(createSortedMapbyType(objectPaths, objTypes), dataStructure, includeIndex);

  ParallelDataAlgorithm dataAlg;
  std::vector<std::map<size_t, std::string>> stringStoreList;
  for(auto& matrix : matrices) // neighborLists automatically stored at the end
  {
    std::map<size_t, std::string> stringStore; // 1 per matrix
    dataAlg.setRange(0, matrix->getSize());
    dataAlg.execute(AssembleHorizontalStringFromIndex(matrix, stringStore, delimiter, componentsPerLine));
    stringStoreList.push_back(stringStore);
  }

  for(auto& stringStore : stringStoreList)
  {
    writeOutWrapper(stringStore, outputStrm);
  }
}

// Creates OFStream from filepath [NO BINARY SUPPORT]
void printDataSetsToSingleFile(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::path& filePath, const std::string& delimiter = "", bool includeIndex = false,
                               size_t componentsPerLine = 0, bool includeHeaders = false)
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
  auto matrices = unpackSortedMapIntoMatricies(createSortedMapbyType(objectPaths, objTypes), dataStructure, hasNeighborLists, includeIndex);

  ParallelDataAlgorithm dataAlg;

  std::vector<std::map<size_t, std::string>> stringStoreList;
  for(auto& matrix : matrices) // neighborLists automatically stored at the end
  {
    std::map<size_t, std::string> stringStore; // 1 per matrix
    dataAlg.setRange(0, matrix->getSize());
    dataAlg.execute(AssembleHorizontalStringFromIndex(matrix, stringStore, delimiter, componentsPerLine));
    stringStoreList.push_back(stringStore);
  }

  std::ofstream outputStrm(filePath.string(), std::ofstream::out); // overwrite old file just in case
  if(!outputStrm.is_open())
  {
    throw std::runtime_error("Invalid file path");
  }

  for(auto& stringStore : stringStoreList)
  {
    writeOutWrapper(stringStore, outputStrm);
  }

}

} // namespace OutputFunctions

} // namespace OStreamUtilities

} // namespace complex
