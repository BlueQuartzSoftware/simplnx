#include "WriteBinaryData.hpp"

#include "Core/Filters/WriteBinaryDataFilter.hpp"
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
class WriteOutBinaryData
{
public:
  WriteOutBinaryData(WriteBinaryData* filter, endian endianess, const uint8 bytes, complex::DataArray<T>& inputData, std::string filePath)
  : m_Filter(filter)
  , m_Endianess(endianess)
  , m_Bytes(bytes)
  , m_InputData(inputData)
  , m_FilePath(filePath)
  {
  }
  ~WriteOutBinaryData() = default;

  // put print function here
  void execute()
  {
    if(endian::native != m_Endianess) // if requested endianess is not native then byteswap
    {
      m_InputData.byteSwapElements();
    }

    int32 recCount = 0;
    auto start = std::chrono::steady_clock::now();
    std::ofstream fout(m_FilePath, std::ofstream::out | std::ios_base::binary);
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
    // load data into contents byte by byte
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
      }
      if(recCount >= k_MaxComponents) // k_MaxComponents = 21,000,000 (essentially flushes buffer every 1 second)
      {
        fout.write(stsm.str().c_str(), stsm.str().length());
        stsm.flush();
        recCount = 0;
      }
    }
    fout.write(stsm.str().c_str(), stsm.str().length());
    stsm.flush();
    fout.close();
  }

private:
  WriteBinaryData* m_Filter = nullptr;
  endian m_Endianess = endian::little;
  const uint8 m_Bytes = 1;
  complex::DataArray<T>& m_InputData;
  std::string m_FilePath = {};
};
} // namespace

// -----------------------------------------------------------------------------
WriteBinaryData::WriteBinaryData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteBinaryDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteBinaryData::~WriteBinaryData() noexcept = default;

// -----------------------------------------------------------------------------
void WriteBinaryData::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}
// -----------------------------------------------------------------------------
const std::atomic_bool& WriteBinaryData::getCancel()
{
  return m_ShouldCancel;
}
// -----------------------------------------------------------------------------
Result<> WriteBinaryData::operator()()
{
  const auto endianess = static_cast<endian>(static_cast<uint8>(m_InputValues->endianess));
  auto selectedDataArrayPaths = m_InputValues->selectedDataArrayPaths;
  for(const auto& selectedArrayPath : selectedDataArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& oldSelectedArray = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPath);
    m_MessageHandler(fmt::format("Now Writing: {}", oldSelectedArray.getName()));
    DataType type = oldSelectedArray.getDataType();
    switch(type)
    {
    case DataType::int8: {
      WriteOutBinaryData<int8>(this, endianess, 1, m_DataStructure.getDataRefAs<DataArray<int8>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::int16: {
      WriteOutBinaryData<int16>(this, endianess, 2, m_DataStructure.getDataRefAs<DataArray<int16>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::int32: {
      WriteOutBinaryData<int32>(this, endianess, 4, m_DataStructure.getDataRefAs<DataArray<int32>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::int64: {
      WriteOutBinaryData<int64>(this, endianess, 8, m_DataStructure.getDataRefAs<DataArray<int64>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::uint8: {
      WriteOutBinaryData<uint8>(this, endianess, 1, m_DataStructure.getDataRefAs<DataArray<uint8>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::uint16: {
      WriteOutBinaryData<uint16>(this, endianess, 2, m_DataStructure.getDataRefAs<DataArray<uint16>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::uint32: {
      WriteOutBinaryData<uint32>(this, endianess, 4, m_DataStructure.getDataRefAs<DataArray<uint32>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::uint64: {
      WriteOutBinaryData<uint64>(this, endianess, 8, m_DataStructure.getDataRefAs<DataArray<uint64>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::float32: {
      WriteOutBinaryData<float32>(this, endianess, 4, m_DataStructure.getDataRefAs<DataArray<float32>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
      break;
    }
    case DataType::float64: {
      WriteOutBinaryData<float64>(this, endianess, 8, m_DataStructure.getDataRefAs<DataArray<float64>>(selectedArrayPath), getFilePath(oldSelectedArray)).execute();
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
std::string WriteBinaryData::getFilePath(const DataObject& selectedArrayPtr) // update for binary endianess
{
  std::string name = selectedArrayPtr.getName();
  std::string extension = m_InputValues->fileExtension;

  std::string fullPath = m_InputValues->outputPath.string() + "/" + name + extension;

  std::ofstream fout(fullPath, std::ofstream::out | std::ios_base::binary); // test name resolution and create file
  if(!fout.is_open())
  {
    MakeErrorResult(-11025, fmt::format("Error opening path {}", fullPath));
    return "";
  }
  fout.close();
  return fullPath;
}
