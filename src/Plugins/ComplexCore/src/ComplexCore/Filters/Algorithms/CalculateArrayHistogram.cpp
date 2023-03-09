#include "CalculateArrayHistogram.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/MessageUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

#include <algorithm>
#include <chrono>
#include <tuple>

using namespace complex;

namespace
{
template <typename DataArrayType>
class GenerateHistogramFromData
{
public:
  GenerateHistogramFromData(const int32 numBins, const IDataArray& inputArray, Float64Array& histogram, std::atomic<usize>& overflow, std::tuple<bool, float64, float64>& range,
                            const std::atomic_bool& shouldCancel, ThreadSafeTaskMessenger& messenger)
  : m_NumBins(numBins)
  , m_InputArray(inputArray)
  , m_Histogram(histogram)
  , m_Overflow(overflow)
  , m_Range(range)
  , m_ShouldCancel(shouldCancel)
  , m_Messenger(messenger)
  {
  }
  ~GenerateHistogramFromData() = default;

  void operator()() const
  {
    const auto& inputArray = dynamic_cast<const DataArray<DataArrayType>&>(m_InputArray);
    auto end = inputArray.getSize();
    const uint64 arrayID = inputArray.getId();
    const usize progressIncrement = m_Messenger.getProgressIncrement(arrayID);

    // tuple visualization: Histogram = {(bin maximum, count), (bin maximum, count), ... }
    float64 min = 0.0;
    float64 max = 0.0;
    if(std::get<0>(m_Range))
    {
      min = std::get<1>(m_Range);
      max = std::get<2>(m_Range);
    }
    else
    {
      auto minMax = std::minmax_element(inputArray.begin(), inputArray.end());
      min = (static_cast<float64>(*minMax.first) - 1);  // ensure upper limit encapsulates max value
      max = (static_cast<float64>(*minMax.second) + 1); // ensure lower limit encapsulates min value
    }

    const float64 increment = (max - min) / static_cast<float64>(m_NumBins);
    if(m_NumBins == 1) // if one bin, just set the first element to total number of points
    {
      m_Histogram[0] = max;
      m_Histogram[1] = end;
    }
    else
    {
      size_t progressCounter = 0;
      for(usize i = 0; i < end; i++)
      {
        if(m_ShouldCancel)
        {
          return;
        }
        if(progressCounter > progressIncrement)
        {
          m_Messenger.updateProgress(progressCounter, arrayID);
          progressCounter = 0;
        }
        const auto bin = std::floor((inputArray[i] - min) / increment);
        if((bin >= 0) && (bin < m_NumBins))
        {
          m_Histogram[bin * 2 + 1]++;
        }
        else
        {
          m_Overflow++;
        }
        progressCounter++;
      }
    }

    for(int64 i = 0; i < m_NumBins; i++)
    {
      m_Histogram[(i * 2)] = static_cast<float64>(min + (increment * (static_cast<float64>(i) + 1.0))); // load bin maximum into respective position {(x, ), (x , ), ...}
    }
  }

private:
  const int32 m_NumBins = 1;
  std::tuple<bool, float64, float64>& m_Range;
  const IDataArray& m_InputArray;
  Float64Array& m_Histogram;
  std::atomic<usize>& m_Overflow;
  const std::atomic_bool& m_ShouldCancel;
  ThreadSafeTaskMessenger& m_Messenger;
};
} // namespace

// -----------------------------------------------------------------------------
CalculateArrayHistogram::CalculateArrayHistogram(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 CalculateArrayHistogramInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CalculateArrayHistogram::~CalculateArrayHistogram() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CalculateArrayHistogram::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CalculateArrayHistogram::operator()()
{
  const auto numBins = m_InputValues->NumberOfBins;
  const auto selectedArrayPaths = m_InputValues->SelectedArrayPaths;

  std::tuple<bool, float64, float64> range = std::make_tuple(m_InputValues->UserDefinedRange, m_InputValues->MinRange, m_InputValues->MaxRange); // Custom bool, min, max
  ParallelTaskAlgorithm taskRunner;
  ThreadSafeTaskMessenger messenger(m_MessageHandler, "Calculating...");
  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(getCancel())
    {
      return {};
    }
    std::atomic<usize> overflow = 0;
    const auto& inputData = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPaths[i]);
    auto& histogram = m_DataStructure.getDataRefAs<DataArray<float64>>(m_InputValues->CreatedHistogramDataPaths.at(i));
    messenger.addArray(inputData.getId(), inputData.getSize(), inputData.getName());

    ExecuteParallelFunction<GenerateHistogramFromData>(inputData.getDataType(), taskRunner, numBins, inputData, histogram, overflow, range, getCancel(), messenger);

    if(overflow > 0)
    {
      const std::string arrayName = inputData.getName();
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("{} values not categorized into bin for array {}", overflow, arrayName));
    }
  }

  taskRunner.wait();

  return {};
}
