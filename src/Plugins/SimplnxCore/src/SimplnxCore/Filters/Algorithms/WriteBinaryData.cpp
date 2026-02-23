#include "WriteBinaryData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
struct ByteSwapArray
{
  template <typename ScalarType>
  Result<> operator()(IDataArray* inputDataArray)
  {
    if constexpr(std::is_same_v<ScalarType, bool> || std::is_same_v<ScalarType, uint8> || std::is_same_v<ScalarType, int8>) // byte-swap unnecessary bail early
    {
      return {};
    }
    auto* dataArray = dynamic_cast<DataArray<ScalarType>*>(inputDataArray);
    dataArray->byteSwapElements();
    return {};
  }
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
Result<> WriteBinaryData::operator()()
{
  const auto endianess = static_cast<endian>(m_InputValues->EndianIndex);
  auto selectedDataArrayPaths = m_InputValues->InputDataArrayPaths;
  for(const auto& selectedArrayPath : selectedDataArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(endian::native != endianess) // if requested endianess is not native then byteswap
    {
      auto* oldSelectedArray = m_DataStructure.getDataAs<IDataArray>(selectedArrayPath);
      ExecuteDataFunction(ByteSwapArray{}, oldSelectedArray->getDataType(), oldSelectedArray);
    }
  }

  auto dirPath = m_InputValues->OutputPath;
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(dirPath);
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  if(!fs::is_directory(dirPath))
  {
    return MakeErrorResult(-23430, fmt::format("{}({}): Function {}: Error. OutputPath must be a directory. '{}'", "WriteBinaryData::operator()", __FILE__, __LINE__, dirPath.string()));
  }
  OStreamUtilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, m_DataStructure, dirPath.string(), m_MessageHandler, m_ShouldCancel, m_InputValues->FileExtension, true);
  return {};
}
