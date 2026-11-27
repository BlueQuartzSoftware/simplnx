#include "WriteBinaryData.hpp"

#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

WriteBinaryData::WriteBinaryData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteBinaryDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

Result<> WriteBinaryData::operator()()
{
  const auto endianess = static_cast<endian>(m_InputValues->endian_index);
  auto selectedDataArrayPaths = m_InputValues->input_data_array_paths;
  auto dirPath = m_InputValues->output_path;
  // Create the selected output directory before opening array files.
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(dirPath);
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  if(!fs::is_directory(dirPath))
  {
    return MakeErrorResult(-23430, fmt::format("{}({}): Function {}: Error. OutputPath must be a directory. '{}'", "WriteBinaryData::operator()", __FILE__, __LINE__, dirPath.string()));
  }
  const bool swapEndian = endian::native != endianess;
  // Swap byte order only in the shared writer's page buffer.
  return o_stream_utilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, m_DataStructure, dirPath.string(), m_MessageHandler, m_ShouldCancel, m_InputValues->file_extension, true, "", false,
                                                          false, 0, swapEndian);
}
