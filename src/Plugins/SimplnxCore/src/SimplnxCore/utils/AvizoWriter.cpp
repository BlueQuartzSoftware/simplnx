#include "AvizoWriter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
AvizoWriter::AvizoWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AvizoWriter::~AvizoWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AvizoWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AvizoWriter::execute()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  FILE* outputFile = fopen(m_InputValues->OutputFile.string().c_str(), "wb");
  if(outputFile == nullptr)
  {
    return MakeErrorResult(-5830, fmt::format("Error creating output file {}", m_InputValues->OutputFile.string()));
  }

  Result<> headerResult = generateHeader(outputFile);
  if(headerResult.invalid())
  {
    return headerResult;
  }

  Result<> result = writeData(outputFile);
  if(result.invalid())
  {
    return result;
  }

  fclose(outputFile);

  return {};
}
