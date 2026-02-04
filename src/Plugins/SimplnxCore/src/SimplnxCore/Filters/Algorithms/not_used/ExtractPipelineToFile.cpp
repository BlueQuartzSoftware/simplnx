#include "ExtractPipelineToFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ExtractPipelineToFile::ExtractPipelineToFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ExtractPipelineToFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractPipelineToFile::~ExtractPipelineToFile() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ExtractPipelineToFile::operator()()
{

  return {};
}
