#include "AbaqusSurfaceMeshWriter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
AbaqusSurfaceMeshWriter::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AbaqusSurfaceMeshWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AbaqusSurfaceMeshWriter::~AbaqusSurfaceMeshWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AbaqusSurfaceMeshWriter::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AbaqusSurfaceMeshWriter::operator()()
{

  return {};
}
