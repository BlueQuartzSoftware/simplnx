#include "FillBadData.hpp"

#include "FillBadDataBFS.hpp"
#include "FillBadDataCCL.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> FillBadData::operator()()
{
  auto* featureIdsArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->featureIdsArrayPath);

  return DispatchAlgorithm<FillBadDataBFS, FillBadDataCCL>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
