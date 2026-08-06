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
    auto* array0 = m_Length ? m_Arrays[0]->template getIDataStoreAs<AbstractDataStore<uint64>>() : nullptr;
    if(m_Length && array0 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Length' array to needed type. Check input array selection.");
    }
    auto* array1 = m_Min ? m_Arrays[1]->template getIDataStoreAs<StoreType>() : nullptr;
    if(m_Min && array1 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Min' array to needed type. Check input array selection.");
    }
    auto* array2 = m_Max ? m_Arrays[2]->template getIDataStoreAs<StoreType>() : nullptr;
    if(m_Max && array2 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Max' array to needed type. Check input array selection.");
    }
    auto* array3 = m_Mean ? m_Arrays[3]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Mean && array3 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Mean' array to needed type. Check input array selection.");
    }
    auto* array4 = m_Median ? m_Arrays[4]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Median && array4 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'Median' array to needed type. Check input array selection.");
    }
    auto* array5 = m_StdDeviation ? m_Arrays[5]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_StdDeviation && array5 == nullptr)
    {
      throw std::invalid_argument("ComputeNeighborListStatisticsFilter::compute() could not dynamic_cast 'StdDev' array to needed type. Check input array selection.");
    }
    auto* array6 = m_Summation ? m_Arrays[6]->template getIDataStoreAs<AbstractDataStore<float32>>() : nullptr;
    if(m_Summation && array6 == nullptr)
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
        array0->setValue(i, val);
      }
      if(m_Min)
      {
        T val = StatisticsCalculations::findMin(tmpList);
        array1->setValue(i, val);
      }
      if(m_Max)
      {
        T val = StatisticsCalculations::findMax(tmpList);
        array2->setValue(i, val);
      }
      if(m_Mean)
      {
        float val = StatisticsCalculations::findMean(tmpList);
        array3->setValue(i, val);
      }
      if(m_Median)
      {
        float val = StatisticsCalculations::findMedian(tmpList);
        array4->setValue(i, val);
      }
      if(m_StdDeviation)
      {
        float val = StatisticsCalculations::findStdDeviation(tmpList);
        array5->setValue(i, val);
      }
      if(m_Summation)
      {
        float val = StatisticsCalculations::findSummation(tmpList);
        array6->setValue(i, val);
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
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(counter);
}

// -----------------------------------------------------------------------------
Result<> ComputeNeighborListStatistics::operator()()
{
  const auto& inputINeighborList = m_DataStructure.getDataRefAs<INeighborList>(m_InputValues->TargetNeighborListPath);

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

  if(m_InputValues->FindLength)
  {
    arrays[0] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->LengthPath);
  }
  if(m_InputValues->FindMin)
  {
    arrays[1] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MinPath);
  }
  if(m_InputValues->FindMax)
  {
    arrays[2] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaxPath);
  }
  if(m_InputValues->FindMean)
  {
    arrays[3] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MeanPath);
  }
  if(m_InputValues->FindMedian)
  {
    arrays[4] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MedianPath);
  }
  if(m_InputValues->FindStdDeviation)
  {
    arrays[5] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->StdDeviationPath);
  }
  if(m_InputValues->FindSummation)
  {
    arrays[6] = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SummationPath);
  }

  m_Throttle.reset(numTuples, "Finding Statistics");

  // Allow data-based parallelization
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numTuples);
  ExecuteParallelFunction<ComputeNeighborListStatisticsImpl, NoBooleanType>(type, dataAlg, this, inputINeighborList, m_InputValues->FindLength, m_InputValues->FindMin, m_InputValues->FindMax,
                                                                            m_InputValues->FindMean, m_InputValues->FindMedian, m_InputValues->FindStdDeviation, m_InputValues->FindSummation, arrays,
                                                                            m_ShouldCancel);

  return {};
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeNeighborListStatistics::getCancel()
{
  return m_ShouldCancel;
}
