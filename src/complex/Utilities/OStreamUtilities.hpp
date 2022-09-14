#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/StringArray.hpp"

#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#include <fstream>
#include <ostream>
#include <sstream>
#include <vector>

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

class COMPLEX_EXPORT OutputFunctions
{
public:
  OutputFunctions::OutputFunctions()
  {
  }
  ~OutputFunctions() = default;

  // multiple datapaths, Creates OFStream from filepath [BINARY CAPABLE, unless neighborlist] // endianess must be determined in calling class
  void printDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::directory_entry& directoryPath, std::string fileExtension = ".txt",
                                    bool exportToBinary = false, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false, size_t componentsPerLine = 0);

  // single path, custom OStream [BINARY CAPABLE] // endianess must be determined in calling class
  void printSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const std::string delimiter = "", bool exportToBinary = false,
                             size_t componentsPerLine = 0);

  // single path, Creates OFStream from filepath [BINARY CAPABLE] // endianess must be determined in calling class
  void printSingleDataObject(const DataPath& objectPath, DataStructure& dataStructure, std::filesystem::path& filePath, const std::string delimiter = "", bool exportToBinary = false,
                             size_t componentsPerLine = 0);

  // custom OStream [NO BINARY SUPPORT]
  void printDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& delimiter = "", bool includeIndex = false,
                                 size_t componentsPerLine = 0, bool includeHeaders = false);

  // Creates OFStream from filepath [NO BINARY SUPPORT]
  void printDataSetsToSingleFile(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::path& filePath, const std::string& delimiter = "", bool includeIndex = false,
                                 size_t componentsPerLine = 0, bool includeHeaders = false);

private:
  void neighborListImplWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, bool hasIndex = false, bool hasHeaders = false);

  void dataAlgParallelWrapper(DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, const size_t& column, bool hasHeaders = false);

  std::vector<std::shared_ptr<PrintMatrix2D>> createNeighborListPrintStringMatrices(DataStructure& dataStructure, const std::vector<DataPath>& paths, bool addIndexValue, bool includeHeaders = false);

  std::shared_ptr<PrintMatrix2D> createSingleTypePrintStringMatrix(DataStructure& dataStructure, const std::vector<DataPath>& paths, const DataObject::Type& typing, bool addIndexValue,
                                                                   bool includeHeaders = false);

  std::shared_ptr<PrintMatrix2D> createMultipleTypePrintStringMatrix(DataStructure& dataStructure, std::map<DataPath, DataObject::Type>& printMapping, bool addIndexValue, bool includeHeaders = false);

  struct AssembleVerticalStringFromIndex;

  struct AssembleHorizontalStringFromIndex;

  template <typename T>
  void writeOut(const std::string& outStr, T& outputStrm, bool isBinary = false);

  template <typename T>
  void writeOutWrapper(std::map<size_t, std::string>& stringStore, T& outputStrm, bool isBinary = false);

  void multiFileWriteOutWrapper(std::map<std::string, std::map<size_t, std::string>>& printMap, bool isBinary = false);

  DataObject::Type getDataType(const DataPath& objectPath, DataStructure& dataStructure);

  std::vector<DataObject::Type> getDataTypesWrapper(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure);

  template <typename T, typename U>
  void loadMapfromVectors(const std::vector<T>& keys, const std::vector<U>& values, std::map<T, U>& mapToLoad);

  template <typename T, typename U>
  std::map<T, U> createHomogeneousMapfromVector(const std::vector<T>& keys, const U& value);

  template <typename T, typename U>
  std::map<U, std::vector<T>> createSortedMapbyType(const std::vector<T>& uniqueValuesVec, const std::vector<U>& parallelSortingVec);

  std::vector<std::shared_ptr<PrintMatrix2D>> unpackSortedMapIntoMatricies(std::map<DataObject::Type, std::vector<DataPath>>& sortedMap, DataStructure& dataStructure, bool includeIndex = false,
                                                                           bool includeHeaders = false);
};

} // namespace OStreamUtilities

} // namespace complex
