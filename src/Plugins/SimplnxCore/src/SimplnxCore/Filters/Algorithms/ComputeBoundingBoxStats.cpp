#include "ComputeBoundingBoxStats.hpp"

#include "ComputeBoundingBoxStatsDirect.hpp"
#include "ComputeBoundingBoxStatsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeBoundingBoxStats::ComputeBoundingBoxStats(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeBoundingBoxStatsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeBoundingBoxStats::~ComputeBoundingBoxStats() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeBoundingBoxStats::operator()()
{
  auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputPath);
  auto& unifiedBounds = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedPath);
  if(inputArray.getDataType() == DataType::boolean)
  {
    return MakeErrorResult(-98500, "Boolean arrays cannot be used as inputs to this filter.");
  }

  return DispatchAlgorithm<ComputeBoundingBoxStatsDirect, ComputeBoundingBoxStatsScanline>({&inputArray, &unifiedBounds}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
