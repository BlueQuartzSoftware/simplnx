#include "ComputeNeighborListStatistics.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Math/StatisticsCalculations.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

namespace
{
constexpr int64 k_BoolTypeNeighborList = -6802;
constexpr int64 k_EmptyNeighborList = -6803;

template <typename T>
class ComputeNeighborListStatisticsImpl
{
public:
  using NeighborListType = NeighborList<T>;
  using DataArrayType = DataArray<T>;
  using StoreType = AbstractDataStore<T>;

  ComputeNeighborListStatisticsImpl(ComputeNeighborListStatistics* filter, const INeighborList& source, bool length, bool min, bool max, bool mean, bool median, bool stdDeviation, bool summation,
                                    std::vector<IDataArray*>& arrays, const std::atomic_bool& shouldCancel)
  : m_Filter(filter)
  , m_ShouldCancel(shouldCancel)
  , m_Source(source)
  , m_Length(length)
  , m_Min(min)
  , m_Max(max)
  , m_Mean(mean)
  , m_Median(median)
  , m_StdDeviation(stdDeviation)
  , m_Summation(summation)
  , m_Arrays(arrays)
  {
  }

  virtual ~ComputeNeighborListStatisticsImpl() = default;

  void compute(usize start, usize end) const
  {
    auto* array0Ptr = m_Length ? m_Arrays[0]->template getIDataStoreAs<AbstractDataStore<uint64>>() : nullptr;
    if(m_Length && array0Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    auto* array1Ptr = m_Min ? m_Arrays[1]->template getIDataStoreAs<StoreType>() : nullptr;
    if(m_Min && array1Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Min' array to needed type. Check input array selection.");
    }
    auto* array2Ptr = m_Max ? m_Arrays[2]->template getIDataStoreAs<StoreType>() : nullptr;
    if(m_Max && array2Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Max' array to needed type. Check input array selection.");
    }
    auto* array3Ptr = m_Mean ? m_Arrays[3]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Mean && array3Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
    }
    auto* array4Ptr = m_Median ? m_Arrays[4]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Median && array4Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    auto* array5Ptr = m_StdDeviation ? m_Arrays[5]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_StdDeviation && array5Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    auto* array6Ptr = m_Summation ? m_Arrays[6]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Summation && array6Ptr == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Summation' array to needed type. Check input array selection.");
    }

    const auto& sourceList = dynamic_cast<const NeighborListType&>(m_Source);

    for(usize i = start; i < end; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const std::vector<T>& tmpList = sourceList.at(i);

      if(m_Length)
      {
        auto val = static_cast<int64_t>(tmpList.size());
        array0Ptr->setValue(i, val);
      }
      if(m_Min)
      {
        T val = statistics_calculations::FindMin(tmpList);
        array1Ptr->setValue(i, val);
      }
      if(m_Max)
      {
        T val = statistics_calculations::FindMax(tmpList);
        array2Ptr->setValue(i, val);
      }
      if(m_Mean)
      {
        float val = statistics_calculations::FindMean(tmpList);
        array3Ptr->setValue(i, val);
      }
      if(m_Median)
      {
        float val = statistics_calculations::FindMedian(tmpList);
        array4Ptr->setValue(i, val);
      }
      if(m_StdDeviation)
      {
        float val = statistics_calculations::findStdDeviation(tmpList);
        array5Ptr->setValue(i, val);
      }
      if(m_Summation)
      {
        float val = statistics_calculations::FindSummation(tmpList);
        array6Ptr->setValue(i, val);
      }

      m_Filter->sendThreadSafeProgressMessage(1);
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  ComputeNeighborListStatistics* m_Filter = nullptr;
  const std::atomic_bool& m_ShouldCancel;

  const INeighborList& m_Source;
  bool m_Length = false;
  bool m_Min = false;
  bool m_Max = false;
  bool m_Mean = false;
  bool m_Median = false;
  bool m_StdDeviation = false;
  bool m_Summation = false;

  std::vector<IDataArray*>& m_Arrays;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeNeighborListStatistics::ComputeNeighborListStatistics(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                             ComputeNeighborListStatisticsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(msgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeNeighborListStatistics::~ComputeNeighborListStatistics() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeNeighborListStatistics::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessageMutex);
  m_Throttle.incrementPercent(counter);
}

// -----------------------------------------------------------------------------
Result<> ComputeNeighborListStatistics::operator()()
{
  const auto& inputINeighborList = m_DataStructure.getDataRefAs<INeighborList>(m_InputValues->target_neighbor_list_path);

  DataType type = inputINeighborList.getDataType();
  if(type == DataType::boolean)
  {
    return MakeErrorResult(k_BoolTypeNeighborList, fmt::format("ComputeNeighborListStatisticsFilter::NeighborList {} was of type boolean, and thus cannot be processed", inputINeighborList.getName()));
  }

  usize numTuples = inputINeighborList.getNumberOfTuples();
  if(numTuples == 0)
  {
    return MakeErrorResult(k_EmptyNeighborList, fmt::format("ComputeNeighborListStatisticsFilter::NeighborList {} was empty", inputINeighborList.getName()));
  }

  std::vector<IDataArray*> arrays(7, nullptr);

  if(m_InputValues->find_length)
  {
    arrays[0] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->length_path);
  }
  if(m_InputValues->find_min)
  {
    arrays[1] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->min_path);
  }
  if(m_InputValues->find_max)
  {
    arrays[2] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->max_path);
  }
  if(m_InputValues->find_mean)
  {
    arrays[3] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->mean_path);
  }
  if(m_InputValues->find_median)
  {
    arrays[4] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->median_path);
  }
  if(m_InputValues->find_std_deviation)
  {
    arrays[5] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->std_deviation_path);
  }
  if(m_InputValues->find_summation)
  {
    arrays[6] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->summation_path);
  }

  m_Throttle.reset(numTuples, "Finding Statistics");

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numTuples);
  ExecuteParallelFunction<ComputeNeighborListStatisticsImpl, NoBooleanType>(type, dataAlg, this, inputINeighborList, m_InputValues->find_length, m_InputValues->find_min, m_InputValues->find_max,
                                                                            m_InputValues->find_mean, m_InputValues->find_median, m_InputValues->find_std_deviation, m_InputValues->find_summation, arrays,
                                                                            m_ShouldCancel);

  return {};
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeNeighborListStatistics::getCancel()
{
  return m_ShouldCancel;
}
