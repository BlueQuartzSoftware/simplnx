#include "ComputeArrayHistogramByFeature.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <simplnx/DataStructure/INeighborList.hpp>
#include <tuple>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeArrayHistogramByFeature::ComputeArrayHistogramByFeature(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                               ComputeArrayHistogramByFeatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeArrayHistogramByFeature::~ComputeArrayHistogramByFeature() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeArrayHistogramByFeature::updateProgress(const std::string& progressMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progressMessage});
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeArrayHistogramByFeature::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeArrayHistogramByFeature::operator()()
{
  const int32 numBins = m_InputValues->NumberOfBins;
  const std::vector<DataPath> selectedArrayPaths = m_InputValues->SelectedArrayPaths;

  ParallelTaskAlgorithm taskRunner;

  std::atomic<usize> overflow = 0;

  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStore = featureIdsArray.getDataStoreRef();

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

    std::unique_ptr<MaskCompare> mask = nullptr;
    if(m_InputValues->UseMask)
    {
      mask = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    }

    //    GenerateFeatureHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, std::optional<NeighborList<Type>&> modalBinRangesDataStore, float64 histMin,
    //                                 float64 histMax, bool histFullRange, const std::atomic_bool& shouldCancel, const int32 numBins, const AbstractDataStore<int32>& featureIdsStore,
    //                                 AbstractDataStore<SizeType>& histogramStore, AbstractDataStore<SizeType>& mostPopulatedStore, const std::unique_ptr<MaskCompare>& mask, std::atomic<usize>&
    //                                 overflow, const IFilter::MessageHandler& msgHandler)

    if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
    {
      auto modalBinRangesPaths = m_InputValues->CreatedBinModalRangesDataPaths.value();
      INeighborList* modalBinRanges = m_DataStructure.getDataAs<INeighborList>(modalBinRangesPaths.at(i));
      ExecuteParallelFunctor<HistogramUtilities::concurrent::InstantiateHistogramByFeatureImplFunctor, NoBooleanType>(
          HistogramUtilities::concurrent::InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges, modalBinRanges, m_InputValues->MinRange,
          m_InputValues->MaxRange, m_InputValues->UserDefinedRange, m_ShouldCancel, numBins, featureIdsStore, counts, mostPopulated, mask, overflow, m_MessageHandler);
    }
    else
    {
      ExecuteParallelFunctor(HistogramUtilities::concurrent::InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), taskRunner, inputData, binRanges, nullptr, m_InputValues->MinRange,
                             m_InputValues->MaxRange, m_InputValues->UserDefinedRange, m_ShouldCancel, numBins, featureIdsStore, counts, mostPopulated, mask, overflow, m_MessageHandler);
    }

    if(overflow > 0)
    {
      const std::string arrayName = inputData->getName();
      ComputeArrayHistogramByFeature::updateProgress(fmt::format("{} values not categorized into bin for array {}", overflow.load(), arrayName));
    }
  }

  taskRunner.wait();

  return {};
}
