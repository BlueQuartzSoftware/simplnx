#include "ComputeArrayHistogram.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <tuple>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeArrayHistogram::ComputeArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeArrayHistogramInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeArrayHistogram::~ComputeArrayHistogram() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeArrayHistogram::updateProgress(const std::string& progressMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progressMessage});
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeArrayHistogram::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeArrayHistogram::operator()()
{
  const int32 numBins = m_InputValues->NumberOfBins;
  const std::vector<DataPath> selectedArrayPaths = m_InputValues->SelectedArrayPaths;

  ParallelTaskAlgorithm taskRunner;

  std::atomic<usize> overflow = 0;

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* inputData = m_DataStructure.getDataAs<IDataArray>(selectedArrayPaths[i]);
    auto& counts = m_DataStructure.getDataAs<DataArray<uint64>>(m_InputValues->CreatedHistogramCountsDataPaths.at(i))->getDataStoreRef();
    auto* binRanges = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CreatedBinRangeDataPaths.at(i));
    Result<> result = {};
    if(m_InputValues->UserDefinedRange)
    {
      ExecuteParallelFunctor(HistogramUtilities::concurrent::InstantiateHistogramImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges,
                             std::make_pair(m_InputValues->MinRange, m_InputValues->MaxRange), m_ShouldCancel, numBins, counts, overflow);
    }
    else
    {
      ExecuteParallelFunctor(HistogramUtilities::concurrent::InstantiateHistogramImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges, m_ShouldCancel, numBins, counts, overflow);
    }

    if(result.invalid())
    {
      return result;
    }

    if(overflow > 0)
    {
      const std::string arrayName = inputData->getName();
      ComputeArrayHistogram::updateProgress(fmt::format("{} values not categorized into bin for array {}", overflow.load(), arrayName));
    }
  }

  taskRunner.wait();

  return {};
}
