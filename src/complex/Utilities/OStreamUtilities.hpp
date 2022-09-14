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

/**
 * @brief This is the primary datastructure used in print string assembly. It is a wrapper struct for
 * adding additionaly functionality/readability (underlying datastructure is a std::vector)
 * @param numrows The number of rows for the matrix
 * @param numcols The number of columns for the matrix
 */
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
  /*
   * This function is used to set balance to true if all values are initialized.
   */
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

/**
 * @brief Parallel enabled | implicit loading of **DataArray**'s elements to PrintMatrix2D vertically
 * @tparam T The primitive type attacthed to **DataArray**
 * @param outputMatrix The PrintMatrix2D that will store string values from *inputArray*
 * @param inputArray The **DataArray** that will have its values translated into strings
 * @param columnToWrite The column that the **DataArray** should be stored in
 * @param hasHeaders bool to determine if index must be advanced 1 to avoid overwrite
 */
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

/**
 * @brief implicit loading of **NeighborList**'s elements to PrintMatrix2D
 * @tparam T The primitive type attacthed to **NeighborList**
 * @param outputMatrix The PrintMatrix2D that will store string values from *inputArray*
 * @param inputArray The **NeighborList** that will have its values translated into strings
 * @param hasIndex bool to determine if index must be advanced 1 to avoid row overwrite
 * @param hasHeaders bool to determine if index must be advanced 1 to avoid column overwrite
 */
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

/**
 * @brief Parallel enabled | implicit loading of **StringArray**'s elements to PrintMatrix2D vertically
 * @param outputMatrix The PrintMatrix2D that will store string values from *inputArray*
 * @param inputArray The **StringArray** that will have its values translated into strings
 * @param columnToWrite The column that the **StringArray** should be stored in
 * @param hasHeaders bool to determine if index must be advanced 1 to avoid overwrite
 */
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

/**
 * @brief Parallel enabled | parse PrintMatrix2D vertically to assemble strings according to params
 * @param outputMatrix The PrintMatrix2D that will be parsed
 * @param
 * @param verticalColumnToPrint The index of column to be parsed
 * @param delimiter The delimiter to be inserted into string (leave blank if binary is end output)
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
struct AssembleVerticalStringFromIndex
{
  AssembleVerticalStringFromIndex(PrintMatrix2D& matrix, std::map<size_t, std::string>& stringStore, const int32 verticalColumnToPrint, const std::string delimiter = "",
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
    size_t length = m_Matrix.getRows(); // will print entire array on one line if no componentsPerLine provided
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
    if(m_Matrix.getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix.getValue(i, m_VertColumn) << m_Delim;
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
          sstrm << m_Matrix.getValue(i, m_VertColumn);
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
          const auto& selected = m_Matrix.getValue(i, m_VertColumn);
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
          const auto& selected = m_Matrix.getValue(i, m_VertColumn);
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
  PrintMatrix2D& m_Matrix;
  std::map<size_t, std::string>& m_StringStore;
  const size_t m_MaxComp;
  const std::string m_Delim = "";
  const int32 m_VertColumn = 0;
};

/**
 * @brief Parallel enabled | parse PrintMatrix2D horizontally to assemble strings according to params
 * @param outputMatrix The PrintMatrix2D that will be parsed
 * @param
 * @param delimiter The delimiter to be inserted into string (leave blank if binary is end output)
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
struct AssembleHorizontalStringFromIndex
{
  AssembleHorizontalStringFromIndex(PrintMatrix2D& matrix, std::map<size_t, std::string>& stringStore, const std::string delimiter = "", const size_t componentsPerLine = 0)
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
    size_t length = m_Matrix.getColumns(); // default is one tuple per line
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
    if(m_Matrix.getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix.getValue(i) << m_Delim;
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
          sstrm << m_Matrix.getValue(i);
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
          if(m_Matrix.getValue(i).find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
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
            sstrm << m_Matrix.getValue(i) << m_Delim;
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
          if(m_Matrix.getValue(i).find("UNINITIALIZED") != std::string::npos) // if is empty insert new line in string and reset count
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
            sstrm << m_Matrix.getValue(i);
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
  PrintMatrix2D& m_Matrix;
  std::map<size_t, std::string>& m_StringStore;
  const size_t m_MaxComp;
  const std::string m_Delim = "";
};

/**
 * @brief Wrapper class for the print functions
 */
class COMPLEX_EXPORT OutputFunctions
{
public:
  OutputFunctions::OutputFunctions()
  {
  }
  ~OutputFunctions() = default;

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
  void printDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::directory_entry& directoryPath, std::string fileExtension = ".txt",
                                    bool exportToBinary = false, const std::string& delimiter = "", bool includeIndex = false, bool includeHeaders = false, size_t componentsPerLine = 0);

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
  void printSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, bool exportToBinary = false, const std::string delimiter = "",
                             size_t componentsPerLine = 0);

  /**
   * @brief [BINARY CAPABLE, unless neighborlist][Single File Output] | writes one IArray child to ofstream | !!!!endianess must be addressed in calling class!!!!
   * @param objectPath The datapath for respective dataObject to be written out
   * @param dataStructure The complex datastructure where *objectPath* datacontainer is stored
   * @param filePath The path to file to write to | used to create file paths for ofstream | will overwrite existing contents
   * //params with defaults
   * @param exportToBinary The boolean that determines if it writes out binary
   * @param delimiter The delimiter to be inserted into string | leave blank if binary is end output
   * @param componentsPerLine The amount of elements to be inserted before newline character
   */
  void printSingleDataObject(const DataPath& objectPath, DataStructure& dataStructure, std::filesystem::path& filePath, bool exportToBinary = false, const std::string delimiter = "",
                             size_t componentsPerLine = 0);

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
  void printDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const std::string& delimiter = "", bool includeIndex = false,
                                 size_t componentsPerLine = 0, bool includeHeaders = false, bool includeNeighborLists = false);

  /**
   * @brief [Single File Output] | writes out multiple arrays to single file
   * @param objectPaths The vector of datapaths for respective dataObjects to be written out
   * @param dataStructure The complex datastructure where *objectPaths* datacontainers are stored
   * @param filePath The path to file to write to | used to create file paths for ofstream | will overwrite existing contents
   * //params with defaults
   * @param delimiter The delimiter to be inserted into string
   * @param includeIndex The boolean that determines if "Feature_IDs" are printed
   * @param componentsPerLine The amount of elements to be inserted before newline character
   * @param includeHeaders The boolean that determines if headers are printed
   * @param IncludeNeighborLists The boolean that determines if NeighborLists are printed at the bottom of file
   */
  void printDataSetsToSingleFile(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, std::filesystem::path& filePath, const std::string& delimiter = "", bool includeIndex = false,
                                 size_t componentsPerLine = 0, bool includeHeaders = false, bool includeNeighborLists = false);

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
                                                                           bool includeHeaders = false, bool includeNeighborLists = false);
};

} // namespace OStreamUtilities

} // namespace complex
