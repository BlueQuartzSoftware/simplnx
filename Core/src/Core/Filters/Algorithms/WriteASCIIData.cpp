#include "WriteASCIIData.hpp"

#include "Core/Filters/WriteASCIIDataFilter.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"

#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

using namespace complex;
namespace // define constant expression for buffer speed
{
constexpr int64 k_MaxComponents = 21000000;
} // namespace

namespace
{
template <typename T>
class WriteOutASCIIData
{
public:
  WriteOutASCIIData(WriteASCIIData* filter, complex::DataArray<T>& inputData, int32 maxValPerLine, char delimiter, std::string filePath)
  : m_Filter(filter)
  , m_InputData(inputData)
  , m_MaxValPerLine(maxValPerLine)
  , m_Delimiter(delimiter)
  , m_FilePath(filePath)
  {
  }
  ~WriteOutASCIIData() = default;

  void execute()
  {
    int32 recCount = 0;
    int32 count = 0;
    auto start = std::chrono::steady_clock::now();
    std::ofstream fout(m_FilePath, std::ios_base::app); // open precreated file in append mode
    std::stringstream stsm;
    if(std::is_same<T, float32>::value)
    {
      stsm.precision(std::numeric_limits<float>::digits10);
    }
    if(std::is_same<T, float64>::value)
    {
      stsm.precision(std::numeric_limits<double>::digits10);
    }
    size_t numTuples = m_InputData.getNumberOfTuples();
    size_t numComp = m_InputData.getNumberOfComponents();
    // size_t lastTup = 0;
    for(size_t tup = 0; tup < numTuples; tup++)
    {
      auto now = std::chrono::steady_clock::now();
      // send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        std::string message = fmt::format("Processing {}: {}% completed", m_InputData.getName(), static_cast<int32>(100 * (static_cast<float>(tup) / static_cast<float>(numTuples))));
        // std::string message = fmt::format("Processing {} completed", tup - lastTup);  // switch message if you need to calculate processing speeds and uncomment lastTup (lines 44 and 54)
        m_Filter->updateProgress(message);
        // lastTup = tup;
        start = std::chrono::steady_clock::now();
      }
      if(m_Filter->getCancel())
      {
        return;
      }
      for(size_t comp = 0; comp < numComp; comp++)
      {
        stsm << m_InputData[tup * numComp + comp];
        recCount++;
        if(comp != numComp - 1)
        {
          stsm << m_Delimiter;
        }
      }
      count++;
      if(count >= m_MaxValPerLine)
      {
        stsm << "\n";
        count = 0;
      }
      else
      {
        stsm << m_Delimiter;
      }
      if(recCount >= k_MaxComponents) // k_MaxComponents = 21,000,000 (essentially flushes buffer every 1 second)
      {
        fout << stsm.str();
        stsm.flush();
        recCount = 0;
      }
    }
    fout << stsm.str();
    stsm.flush();
    fout.close();
  }

private:
  WriteASCIIData* m_Filter = nullptr;
  complex::DataArray<T>& m_InputData;
  int32 m_MaxValPerLine = 1;
  char m_Delimiter = ' ';
  std::string m_FilePath = {};
};
} // namespace

// -----------------------------------------------------------------------------
WriteASCIIData::WriteASCIIData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteASCIIDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteASCIIData::~WriteASCIIData() noexcept = default;

// -----------------------------------------------------------------------------
void WriteASCIIData::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}
// -----------------------------------------------------------------------------
const std::atomic_bool& WriteASCIIData::getCancel()
{
  return m_ShouldCancel;
}
// -----------------------------------------------------------------------------
Result<> WriteASCIIData::operator()()
{
  const auto del = static_cast<WriteASCIIDataFilter::Delimiter>(m_InputValues->delimiter);
  char delimiter = ' ';
  switch(del)
  {
  case WriteASCIIDataFilter::Delimiter::Space: // 0
  {
    delimiter = ' ';
    break;
  }
  case WriteASCIIDataFilter::Delimiter::Semicolon: // 1
  {
    delimiter = ';';
    break;
  }
  case WriteASCIIDataFilter::Delimiter::Comma: // 2
  {
    delimiter = ',';
    break;
  }
  case WriteASCIIDataFilter::Delimiter::Colon: // 3
  {
    delimiter = ':';
    break;
  }
  case WriteASCIIDataFilter::Delimiter::Tab: // 4
  {
    delimiter = '\t';
    break;
  }
  default: {
    delimiter = ' ';
  }
  }
  int32 maxValPerLine = m_InputValues->maxValPerLine;
  if(maxValPerLine < 1)
  {
    MakeErrorResult(-11020, fmt::format("Characters per line must be greater than zero: {}", m_InputValues->maxValPerLine));
    return {};
  }

  auto selectedDataArrayPaths = m_InputValues->selectedDataArrayPaths;
  std::string filePath = "";

  if(static_cast<WriteASCIIDataFilter::OutputStyle>(m_InputValues->outputStyle) == WriteASCIIDataFilter::OutputStyle::SingleFile) // SingleFile = 1
  {
    int32 count = 0;
    for(const auto& selectedArrayPath : selectedDataArrayPaths)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler(fmt::format("Generating Header Number: {}", count + 1));
      auto& selectedArrayPtr = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPath);
      if(count == 0) // initialize file path
      {
        filePath = getFilePath(selectedArrayPtr);
      }
      std::ofstream fout(filePath, std::ios_base::app); // open precreated file in append mode
      fout << selectedArrayPtr.getName() << delimiter;
      count++;
      if(count == selectedDataArrayPaths.size())
      {
        fout << "\n";
      }
      fout.close();
    }
  }

  // begin printing arrays
  for(const auto& selectedArrayPath : selectedDataArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& oldSelectedArray = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPath);
    m_MessageHandler(fmt::format("Now Writing: {}", oldSelectedArray.getName()));
    if(static_cast<WriteASCIIDataFilter::OutputStyle>(m_InputValues->outputStyle) == WriteASCIIDataFilter::OutputStyle::MultipleFiles)
    {
      filePath = getFilePath(oldSelectedArray);
    }
    DataType type = oldSelectedArray.getDataType();
    switch(type)
    {
    case DataType::int8: {
      WriteOutASCIIData<int8>(this, m_DataStructure.getDataRefAs<DataArray<int8>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::int16: {
      WriteOutASCIIData<int16>(this, m_DataStructure.getDataRefAs<DataArray<int16>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::int32: {
      WriteOutASCIIData<int32>(this, m_DataStructure.getDataRefAs<DataArray<int32>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::int64: {
      WriteOutASCIIData<int64>(this, m_DataStructure.getDataRefAs<DataArray<int64>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::uint8: {
      WriteOutASCIIData<uint8>(this, m_DataStructure.getDataRefAs<DataArray<uint8>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::uint16: {
      WriteOutASCIIData<uint16>(this, m_DataStructure.getDataRefAs<DataArray<uint16>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::uint32: {
      WriteOutASCIIData<uint32>(this, m_DataStructure.getDataRefAs<DataArray<uint32>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::uint64: {
      WriteOutASCIIData<uint64>(this, m_DataStructure.getDataRefAs<DataArray<uint64>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::float32: {
      WriteOutASCIIData<float32>(this, m_DataStructure.getDataRefAs<DataArray<float32>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    case DataType::float64: {
      WriteOutASCIIData<float64>(this, m_DataStructure.getDataRefAs<DataArray<float64>>(selectedArrayPath), maxValPerLine, delimiter, filePath).execute();
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
std::string WriteASCIIData::getFilePath(const DataObject& selectedArrayPtr)
{
  std::string name = selectedArrayPtr.getName();
  std::string extension = m_InputValues->fileExtension;

  std::string fullPath = m_InputValues->outputPath.string() + "/" + name + extension;

  std::ofstream fout(fullPath, std::ofstream::out); // test name resolution and create file
  if(!fout.is_open())
  {
    MakeErrorResult(-11025, fmt::format("Error opening path {}", fullPath));
    return "";
  }
  fout.close();
  return fullPath;
}
