#include "ComputeArrayHistogram.hpp"

#include "SimplnxCore/Filters/ComputeArrayHistogramFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <chrono>
#include <tuple>

using namespace nx::core;

namespace
{
template <typename DataArrayType>
class GenerateHistogramFromData
{
public:
  GenerateHistogramFromData(ComputeArrayHistogram& filter, const int32 numBins, const IDataArray& inputArray, AbstractDataStore<float64>& histogram, std::atomic<usize>& overflow,
                            std::tuple<bool, float64, float64>& range, size_t progressIncrement)
  : m_Filter(filter)
  , m_NumBins(numBins)
  , m_InputArray(inputArray)
  , m_Histogram(histogram)
  , m_Overflow(overflow)
  , m_Range(range)
  , m_ProgressIncrement(progressIncrement)
  {
  }
  ~GenerateHistogramFromData() = default;

  void operator()() const
  {
    const auto& inputArray = dynamic_cast<const DataArray<DataArrayType>&>(m_InputArray).getDataStoreRef();
    auto end = inputArray.getSize();

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
        if(progressCounter > m_ProgressIncrement)
        {
          m_Filter.updateThreadSafeProgress(progressCounter);
          progressCounter = 0;
        }
        if(m_Filter.getCancel())
        {
          return;
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
  ComputeArrayHistogram& m_Filter;
  const int32 m_NumBins = 1;
  std::tuple<bool, float64, float64>& m_Range;
  const IDataArray& m_InputArray;
  AbstractDataStore<float64>& m_Histogram;
  std::atomic<usize>& m_Overflow;
  size_t m_ProgressIncrement = 100;
};
} // namespace

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
void ComputeArrayHistogram::updateThreadSafeProgress(size_t counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update
  {
    auto progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / static_cast<double>(m_TotalElements)) * 100.0);
    std::string progressMessage = "Calculating... ";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeArrayHistogram::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeArrayHistogram::operator()()
{
  const auto numBins = m_InputValues->NumberOfBins;
  const auto selectedArrayPaths = m_InputValues->SelectedArrayPaths;

  for(const auto& arrayPath : selectedArrayPaths)
  {
    m_TotalElements += m_DataStructure.getDataAs<IDataArray>(arrayPath)->getSize();
  }
  auto progressIncrement = m_TotalElements / 100;

  std::tuple<bool, float64, float64> range = std::make_tuple(m_InputValues->UserDefinedRange, m_InputValues->MinRange, m_InputValues->MaxRange); // Custom bool, min, max
  ParallelTaskAlgorithm taskRunner;

  std::atomic<usize> overflow = 0;

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& inputData = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPaths[i]);
    auto& histogram = m_DataStructure.getDataAs<DataArray<float64>>(m_InputValues->CreatedHistogramDataPaths.at(i))->getDataStoreRef();
    ExecuteParallelFunction<GenerateHistogramFromData>(inputData.getDataType(), taskRunner, *this, numBins, inputData, histogram, overflow, range, progressIncrement);

    if(overflow > 0)
    {
      const std::string arrayName = inputData.getName();
      ComputeArrayHistogram::updateProgress(fmt::format("{} values not categorized into bin for array {}", overflow.load(), arrayName));
    }
  }

  taskRunner.wait();

  return {};
}
