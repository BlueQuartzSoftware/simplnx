#include "WriteASCIIData.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// OutputStyle enum values matching WriteASCIIDataFilter::OutputStyle
constexpr uint64 k_MultipleFiles = 0;
constexpr uint64 k_SingleFile = 1;

// Includes enum values matching WriteASCIIDataFilter::Includes
constexpr uint64 k_Neither = 0;
constexpr uint64 k_Headers = 1;
constexpr uint64 k_ColumnIndex = 2;
constexpr uint64 k_Both = 3;
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
Result<> WriteASCIIData::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Writing ASCII data...");

  auto headerOption = m_InputValues->HeaderOptionIndex;
  bool includeHeaders = false;
  bool includeIndex = false;

  switch(headerOption)
  {
  case k_Neither: {
    includeHeaders = false;
    includeIndex = false;
    break;
  }
  case k_Headers: {
    includeHeaders = true;
    includeIndex = false;
    break;
  }
  case k_ColumnIndex: {
    includeHeaders = false;
    includeIndex = true;
    break;
  }
  case k_Both: {
    includeHeaders = true;
    includeIndex = true;
    break;
  }
  default: {
    includeHeaders = false;
    includeIndex = false;
  }
  }

  const std::string delimiter = OStreamUtilities::DelimiterToString(m_InputValues->DelimiterIndex);
  auto selectedDataArrayPaths = m_InputValues->InputDataArrayPaths;
  auto fileType = m_InputValues->OutputStyleIndex;

  if(fileType == k_SingleFile)
  {
    auto atomicFileResult = AtomicFile::Create(m_InputValues->OutputPath);
    if(atomicFileResult.invalid())
    {
      return ConvertResult(std::move(atomicFileResult));
    }
    AtomicFile atomicFile = std::move(atomicFileResult.value());

    auto outputPath = atomicFile.tempFilePath();
    // Make sure any directory path is also available as the user may have just typed
    // in a path without actually creating the full path
    Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(outputPath.parent_path());
    if(createDirectoriesResult.invalid())
    {
      return createDirectoriesResult;
    }

    // Scope file writer in code block to get around file lock on windows (enforce destructor order)
    {
      // Create the output file
      std::ofstream outStrm(outputPath, std::ios_base::out | std::ios_base::binary);
      if(!outStrm.is_open())
      {
        return MakeErrorResult(-11021, fmt::format("Unable to create output file {}", outputPath.string()));
      }

      OStreamUtilities::PrintDataSetsToSingleFile(outStrm, selectedDataArrayPaths, m_DataStructure, m_MessageHandler, m_ShouldCancel, delimiter, includeIndex, includeHeaders);
    }

    Result<> commitResult = atomicFile.commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  if(fileType == k_MultipleFiles)
  {
    auto directoryPath = m_InputValues->OutputDir;
    auto fileExtension = m_InputValues->FileExtension;
    auto maxTuplePerLine = m_InputValues->MaxValPerLine;

    if(!fs::exists(directoryPath))
    {
      std::error_code err;
      if(!fs::create_directories(directoryPath, err))
      {
        return MakeErrorResult(-11022,
                               fmt::format("Unable to create output directory '{}'. Operating system returned error '{}' with message\n    '{}'", directoryPath.string(), err.value(), err.message()));
      }
    }
    return OStreamUtilities::PrintDataSetsToMultipleFiles(selectedDataArrayPaths, m_DataStructure, directoryPath.string(), m_MessageHandler, m_ShouldCancel, fileExtension, false, delimiter,
                                                          includeIndex, includeHeaders, maxTuplePerLine);
  }

  return {};
}
