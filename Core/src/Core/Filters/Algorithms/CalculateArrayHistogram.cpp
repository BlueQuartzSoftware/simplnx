#include "CalculateArrayHistogram.hpp"

#include "Core/Filters/CalculateArrayHistogramFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
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
  GenerateHistogramFromData(CalculateArrayHistogram& filter, const int32 numBins, const IDataArray& inputArray, Float64Array& histogram, std::atomic<usize>& overflow,
                            std::tuple<bool, float64, float64>& range, size_t progIncrement)
  : m_Filter(filter)
  , m_NumBins(numBins)
  , m_InputArray(inputArray)
  , m_Histogram(histogram)
  , m_Overflow(overflow)
  , m_Range(range)
  , m_ProgIncrement(progIncrement)
  {
  }
  ~GenerateHistogramFromData() = default;

  void operator()() const
  {
      static_assert(std::is_base_of_v<DataObject, DataArrayType>);
      const DataArrayType& inputArray = dynamic_cast<const DataArrayType&>(m_InputArray);
      auto end = inputArray.getSize();

      // tuple visualization: Histogram = {(bin maximum, count), (bin maximum, count), ... }
      float64 min = std::numeric_limits<float>::max();
      float64 max = -1.0 * std::numeric_limits<float>::max();
      if (std::get<0>(m_Range))
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
      if (m_NumBins == 1) // if one bin, just set the first element to total number of points
      {
          m_Histogram[0] = max;
          m_Histogram[1] = end;
      }
      else
      {
          size_t progCounter = 0;
          for (usize i = 0; i < end; i++)
          {
              if (progCounter > m_ProgIncrement)
              {
                  m_Filter.updateThreadSafeProgress(progCounter);
                  progCounter = 0;
              }
              if (m_Filter.getCancel())
              {
                  return;
              }
              const auto bin = std::floor((inputArray[i] - min) / increment);
              if ((bin >= 0) && (bin < m_NumBins))
              {
                  m_Histogram[bin * 2 + 1]++;
              }
              else
              {
                  m_Overflow++;
              }
              progCounter++;
          }
      }

      for (int64 i = 0; i < m_NumBins; i++)
      {
          m_Histogram[(i * 2)] = min + (increment * (i + 1.0)); // load bin maximum into respective postion {(x, ), (x , ), ...}
      }
  }

private:
  CalculateArrayHistogram& m_Filter;
  const int32 m_NumBins = 1;
  std::tuple<bool, float64, float64>& m_Range;
  const IDataArray& m_InputArray;
  Float64Array& m_Histogram;
  std::atomic<usize>& m_Overflow;
  size_t m_ProgIncrement = 100;
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
void CalculateArrayHistogram::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}
// -----------------------------------------------------------------------------
void CalculateArrayHistogram::updateThreadSafeProgress(size_t counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update
  {
    size_t progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / m_TotalElements) * 100.0);
    std::string progressMessage = "Calculating... ";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}

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

  for(const auto& arrayPath : selectedArrayPaths)
  {
    m_TotalElements += m_DataStructure.getDataAs<IDataArray>(arrayPath)->getSize();
  }
  auto progIncrement = m_TotalElements / 100;

  std::tuple<bool, float64, float64> range = std::make_tuple(m_InputValues->UserDefinedRange, m_InputValues->MinRange, m_InputValues->MaxRange); // Custom bool, min, max
  ParallelTaskAlgorithm taskRunner;

  for(int32 i = 0; i < selectedArrayPaths.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    std::atomic<usize> overflow = 0;
    const auto& inputData = m_DataStructure.getDataRefAs<IDataArray>(selectedArrayPaths[i]);
    auto& histogram = m_DataStructure.getDataRefAs<DataArray<float64>>(m_InputValues->CreatedHistogramDataPaths.at(i));

    auto type = inputData.getDataType();
    switch(type)
    {
    case DataType::int8: {
      taskRunner.execute(GenerateHistogramFromData<Int8Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::int16: {
      taskRunner.execute(GenerateHistogramFromData<Int16Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::int32: {
      taskRunner.execute(GenerateHistogramFromData<Int32Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::int64: {
      taskRunner.execute(GenerateHistogramFromData<Int64Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::uint8: {
      taskRunner.execute(GenerateHistogramFromData<UInt8Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::uint16: {
      taskRunner.execute(GenerateHistogramFromData<UInt16Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::uint32: {
      taskRunner.execute(GenerateHistogramFromData<UInt32Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::uint64: {
      taskRunner.execute(GenerateHistogramFromData<UInt64Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::float32: {
      taskRunner.execute(GenerateHistogramFromData<Float32Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    case DataType::float64: {
      taskRunner.execute(GenerateHistogramFromData<Float64Array>(*this, numBins, inputData, histogram, overflow, range, progIncrement));
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }

    if(overflow > 0)
    {
      const std::string arrayName = inputData.getName();
      CalculateArrayHistogram::updateProgress(fmt::format("{} values not categorized into bin for array {}", overflow, arrayName));
    }
  }

  taskRunner.wait();

  return {};
}
