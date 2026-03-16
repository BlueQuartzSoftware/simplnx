#include "IdentifySample.hpp"

#include "IdentifySampleBFS.hpp"
#include "IdentifySampleCCL.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
IdentifySample::IdentifySample(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySample::~IdentifySample() noexcept = default;

// -----------------------------------------------------------------------------
Result<> IdentifySample::operator()()
{
  auto* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);

  return DispatchAlgorithm<IdentifySampleBFS, IdentifySampleCCL>({maskArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
