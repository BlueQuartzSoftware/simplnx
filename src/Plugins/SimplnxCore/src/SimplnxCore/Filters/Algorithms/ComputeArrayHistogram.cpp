#include "ComputeArrayHistogram.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <simplnx/DataStructure/INeighborList.hpp>
#include <tuple>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeArrayHistogram::ComputeArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeArrayHistogramInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeArrayHistogram::~ComputeArrayHistogram() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeArrayHistogram::updateProgress(const std::string& progressMessage)
{
  m_MessageHandler.sendInfoMessage(progressMessage);
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

  std::unique_ptr<MaskCompareUtilities::MaskCompare> mask = nullptr;
  if(m_InputValues->MaskPath.has_value())
  {
    mask = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskPath.value());
  }

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* inputData = m_DataStructure.getDataAs<IDataArray>(selectedArrayPaths[i]);
    auto* binRanges = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CreatedBinRangeDataPaths.at(i));
    auto& counts = m_DataStructure.getDataAs<DataArray<uint64>>(m_InputValues->CreatedHistogramCountsDataPaths.at(i))->getDataStoreRef();
    auto& mostPopulated = m_DataStructure.getDataAs<DataArray<uint64>>(m_InputValues->CreatedBinMostPopulatedDataPaths.at(i))->getDataStoreRef();

    if(m_InputValues->UserDefinedRange)
    {
      ExecuteParallelFunctor(HistogramUtilities::concurrent::InstantiateHistogramImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges,
                             std::make_pair(m_InputValues->MinRange, m_InputValues->MaxRange), m_ShouldCancel, numBins, counts, mostPopulated, mask, overflow);
    }
    else
    {
      ExecuteParallelFunctor(HistogramUtilities::concurrent::InstantiateHistogramImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges, m_ShouldCancel, numBins, counts,
                             mostPopulated, mask, overflow);
    }

    if(overflow > 0)
    {
      const std::string arrayName = inputData->getName();
      ComputeArrayHistogram::updateProgress(fmt::format("{} values not categorized into bin for array {}", overflow.load(), arrayName));
    }
  }

  taskRunner.wait();

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto* inputData = m_DataStructure.getDataAs<IDataArray>(selectedArrayPaths[i]);
    auto* binRanges = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CreatedBinRangeDataPaths.at(i));
    INeighborList* modalBinRanges = nullptr;
    if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
    {
      auto modalBinRangesPaths = m_InputValues->CreatedBinModalRangesDataPaths.value();
      modalBinRanges = m_DataStructure.getDataAs<INeighborList>(modalBinRangesPaths.at(i));
      ExecuteParallelFunctor<HistogramUtilities::concurrent::CalculateModalBinRangesImplFunctor, NoBooleanType>(
          HistogramUtilities::concurrent::CalculateModalBinRangesImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges, modalBinRanges, mask, m_ShouldCancel);
    }
  }

  taskRunner.wait();

  return {};
}
