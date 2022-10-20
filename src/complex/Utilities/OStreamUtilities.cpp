#include "OStreamUtilities.hpp"
#include "StringUtilities.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace complex;

namespace // for nonmember functions
{
/**
 * @brief This is the primary datastructure used in print string assembly. It is a wrapper struct for
 * adding additionaly functionality/readability (underlying datastructure is a std::vector)
 * @param numrows The number of rows for the matrix
 * @param numcols The number of columns for the matrix
 */
class PrintMatrix2D
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
public:
  PrintMatrix2D(size_t numrows, size_t numcols)
  : m_Rows(numrows)
  , m_Columns(numcols)
  , m_Matrix(numrows * numcols, "UNINITIALIZED")
  {
  }

  std::string& getValue(size_t row, size_t column) // no error checking for walking off edge
  {
    return m_Matrix[row * m_Columns + column];
  }

  std::string& getValue(size_t index) // no error checking for walking off edge
  {
    return m_Matrix[index];
  }

  const std::string& getValue(size_t row, size_t column) const // no error checking for walking off edge
  {
    return m_Matrix[row * m_Columns + column];
  }

  const std::string& getValue(size_t index) const // no error checking for walking off edge
  {
    return m_Matrix[index];
  }

  std::string& operator()(size_t row, size_t column) // no error checking for walking off edge
  {
    return m_Matrix[row * m_Columns + column];
  }

  std::string& operator()(size_t index) // no error checking for walking off edge
  {
    return m_Matrix[index];
  }

  const std::string& operator()(size_t row, size_t column) const // no error checking for walking off edge
  {
    return m_Matrix[row * m_Columns + column];
  }

  const std::string& operator()(size_t index) const // no error checking for walking off edge
  {
    return m_Matrix[index];
  }

  size_t getSize() const
  {
    return m_Rows * m_Columns;
  }

  size_t getRows() const
  {
    return m_Rows;
  }
  size_t getColumns() const
  {
    return m_Columns;
  }
  bool getBalance() const
  {
    for(const auto& value : m_Matrix)
    {
      if(value.find("UNINITIALIZED") != std::string::npos)
      {
        return false;
      }
    }
    return true;
  }

private:
  size_t m_Rows = 0;
  size_t m_Columns = 0;
  // since its being allocated to heap anyways no significant advantage to array over vector (small sizing difference)
  std::vector<std::string> m_Matrix;
};

/**
 * @brief Parallel enabled | implicit loading of **DataArray**'s elements to PrintMatrix2D vertically
 * @tparam ScalarType The primitive type attacthed to **DataArray**
 * @param outputMatrix The PrintMatrix2D that will store string values from *inputArray*
 * @param inputArray The **DataArray** that will have its values translated into strings
 * @param columnToWrite The column that the **DataArray** should be stored in
 * @param hasHeaders bool to determine if index must be advanced 1 to avoid overwrite
 */
template <typename ScalarType>
class LoadDataArrayToMatrixImpl
{
public:
  LoadDataArrayToMatrixImpl(PrintMatrix2D& outputMatrix, const DataArray<ScalarType>& inputArray, size_t columnToWrite, bool hasHeaders = false)
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
      for(size_t i = start; i < end; i++)
      {
        size_t row = i + 1;
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
  void operator()(const Range& range) const
  {
    loadToMatrix(range.min(), range.max());
  }

private:
  PrintMatrix2D& m_OutputMatrix;
  size_t m_ColumnToWrite = 0;
  const DataArray<ScalarType>& m_InputArray;
  bool m_HasHeaders = false;
};

/**
 * @brief implicit loading of **NeighborList**'s elements to PrintMatrix2D
 * @tparam ScalarType The primitive type attacthed to **NeighborList**
 * @param outputMatrix The PrintMatrix2D that will store string values from *inputArray*
 * @param inputArray The **NeighborList** that will have its values translated into strings
 * @param hasIndex bool to determine if index must be advanced 1 to avoid row overwrite
 * @param hasHeaders bool to determine if index must be advanced 1 to avoid column overwrite
 */
template <typename ScalarType>
class LoadNeighborListToMatrixImpl // adds component count implicitly
{
public:
  LoadNeighborListToMatrixImpl(PrintMatrix2D& outputMatrix, const NeighborList<ScalarType>& inputArray, bool hasIndex = false, bool hasHeader = false)
  : m_OutputMatrix(outputMatrix)
  , m_InputArray(inputArray)
  , m_HasIndex(hasIndex)
  , m_HasHeader(hasHeader)
  {
  }
  ~LoadNeighborListToMatrixImpl() = default;

  void loadToMatrix(size_t start, size_t end) const // use initial logic checking to avoid repeating ifs in loops
  {
    if(m_HasHeader)
    {
      if(m_HasIndex) // yes index yes header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          size_t row = i + 1;
          const auto& grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(row, 0) = StringUtilities::number(i);            // add comp count
          m_OutputMatrix.getValue(row, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            size_t column = index + 2;
            m_OutputMatrix.getValue(row, column) = StringUtilities::number(grain[index]);
          }
        }
      }
      else // no index yes header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          size_t row = i + 1;
          const auto& grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(row, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            size_t column = index + 1;
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
          const auto& grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(i, 0) = StringUtilities::number(i);            // add comp count
          m_OutputMatrix.getValue(i, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            size_t column = index + 2;
            m_OutputMatrix.getValue(i, column) = StringUtilities::number(grain[index]);
          }
        }
      }
      else // no index no header
      {
        for(size_t i = start; i < m_InputArray.getNumberOfLists(); i++)
        {
          const auto& grain = m_InputArray.getListReference(i);
          m_OutputMatrix.getValue(i, 1) = StringUtilities::number(grain.size()); // add comp count
          for(size_t index = 0; index < grain.size(); index++)
          {
            size_t column = index + 1;
            m_OutputMatrix.getValue(i, index) = StringUtilities::number(grain[index]);
          }
        }
      }
    }
  }

private:
  PrintMatrix2D& m_OutputMatrix;
  const NeighborList<ScalarType>& m_InputArray;
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
class LoadStringArrayToMatrixImpl
{
public:
  LoadStringArrayToMatrixImpl(PrintMatrix2D& outputMatrix, const StringArray& inputArray, size_t columnToWrite, bool hasHeaders = false)
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
      for(size_t i = start; i < end; i++)
      {
        size_t row = i + 1;
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
  void operator()(const Range& range) const
  {
    loadToMatrix(range.min(), range.max());
  }

private:
  PrintMatrix2D& m_OutputMatrix;
  size_t m_ColumnToWrite = 0;
  const StringArray& m_InputArray;
  bool m_HasHeaders = false;
};

/**
 * @brief Parallel enabled | parse PrintMatrix2D vertically to assemble strings according to params
 * @param outputMatrix The PrintMatrix2D that will be parsed
 * @param stringStore A vector of strings preinitialized to update by index
 * @param verticalColumnToPrint The index of column to be parsed
 * @param delimiter The delimiter to be inserted into string (leave blank if binary is end output)
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
class AssembleVerticalStringFromIndex
{
public:
  AssembleVerticalStringFromIndex(const PrintMatrix2D& matrix, std::vector<std::string>& stringStore, int32 verticalColumnToPrint, const std::string& delimiter = "", size_t componentsPerLine = 0,
                                  bool isBinary = false)
  : m_Matrix(matrix)
  , m_StringStore(stringStore)
  , m_MaxComp(componentsPerLine)
  , m_Delim(delimiter)
  , m_VertColumn(verticalColumnToPrint)
  , m_IsBinary(isBinary)
  {
  }
  ~AssembleVerticalStringFromIndex() = default;

  void verticalPrint(size_t start, size_t end) const
  {
    std::stringstream sstrm;
    size_t length = m_Matrix.getRows(); // will print entire array on one line if no componentsPerLine provided
    size_t count = 0;
    size_t highestIndex = m_Matrix.getRows();
    if(m_MaxComp != 0) // if a custom print length provided
    {
      length = m_MaxComp;
    }
    if(m_Matrix.getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix.getValue(i, m_VertColumn) << m_Delim;
          if(i != 0 && i % length == 0)
          {
            sstrm << "\n";
          }
        }
      }
      else // no delimiter / check binary as binary should NEVER uses delimiter
      {
        if(m_IsBinary)
        {
          for(size_t i = start; i < end; i++)
          {
            sstrm << m_Matrix.getValue(i, m_VertColumn);
          }
        }
        else
        {
          for(size_t i = start; i < end; i++)
          {
            sstrm << m_Matrix.getValue(i, m_VertColumn);
            if(i != 0 && i % length == 0)
            {
              sstrm << "\n";
            }
          }
        }
      }
    }
    else // unbalanced data arrays (must search for uninitialized)
    {
      if(start > length) // figure out position relative to current column
      {
        count = static_cast<size_t>(start % length);
      }
      else
      {
        count = start;
      }
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          const auto& selected = m_Matrix.getValue(i, m_VertColumn);
          if(selected.find("UNINITIALIZED") == std::string::npos) // if is not empty insert
          {
            sstrm << selected << m_Delim;
            count++;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
            }
            else if(i + 1 < highestIndex) // if next is less than max
            {
              if(m_Matrix.getValue(i + 1).find("UNINITIALIZED") != std::string::npos) // if next is empty
              {
                sstrm << "\n";
                count = 0;
              }
            }
          }
        }
      }
      else // empty delimiter / check binary as binary should NEVER uses delimiter
      {
        if(m_IsBinary)
        {
          for(size_t i = start; i < end; i++)
          {
            const auto& selected = m_Matrix.getValue(i, m_VertColumn);
            if(selected.find("UNINITIALIZED") == std::string::npos) // if is not empty insert
            {
              sstrm << selected;
            }
          }
        }
        else
        {
          for(size_t i = start; i < end; i++)
          {
            const auto& selected = m_Matrix.getValue(i, m_VertColumn);
            if(selected.find("UNINITIALIZED") == std::string::npos) // if has value
            {
              sstrm << selected;
              count++;
              if(count == length)
              {
                sstrm << "\n";
                count = 0;
              }
              else if(i + 1 < highestIndex) // if next is less than max
              {
                if(m_Matrix.getValue(i + 1).find("UNINITIALIZED") != std::string::npos) // if next is empty
                {
                  sstrm << "\n";
                  count = 0;
                }
              }
            }
          }
        }
      }
    }
    m_StringStore[start] = sstrm.str();
    sstrm.flush();
  }

  void operator()(Range range) const // for this parallelization range should go to size of largest data array to avoid hitting possible neighborlists for default printing
  {
    verticalPrint(range.min(), range.max());
  }

private:
  const PrintMatrix2D& m_Matrix;
  std::vector<std::string>& m_StringStore;
  size_t m_MaxComp = 0;
  const std::string m_Delim = {""};
  int32 m_VertColumn = 0;
  bool m_IsBinary = false;
};

/**
 * @brief Parallel enabled | parse PrintMatrix2D horizontally to assemble strings according to params
 * @param outputMatrix The PrintMatrix2D that will be parsed
 * @param stringStore A vector of strings preinitialized to update by index
 * @param delimiter The delimiter to be inserted into string (leave blank if binary is end output)
 * @param componentsPerLine The amount of elements to be inserted before newline character
 */
class AssembleHorizontalStringFromIndex
{
public:
  AssembleHorizontalStringFromIndex(const PrintMatrix2D& matrix, std::vector<std::string>& stringStore, const std::string& delimiter = "", size_t componentsPerLine = 0)
  : m_Matrix(matrix)
  , m_StringStore(stringStore)
  , m_MaxComp(componentsPerLine)
  , m_Delim(delimiter)
  {
  }
  ~AssembleHorizontalStringFromIndex() = default;

  void horizontalPrint(size_t start, size_t end) const // mostly single file use
  {
    std::stringstream sstrm;
    size_t length = m_Matrix.getColumns(); // default is one tuple per line
    size_t count = 0;
    size_t highestIndex = m_Matrix.getSize();
    if(m_MaxComp != 0) // if a custom print length provided
    {
      length = m_MaxComp;
    }
    if(start > length) // if start not index 0, then figure out position relative to current row for newline char
    {
      count = static_cast<size_t>(std::round(start % length)); // plus one converts index to component count
    }
    else
    {
      count = start;
    }
    if(m_Matrix.getBalance()) // faster (all arrays are same length)
    {
      if(!m_Delim.empty())
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix.getValue(i) << m_Delim;
          count++;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
          }
        }
      }
      else // no delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          sstrm << m_Matrix.getValue(i);
          count++;
          if(count == length)
          {
            sstrm << "\n";
            count = 0;
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
          if(m_Matrix.getValue(i).find("UNINITIALIZED") == std::string::npos) // not empty
          {
            sstrm << m_Matrix.getValue(i) << m_Delim;
            count++;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
            }
            else if(i + 1 < highestIndex) // if next is less than max
            {
              if(m_Matrix.getValue(i + 1).find("UNINITIALIZED") != std::string::npos) // if next is empty
              {
                sstrm << "\n";
                count = 0;
              }
            }
          }
        }
      }
      else // empty delimiter
      {
        for(size_t i = start; i < end; i++)
        {
          if(m_Matrix.getValue(i).find("UNINITIALIZED") == std::string::npos) // not empty
          {
            sstrm << m_Matrix.getValue(i);
            count++;
            if(count == length)
            {
              sstrm << "\n";
              count = 0;
            }
            else if(i + 1 < highestIndex) // if next is less than max
            {
              if(m_Matrix.getValue(i + 1).find("UNINITIALIZED") != std::string::npos) // if next is empty
              {
                sstrm << "\n";
                count = 0;
              }
            }
          }
        }
      }
    }
    m_StringStore[start] = sstrm.str();
    sstrm.flush();
  }

  void operator()(Range range) const // this parallelization should feed the size of the entire matrix into the upper range limit for default printing
  {
    horizontalPrint(range.min(), range.max());
  }

private:
  const PrintMatrix2D& m_Matrix;
  std::vector<std::string>& m_StringStore;
  size_t m_MaxComp = 0;
  const std::string m_Delim = {""};
};

void NeighborListImplWrapper(const DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, bool hasIndex, bool hasHeaders)
{
  if(type != DataObject::Type::NeighborList)
  {
    throw std::runtime_error("NeighborListImplWrapper can only process neighborLists");
    return;
  }
  const auto& iNeighborList = dataStructure.getDataRefAs<INeighborList>(path);
  auto dataType = iNeighborList.getDataType();
  switch(dataType)
  {
  case DataType::int8: {
    const auto& neighborList = dynamic_cast<const NeighborList<int8>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::int16: {
    const auto& neighborList = dynamic_cast<const NeighborList<int16>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::int32: {
    const auto& neighborList = dynamic_cast<const NeighborList<int32>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::int64: {
    const auto& neighborList = dynamic_cast<const NeighborList<int64>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::uint8: {
    const auto& neighborList = dynamic_cast<const NeighborList<uint8>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::uint16: {
    const auto& neighborList = dynamic_cast<const NeighborList<uint16>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::uint32: {
    const auto& neighborList = dynamic_cast<const NeighborList<uint32>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::uint64: {
    const auto& neighborList = dynamic_cast<const NeighborList<uint64>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::float32: {
    const auto& neighborList = dynamic_cast<const NeighborList<float32>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  case DataType::float64: {
    const auto& neighborList = dynamic_cast<const NeighborList<float64>&>(iNeighborList);
    LoadNeighborListToMatrixImpl(matrixRef, neighborList, hasIndex, hasHeaders).loadToMatrix(0, neighborList.getSize());
    break;
  }
  default: {
    throw std::runtime_error("Cannot boolean in this data structure type");
    break;
  }
  }
}

void DataAlgParallelWrapper(const DataStructure& dataStructure, PrintMatrix2D& matrixRef, DataObject::Type type, const DataPath& path, size_t column, bool hasHeaders)
{
  ParallelDataAlgorithm dataAlg;
  if(type == DataObject::Type::StringArray)
  {
    const auto& stringArray = dataStructure.getDataRefAs<StringArray>(path);
    if(hasHeaders)
    {
      matrixRef.getValue(0, column) = stringArray.getName();
    }
    dataAlg.setRange(0, stringArray.getSize());
    dataAlg.execute(LoadStringArrayToMatrixImpl(matrixRef, stringArray, column, hasHeaders));
  }
  else if(type == DataObject::Type::DataArray) // Data array of unknown typing
  {
    const auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(path);
    if(hasHeaders)
    {
      matrixRef.getValue(0, column) = iDataArray.getName();
    }
    dataAlg.setRange(0, iDataArray.getSize());
    auto dataType = iDataArray.getDataType();
    switch(dataType)
    {
    case DataType::int8: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<int8>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::int16: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<int16>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::int32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<int32>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::int64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<int64>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::uint8: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<uint8>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::uint16: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<uint16>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::uint32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<uint32>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::uint64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<uint64>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::float32: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<float32>&>(iDataArray), column, hasHeaders));
      break;
    }
    case DataType::float64: {
      dataAlg.execute(LoadDataArrayToMatrixImpl(matrixRef, dynamic_cast<const DataArray<float64>&>(iDataArray), column, hasHeaders));
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

template <typename ScalarType>
size_t GetMaxNeighborlistElements(const NeighborList<ScalarType>& list)
{
  size_t max = 0;
  for(size_t i = 0; i < list.getNumberOfLists(); i++)
  {
    const auto& grain = list.getListReference(i);
    for(size_t index = 0; index < grain.size(); index++)
    {
      ScalarType value = grain[index];
      if(value > max)
      {
        max = value;
      }
    }
  }
  return max;
}

std::vector<std::shared_ptr<PrintMatrix2D>> CreateNeighborListPrintStringMatrices(DataStructure& dataStructure, const std::vector<DataPath>& paths, bool addIndexValue, bool includeHeaders)
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
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int8>>(path));
      break;
    }
    case DataType::int16: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int16>>(path));
      break;
    }
    case DataType::int32: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int32>>(path));
      break;
    }
    case DataType::int64: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<int64>>(path));
      break;
    }
    case DataType::uint8: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint8>>(path));
      break;
    }
    case DataType::uint16: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint16>>(path));
      break;
    }
    case DataType::uint32: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint32>>(path));
      break;
    }
    case DataType::uint64: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<uint64>>(path));
      break;
    }
    case DataType::float32: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<float32>>(path));
      break;
    }
    case DataType::float64: {
      columns += GetMaxNeighborlistElements(dataStructure.getDataRefAs<NeighborList<float64>>(path));
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
    NeighborListImplWrapper(dataStructure, *matrixPtr, DataObject::Type::NeighborList, path, addIndexValue, includeHeaders);
    outputPtrs.push_back(matrixPtr);
  }
  return outputPtrs;
}

std::shared_ptr<PrintMatrix2D> CreateSingleTypePrintStringMatrix(DataStructure& dataStructure, const std::vector<DataPath>& paths, const DataObject::Type& typing, bool addIndexValue,
                                                                 bool includeHeaders)
// implicitly runs checkBalance [except on neighborLists]
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
      DataAlgParallelWrapper(dataStructure, *matrixPtr, typing, paths[i], (i + 1), includeHeaders);
    }
  }
  else
  {
    for(size_t i = 0; i < paths.size(); i++)
    {
      DataAlgParallelWrapper(dataStructure, *matrixPtr, typing, paths[i], i, includeHeaders);
    }
  }
  return matrixPtr;
}

std::shared_ptr<PrintMatrix2D> CreateMultipleTypePrintStringMatrix(DataStructure& dataStructure, std::map<DataPath, DataObject::Type>& printMapping, bool addIndexValue, bool includeHeaders)
{
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
      DataAlgParallelWrapper(dataStructure, *matrixPtr, types[i], paths[i], (i + 1), includeHeaders);
    }
  }
  else
  {
    for(size_t i = 0; i < printMapping.size(); i++)
    {
      DataAlgParallelWrapper(dataStructure, *matrixPtr, types[i], paths[i], i, includeHeaders);
    }
  }
  return matrixPtr;
}

template <typename OutputStream>
void WriteOut(const std::string& outStr, OutputStream& outputStrm, bool isBinary)
{
  if((isBinary) && ((std::is_same<OutputStream, std::ofstream>::value) || (std::is_same<OutputStream, std::ostringstream>::value))) // endianess should be considered before this point
  {
    outputStrm.write(outStr.c_str(), outStr.length());
  }
  else
  {
    outputStrm << outStr;
  }
}

template <typename OutputStream>
void WriteOutWrapper(std::map<size_t, std::string>& stringStore, OutputStream& outputStrm, bool isBinary)
// requires unique stringStore for each Print2DMatrix to function properly
{
  for(auto& [key, value] : stringStore) // take advantage of maps automatic descending order sort
  {
    WriteOut(value, outputStrm, isBinary);
  }
}

void MultiFileWriteOutWrapper(std::map<std::string, std::map<size_t, std::string>>& printMap, bool isBinary)
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
      WriteOut(value, outputStrm, isBinary);
    }
  }
}

DataObject::Type GetDataType(const DataPath& objectPath, DataStructure& dataStructure)
{
  return dataStructure.getDataAs<DataObject>(objectPath)->getDataObjectType();
}

std::vector<DataObject::Type> GetDataTypesWrapper(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure)
{
  std::vector<DataObject::Type> types;
  for(const auto& path : objectPaths)
  {
    types.push_back(std::move(GetDataType(path, dataStructure)));
  }
  return types;
}

template <typename Path, typename ObjectType>
std::map<Path, ObjectType> CreateHomogeneousMapfromVector(const std::vector<Path>& keys, const ObjectType& value)
{
  std::map<Path, ObjectType> homogMap;
  for(const auto& key : keys)
  {
    homogMap.emplace(key, value);
  }
  return homogMap;
}

template <typename ObjectType, typename Path>
std::map<ObjectType, std::vector<Path>> CreateSortedMapbyType(const std::vector<ObjectType>& parallelSortingVec, const std::vector<Path>& uniqueValuesVec)
{
  if(uniqueValuesVec.size() != parallelSortingVec.size())
  {
    throw std::runtime_error("Vectors are not equivalent!");
  }

  auto uniqueTypes = parallelSortingVec;
  std::sort(uniqueTypes.begin(), uniqueTypes.end());
  auto dupes = std::unique(uniqueTypes.begin(), uniqueTypes.end());
  uniqueTypes.erase(dupes, uniqueTypes.end());

  std::map<ObjectType, std::vector<Path>> sortedMap;

  for(const auto& type : uniqueTypes)
  {
    std::vector<Path> uniquesTemp;
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

std::vector<std::shared_ptr<PrintMatrix2D>> UnpackSortedMapIntoMatricies(std::map<DataObject::Type, std::vector<DataPath>>& sortedMap, DataStructure& dataStructure,
                                                                         const IFilter::MessageHandler& mesgHandler, bool includeIndex, bool includeHeaders, bool includeNeighborLists)
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
      outputPtrs.emplace_back(CreateSingleTypePrintStringMatrix(dataStructure, paths, type, includeIndex, includeHeaders));
      return outputPtrs;
    }
    switch(type)
    {
    case DataObject::Type::DataArray: {
      mesgHandler(IFilter::Message::Type::Info, "DataArrays identifed");
      arrayMap.merge(CreateHomogeneousMapfromVector(paths, type));
      break;
    }
    case DataObject::Type::StringArray: {
      mesgHandler(IFilter::Message::Type::Info, "StringArrays Identified");
      arrayMap.merge(CreateHomogeneousMapfromVector(paths, type));
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
  outputPtrs.emplace_back(CreateMultipleTypePrintStringMatrix(dataStructure, arrayMap, includeIndex, includeHeaders));

  if((neighborLists.size() != 0) && (includeNeighborLists))
  {
    mesgHandler(IFilter::Message::Type::Info, "Assembling NeighborList print objects");
    auto neighborPtrs = CreateNeighborListPrintStringMatrices(dataStructure, neighborLists, includeIndex, includeHeaders);
    outputPtrs.insert(outputPtrs.end(), neighborPtrs.begin(), neighborPtrs.end());
  }

  return outputPtrs;
}

std::map<size_t, std::string> CreateStringMapFromVector(const std::vector<std::string>& strings)
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
} // namespace

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
void PrintDataSetsToMultipleFiles(const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, fs::directory_entry& directoryPath, const IFilter::MessageHandler& mesgHandler,
                                  std::string fileExtension, bool exportToBinary, const std::string& delimiter, bool includeIndex, bool includeHeaders, size_t componentsPerLine,
                                  bool includeNeighborLists)
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
  auto objTypes = GetDataTypesWrapper(objectPaths, dataStructure);
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
  auto sortedMap = CreateSortedMapbyType(objTypes, objectPaths); // used later
  auto matrices = UnpackSortedMapIntoMatricies(sortedMap, dataStructure, mesgHandler, includeIndex, true, includeNeighborLists);
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
  size_t index = 0;
  while(arrayIndex < matrix->getColumns())
  {
    std::vector<std::string> stringStore(matrix->getRows(), "UNINITIALIZED"); // 1 per array
    dataAlg.execute(AssembleVerticalStringFromIndex(*matrix, stringStore, arrayIndex, delimiter, componentCountOrder[index], exportToBinary));
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
    printMap.emplace(path, CreateStringMapFromVector(stringStore)); // try to make as resource efficient as possible
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
      dataAlg.execute(AssembleHorizontalStringFromIndex(*matrices[i], stringStore, delimiter, componentsPerLine));
      auto path = directoryPath.path().string() + "/" + neighborNames[(i - 1)] + fileExtension;
      fout = std::ofstream(path, std::ofstream::out);
      if(!fout.is_open())
      {
        throw std::runtime_error("Error creating the file");
      }
      printMap.emplace(path, CreateStringMapFromVector(stringStore)); // try to make as resource efficient as possible
    }
  }
  mesgHandler(IFilter::Message::Type::Info, "Printing out to file");
  if(hasNeighborLists)
  {
    MultiFileWriteOutWrapper(printMap, false);
  }
  else
  {
    MultiFileWriteOutWrapper(printMap, exportToBinary);
  }
};

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
void PrintSingleDataObject(std::ostream& outputStrm, const DataPath& objectPath, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, bool exportToBinary,
                           const std::string& delimiter, size_t componentsPerLine)
{
  bool hasNeighborLists = false;
  if(exportToBinary)
  {
    componentsPerLine = 0;
  }
  // wrap DataPath in vector to take advantage of existing framework
  const std::vector<DataPath> objectPaths = {objectPath};
  auto objTypes = GetDataTypesWrapper(objectPaths, dataStructure);
  for(const auto& objType : objTypes)
  {
    if(objType == DataObject::Type::NeighborList)
    {
      hasNeighborLists = true;
    }
  }
  auto sortedMap = std::move(CreateSortedMapbyType(objTypes, objectPaths));
  auto matrices = UnpackSortedMapIntoMatricies(sortedMap, dataStructure, mesgHandler, false, false, hasNeighborLists);
  // unpack matrix from vector
  auto matrix = matrices[0];

  ParallelDataAlgorithm dataAlg;
  std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
  dataAlg.setRange(0, matrix->getRows());
  mesgHandler(IFilter::Message::Type::Progress, "Now processing print object");
  dataAlg.execute(AssembleVerticalStringFromIndex(*matrix, stringStore, 0, delimiter, componentsPerLine));

  mesgHandler(IFilter::Message::Type::Info, "Printing out");
  auto stringMap = std::move(CreateStringMapFromVector(stringStore));
  if(hasNeighborLists)
  {
    WriteOutWrapper(stringMap, outputStrm, false);
  }
  else
  {
    WriteOutWrapper(stringMap, outputStrm, exportToBinary);
  }
};

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
void PrintDataSetsToSingleFile(std::ostream& outputStrm, const std::vector<DataPath>& objectPaths, DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler,
                               const std::string& delimiter, bool includeIndex, size_t componentsPerLine, bool includeHeaders, bool includeNeighborLists)
{
  auto objTypes = GetDataTypesWrapper(objectPaths, dataStructure);
  auto sortedMap = std::move(CreateSortedMapbyType(objTypes, objectPaths));
  auto matrices = UnpackSortedMapIntoMatricies(sortedMap, dataStructure, mesgHandler, includeIndex, includeHeaders, includeNeighborLists);

  ParallelDataAlgorithm dataAlg;
  std::vector<std::map<size_t, std::string>> stringStoreList;
  size_t count = 0;
  for(auto& matrix : matrices) // neighborLists automatically stored at the end
  {
    mesgHandler(IFilter::Message::Type::Progress, fmt::format("Now processing print object {} of {}", count, matrices.size()));
    std::vector<std::string> stringStore(matrix->getSize(), "UNINITIALIZED"); // 1 per matrix
    dataAlg.setRange(0, matrix->getSize());
    dataAlg.execute(AssembleHorizontalStringFromIndex(*matrix, stringStore, delimiter, componentsPerLine));
    stringStoreList.emplace_back(CreateStringMapFromVector(stringStore));
    count++;
  }

  mesgHandler(IFilter::Message::Type::Info, "Printing out");
  for(size_t i = 0; i < stringStoreList.size(); i++)
  {
    WriteOutWrapper(stringStoreList[i], outputStrm, false);
    if(i < (stringStoreList.size() - 1))
    {
      outputStrm << "\n"; // seperate neighborlists
    }
  }
};
} // namespace OStreamUtilities
} // namespace complex
