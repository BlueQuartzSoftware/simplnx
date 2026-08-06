#include "ComputeArrayHistogramByFeature.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramByFeatureFilter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Utilities/HistogramUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <tuple>

using namespace nx::core;

/**
 * @class GenerateHistogramImpl
 * @brief This class is a pseudo-wrapper for the serial::GenerateHistogram, the reason for this class' existence is to hold/define ownership of objects in each thread
 * @tparam Type this the end type of the function in that the container and data values are of this type
 * @tparam SizeType this is the scalar type of the bin counts container
 */
template <typename Type, std::integral SizeType>
class GenerateFeatureHistogramImpl
{
public:
  /**
   * @function constructor
   * @brief This constructor requires a defined range and creates the object
   * @param inputStore this is the AbstractDataStore holding the data that will be binned
   * @param binRangesStore this is the AbstractDataStore that the ranges will be loaded into.
   * @param rangeMinMax this is assumed to be the inclusive minimum value and exclusive maximum value for the overall histogram bins. FORMAT: [minimum, maximum)
   * @param shouldCancel this is an atomic value that will determine whether execution ends early
   * @param numBins this is the total number of bin ranges being calculated and by extension the indexing value for the ranges
   * @param histogramStore this is the AbstractDataStore that will hold the counts for each bin (variable type sizing)
   * @param overflow this is an atomic counter for the number of values that fall outside the bin range
   */
  GenerateFeatureHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, NeighborList<Type>* modalBinRangesList,
                               const AbstractDataStore<int32>& featureIdsStore, float64 histMin, float64 histMax, bool histFullRange, const std::atomic_bool& shouldCancel, const int32 numBins,
                               AbstractDataStore<SizeType>& histogramStore, AbstractDataStore<SizeType>& mostPopulatedStore, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask,
                               std::atomic<usize>& overflow, ComputeArrayHistogramByFeature* filter)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRangesList(modalBinRangesList)
  , m_HistMin(histMin)
  , m_HistMax(histMax)
  , m_HistFullRange(histFullRange)
  , m_HistogramStore(histogramStore)
  , m_MostPopulatedStore(mostPopulatedStore)
  , m_FeatureIdsStore(featureIdsStore)
  , m_Mask(mask)
  , m_Overflow(overflow)
  , m_Filter(filter)
  {
  }

  GenerateFeatureHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, const AbstractDataStore<int32>& featureIdsStore, float64 histMin, float64 histMax,
                               bool histFullRange, const std::atomic_bool& shouldCancel, const int32 numBins, AbstractDataStore<SizeType>& histogramStore,
                               AbstractDataStore<SizeType>& mostPopulatedStore, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask, std::atomic<usize>& overflow,
                               ComputeArrayHistogramByFeature* filter)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_ModalBinRangesList(nullptr)
  , m_HistMin(histMin)
  , m_HistMax(histMax)
  , m_HistFullRange(histFullRange)
  , m_HistogramStore(histogramStore)
  , m_MostPopulatedStore(mostPopulatedStore)
  , m_FeatureIdsStore(featureIdsStore)
  , m_Mask(mask)
  , m_Overflow(overflow)
  , m_Filter(filter)
  {
  }

  ~GenerateFeatureHistogramImpl() = default;

  /**
   * @function operator()
   * @brief This function serves as the execute method
   */
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

  void compute(usize start, usize end) const
  {

    const usize numTuples = m_FeatureIdsStore.getNumberOfTuples();
    const usize numCurrentFeatures = end - start;

    auto [length, min, max, summation, modalMaps] = HistogramUtilities::concurrent::CalculateFeatureHasDataStats(m_InputStore, m_FeatureIdsStore, start, end, m_Mask, {}, m_ShouldCancel);
    if(m_ShouldCancel)
    {
      return;
    }

    usize progressIncrement = numCurrentFeatures / 100;
    usize progressCount = 0;
    for(usize j = start; j < end; j++)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const usize localFeatureIndex = j - start;

      std::vector<Type> ranges(m_NumBins * 2);
      std::vector<uint64> histogram(m_NumBins, 0);
      if(length[localFeatureIndex] > 0)
      {
        auto histMin = static_cast<Type>(m_HistMin);
        auto histMax = static_cast<Type>(m_HistMax);

        if(m_HistFullRange)
        {
          histMin = min[localFeatureIndex];
          histMax = max[localFeatureIndex] + static_cast<Type>(1.0);
        }

        HistogramUtilities::serial::FillBinRanges(ranges, std::make_pair(histMin, histMax), m_NumBins);

        const float32 increment = HistogramUtilities::serial::CalculateIncrement(histMin, histMax, m_NumBins);
        if(std::fabs(increment) < 1E-10)
        {
          histogram[0] = length[localFeatureIndex];
        }
        else
        {
          for(usize i = 0; i < numTuples; ++i)
          {
            if(m_ShouldCancel)
            {
              return;
            }
            if(m_Mask != nullptr && !m_Mask->isTrue(i))
            {
              continue;
            }
            if(m_FeatureIdsStore[i] != static_cast<int32>(j))
            {
              continue;
            }
            const Type value = m_InputStore[i];
            const auto bin = static_cast<int32>(HistogramUtilities::serial::CalculateBin(value, static_cast<Type>(histMin), increment)); // find bin for this input array value
            if((bin >= 0) && (bin < m_NumBins))                                                                                          // make certain bin is in range
            {
              histogram[bin]++; // increment histogram element corresponding to this input array value
            }
            else
            {
              m_Overflow++;
            }
          } // end of numTuples loop
        } // end of increment else

        // Bool breaks neighbor lists; if we have made it here we know m_ModalBinRangesList is a nullptr
        if constexpr(!std::is_same_v<Type, bool>)
        {
          if(m_ModalBinRangesList != nullptr)
          {
            if(std::fabs(increment) < 1E-10)
            {
              m_ModalBinRangesList->addEntry(j, start);
              m_ModalBinRangesList->addEntry(j, end);
            }
            else if(!modalMaps[localFeatureIndex].empty())
            {
              // Find the maximum occurrence
              auto pr = std::max_element(modalMaps[localFeatureIndex].begin(), modalMaps[localFeatureIndex].end(), [](const auto& x, const auto& y) { return x.second < y.second; });
              int maxCount = pr->second;

              // Store all values that have this maximum occurrence under the proper feature id
              for(const auto& modalPair : modalMaps[localFeatureIndex])
              {
                if(modalPair.second == maxCount)
                {
                  const Type mode = modalPair.first;
                  const auto modalBin = HistogramUtilities::serial::CalculateBin(mode, histMin, increment);
                  if((modalBin >= 0) && (modalBin < m_NumBins)) // make certain bin is in range
                  {
                    m_ModalBinRangesList->addEntry(j, ranges[modalBin]);
                    m_ModalBinRangesList->addEntry(j, ranges[modalBin + 1]);
                  }
                }
              }
            }
          }
        }

      } // end of length if

      for(usize k = 0; k < histogram.size(); k++)
      {
        m_HistogramStore.setComponent(j, k, histogram[k]);
      }
      for(usize k = 0; k < ranges.size(); k++)
      {
        m_BinRangesStore.setComponent(j, k, ranges[k]);
      }

      auto maxElementIt = std::max_element(histogram.begin(), histogram.end());
      uint64 index = std::distance(histogram.begin(), maxElementIt);
      m_MostPopulatedStore.setComponent(j, 0, index);
      m_MostPopulatedStore.setComponent(j, 1, histogram[index]);

      progressCount++;
      if(progressCount > progressIncrement)
      {
        m_Filter->sendThreadSafeProgressMessage(progressCount);
        progressCount = 0;
      }
    }

    // Send one at the end so that the progress is communicated properly
    m_Filter->sendThreadSafeProgressMessage(progressCount);
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  float64 m_HistMin;
  float64 m_HistMax;
  bool m_HistFullRange;
  int32 m_NumBins;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  const AbstractDataStore<Type>& m_InputStore;
  const AbstractDataStore<int32>& m_FeatureIdsStore;
  AbstractDataStore<SizeType>& m_HistogramStore;
  AbstractDataStore<Type>& m_BinRangesStore;
  AbstractDataStore<uint64>& m_MostPopulatedStore;
  NeighborList<Type>* m_ModalBinRangesList;
  std::atomic<usize>& m_Overflow;
  ComputeArrayHistogramByFeature* m_Filter = nullptr;
};

/**
 * @class InstantiateHistogramImplFunctor
 * @brief This is a compatibility functor that leverages existing typecasting functions to create the appropriately typed GenerateHistogramImpl() cleanly.
 * Designed for compatibility with the existing parallel execution classes.
 */
struct InstantiateHistogramByFeatureImplFunctor
{
  template <typename T, class... ArgsT>
  auto operator()(INeighborList* modalBinRangesNL, const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args)
  {
    return GenerateFeatureHistogramImpl(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), binRangesArray->template getIDataStoreRefAs<AbstractDataStore<T>>(),
                                        dynamic_cast<NeighborList<T>*>(modalBinRangesNL), std::forward<ArgsT>(args)...);
  }
  template <typename T, class... ArgsT>
  auto operator()(const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args)
  {
    return GenerateFeatureHistogramImpl(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), binRangesArray->template getIDataStoreRefAs<AbstractDataStore<T>>(),
                                        std::forward<ArgsT>(args)...);
  }
};

// -----------------------------------------------------------------------------
ComputeArrayHistogramByFeature::ComputeArrayHistogramByFeature(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                               ComputeArrayHistogramByFeatureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(msgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeArrayHistogramByFeature::~ComputeArrayHistogramByFeature() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeArrayHistogramByFeature::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementCount(counter);
}

// -----------------------------------------------------------------------------
Result<> ComputeArrayHistogramByFeature::operator()()
{
  const int32 numBins = m_InputValues->NumberOfBins;
  const std::vector<DataPath> selectedArrayPaths = m_InputValues->SelectedArrayPaths;
  std::atomic<usize> overflow = 0;

  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIdsArray.getDataStoreRef();

  usize numFeatures = *std::max_element(featureIdsStore.begin(), featureIdsStore.end()) + 1;


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

    binRanges->resizeTuples({numFeatures});
    counts.resizeTuples({numFeatures});
    mostPopulated.resizeTuples({numFeatures});

    std::unique_ptr<MaskCompareUtilities::MaskCompare> mask = nullptr;
    if(m_InputValues->UseMask)
    {
      mask = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    }

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numFeatures);

    bool histFullRange = !m_InputValues->UserDefinedRange;

    m_Throttle.reset(numFeatures, "Calculating feature histograms");

    if(m_InputValues->CreatedBinModalRangesDataPaths.has_value())
    {
      std::vector<DataPath> modalBinRangesPaths = m_InputValues->CreatedBinModalRangesDataPaths.value();
      auto* modalBinRanges = m_DataStructure.getDataAs<INeighborList>(modalBinRangesPaths.at(i));
      modalBinRanges->resizeTuples({numFeatures});
      ExecuteParallelFunctor<InstantiateHistogramByFeatureImplFunctor, NoBooleanType>(InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), dataAlg, modalBinRanges, inputData,
                                                                                      binRanges, featureIdsStore, m_InputValues->MinRange, m_InputValues->MaxRange, histFullRange, m_ShouldCancel,
                                                                                      numBins, counts, mostPopulated, mask, overflow, this);
    }
    else
    {
      ExecuteParallelFunctor(InstantiateHistogramByFeatureImplFunctor{}, inputData->getDataType(), dataAlg, inputData, binRanges, featureIdsStore, m_InputValues->MinRange, m_InputValues->MaxRange,
                             histFullRange, m_ShouldCancel, numBins, counts, mostPopulated, mask, overflow, this);
    }

    m_MessageHandler.sendInfoMessage(fmt::format("Calculated {} feature histograms!", numFeatures));

    if(overflow > 0)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("{} values not categorized into bin for array {}", overflow.load(), inputData->getName()));
    }
  }

  return {};
}
