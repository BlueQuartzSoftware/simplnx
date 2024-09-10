#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

namespace nx::core::HistogramUtilities
{
namespace serial
{
/**
 * @function FillBinRange
 * @brief This function fills a container that is STL compatible and has a bracket operator defined with the bin ranges in the following pattern:
 *  bin_ranges = {minimum, maximum, next maximum, ...} with the format being that the bin's range is defined by bin_ranges[bin_index] <= X < bin_ranges[bin_index + 1]
 * @tparam Type this the end type of the function in that the container and data values are of this type
 * @tparam Container this is the type of object the ranges are loaded into, !!! It is expected that this class is STL compatible nd has a defined `[]` operator !!!
 * @param outputContainer this is the object that the ranges will be loaded into. ASSUMPTION: size is >= numBins + 1 !!! NO Bounds Check!!!
 * @param rangeMinMax this is assumed to be the inclusive minimum value and exclusive maximum value for the overall histogram bins. FORMAT: [minimum, maximum)
 * @param numBins this is the total number of bin ranges being calculated and by extension the indexing value for the ranges
 * @param increment this is the uniform size of the bins
 */
template <typename Type, class Container>
SIMPLNX_EXPORT void FillBinRanges(Container& outputContainer, const std::pair<Type, Type>& rangeMinMax, const int32 numBins, const Type increment)
{
  // WARNING: No bounds checking for type compatibility, it is expected to be done higher up where the type is not abstracted
  // EXPECTED CONTAINER SIZE: numBins + 1

  if(numBins == 1) // if one bin, just set the first element to total number of points
  {
    outputContainer[0] = rangeMinMax.first;
    outputContainer[1] = rangeMinMax.second;
    return;
  }

  // iterate through loading the middle values of the sequence considering `lower bound inclusive, upper bound exclusive`
  for(int32 i = 0; i < numBins; i++)
  {
    outputContainer[i] = rangeMinMax.first + static_cast<Type>(increment * i);
  }

  outputContainer[numBins] = rangeMinMax.second;
}

/**
 * @function FillBinRange
 * @brief This overload is provided in the case the bin size is not provided and therefore must be calculated - see above overload for more detail on functionality
 * @tparam Type this the end type of the function in that the container and data values are of this type
 * @tparam Container this is the type of object the ranges are loaded into, !!! It is expected that this class is STL compatible nd has a defined `[]` operator !!!
 * @param outputContainer this is the object that the ranges will be loaded into. ASSUMPTION: size is >= numBins + 1 !!! NO Bounds Check!!!
 * @param rangeMinMax this is assumed to be the inclusive minimum value and exclusive maximum value for the overall histogram bins. FORMAT: [minimum, maximum)
 * @param numBins this is the total number of bin ranges being calculated and by extension the indexing value for the ranges
 */
template <typename Type, class Container>
SIMPLNX_EXPORT void FillBinRanges(Container& outputContainer, const std::pair<Type, Type>& rangeMinMax, const int32 numBins)
{
  // DEV NOTE: this function also serves to act as a jumping off point for implementing logarithmic histograms down the line

  // Uniform Bin Sizes
  const Type increment = (rangeMinMax.second - rangeMinMax.first) / static_cast<Type>(numBins);

  FillBinRanges(outputContainer, rangeMinMax, numBins, increment);
}

template <typename Type>
SIMPLNX_EXPORT Type CalculateBin(Type value, Type min, Type increment)
{
  return std::floor((value - min) / increment);
}

/**
 * @function GenerateHistogram
 * @brief This function creates a uniform histogram (logarithmic possible, but not currently implemented) it fills two arrays one with the ranges for each bin and one for bin counts
 * See FillBinRanges function for details on the high level structuring of the bin ranges array
 * @tparam Type this the end type of the function in that it is the scalar type of the input and by extension range data
 * @tparam SizeType this is the scalar type of the bin counts container
 * @tparam InputContainer this is the type of object the values are read from:
 * !!! In current implementation it is expected that this class is either AbstractDataStore or std::vector !!!
 * * @tparam OutputContainer this is the type of object the values are stored/written to:
 * !!! In current implementation it is expected that this class is either AbstractDataStore or std::vector !!!
 * @param inputStore this is the container holding the data that will be binned
 * @param binRangesStore this is the object that the ranges will be loaded into.
 * @param rangeMinMax this is assumed to be the inclusive minimum value and exclusive maximum value for the overall histogram bins. FORMAT: [minimum, maximum)
 * @param shouldCancel this is an atomic value that will determine whether execution ends early; `true` cancels algorithm
 * @param numBins this is the total number of bin ranges being calculated and by extension the indexing value for the ranges
 * @param histogramCountsStore this is the container that will hold the counts for each bin (variable type sizing)
 * @param overflow this is an atomic counter for the number of values that fall outside the bin range
 */
template <typename Type, std::integral SizeType, template <typename> class InputContainer, template <typename> class OutputContainer>
SIMPLNX_EXPORT Result<> GenerateHistogram(const InputContainer<Type>& inputStore, OutputContainer<Type>& binRangesStore, const std::pair<Type, Type>& rangeMinMax, const std::atomic_bool& shouldCancel,
                                          const int32 numBins, OutputContainer<SizeType>& histogramCountsStore, std::atomic<usize>& overflow)
{
  usize end = 0;
  if constexpr(std::is_same_v<std::vector<Type>, InputContainer<Type>>)
  {
    end = inputStore.size();
  }
  if constexpr(std::is_same_v<std::vector<Type>, OutputContainer<Type>>)
  {
    // just resize outputs to ensure no wasted space and won't be out of bounds
    binRangesStore.resize(numBins + 1);
    histogramCountsStore.resize(numBins);
  }
  if constexpr(std::is_same_v<AbstractDataStore<Type>, InputContainer<Type>>)
  {
    end = inputStore.getSize();
  }
  if constexpr(std::is_same_v<AbstractDataStore<Type>, OutputContainer<Type>>)
  {
    if(binRangesStore.getSize() < numBins + 1)
    {
      return MakeErrorResult(-23761, fmt::format("HistogramUtilities::{}: binRangesStore is too small to hold ranges. Needed: {} | Current Size: {}. {}:{}", __func__, numBins + 1,
                                                 binRangesStore.getSize(), __FILE__, __LINE__));
    }
    if(histogramCountsStore.getSize() < numBins)
    {
      return MakeErrorResult(-23762, fmt::format("HistogramUtilities::{}: histogramCountsStore is too small to hold counts. Needed: {} | Current Size: {}. {}:{}", __func__, numBins,
                                                 histogramCountsStore.getSize(), __FILE__, __LINE__));
    }
  }

  const Type increment = (rangeMinMax.second - rangeMinMax.first) / static_cast<Type>(numBins);

  // Fill Bins
  FillBinRanges(binRangesStore, rangeMinMax, numBins, increment);

  if(shouldCancel)
  {
    return MakeErrorResult(-23762, fmt::format("HistogramUtilities::{}: Signal Interrupt Received. {}:{}", __func__, __FILE__, __LINE__));
  }

  for(usize i = 0; i < end; i++)
  {
    if(shouldCancel)
    {
      return MakeErrorResult(-23763, fmt::format("HistogramUtilities::{}: Signal Interrupt Received. {}:{}", __func__, __FILE__, __LINE__));
    }
    const auto bin = CalculateBin(inputStore[i], rangeMinMax.first, increment);
    if((bin >= 0) && (bin < numBins))
    {
      histogramCountsStore[bin]++;
    }
    else
    {
      overflow++;
    }
  }

  if(overflow > 0)
  {
    return MakeWarningVoidResult(-23762, fmt::format("HistogramUtilities::{}: Overflow detected: overflow count {}. {}:{}", __func__, overflow.load(), __FILE__, __LINE__));
  }

  return {};
}

/**
 * @class GenerateHistogramFunctor
 * @brief This is a compatibility functor that leverages existing typecasting functions to execute GenerateHistogram() cleanly. In it there are two
 * definitions for the `()` operator that allows for implicit calculation of range, predicated whether a range is passed in or not
 */
struct SIMPLNX_EXPORT GenerateHistogramFunctor
{
  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args) const
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    auto minMax = std::minmax_element(inputStore.begin(), inputStore.end());

    return GenerateHistogram(inputStore, binRangesArray->template getIDataStoreRefAs<AbstractDataStore<Type>>(), std::make_pair(*minMax.first, *minMax.second + static_cast<Type>(1.0)),
                             std::forward<ArgsT>(args)...);
  }

  template <typename Type, class... ArgsT>
  Result<> operator()(const IDataArray* inputArray, IDataArray* binRangesArray, std::pair<float64, float64>&& rangeMinMax, ArgsT&&... args) const
  {
    const auto& inputStore = inputArray->template getIDataStoreRefAs<AbstractDataStore<Type>>();

    // check range ordering : should be min, max
    if(rangeMinMax.first > rangeMinMax.second)
    {
      return MakeErrorResult(-23760, fmt::format("GenerateHistogramFunctor::{}: The range min value is larger than the max value. Min value: {} | Max Value: {}. {}:{}", __func__, rangeMinMax.first,
                                                 rangeMinMax.second, __FILE__, __LINE__));
    }

    return GenerateHistogram(inputStore, binRangesArray->template getIDataStoreRefAs<AbstractDataStore<Type>>(),
                             std::make_pair(static_cast<Type>(rangeMinMax.first), static_cast<Type>(rangeMinMax.second)), std::forward<ArgsT>(args)...);
  }
};
} // namespace serial

namespace concurrent
{
/**
 * @class GenerateHistogramImpl
 * @brief This class is a pseudo-wrapper for the serial::GenerateHistogram, the reason for this class' existence is to hold/define ownership of objects in each thread
 * @tparam Type this the end type of the function in that the container and data values are of this type
 * @tparam SizeType this is the scalar type of the bin counts container
 */
template <typename Type, std::integral SizeType>
class SIMPLNX_EXPORT GenerateHistogramImpl
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
  GenerateHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, std::pair<float64, float64>&& rangeMinMax, const std::atomic_bool& shouldCancel,
                        const int32 numBins, AbstractDataStore<SizeType>& histogramStore, std::atomic<usize>& overflow)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_HistogramStore(histogramStore)
  , m_Overflow(overflow)
  {
    m_Range = std::make_pair(static_cast<Type>(rangeMinMax.first), static_cast<Type>(rangeMinMax.second));
  }

  /**
   * @function constructor
   * @brief This constructor constructs the object then calculates and stores the range implicitly
   * @param inputStore this is the AbstractDataStore holding the data that will be binned
   * @param binRangesStore this is the AbstractDataStore that the ranges will be loaded into.
   * @param shouldCancel this is an atomic value that will determine whether execution ends early
   * @param numBins this is the total number of bin ranges being calculated and by extension the indexing value for the ranges
   * @param histogramStore this is the AbstractDataStore that will hold the counts for each bin (variable type sizing)
   * @param overflow this is an atomic counter for the number of values that fall outside the bin range
   */
  GenerateHistogramImpl(const AbstractDataStore<Type>& inputStore, AbstractDataStore<Type>& binRangesStore, const std::atomic_bool& shouldCancel, const int32 numBins,
                        AbstractDataStore<SizeType>& histogramStore, std::atomic<usize>& overflow)
  : m_InputStore(inputStore)
  , m_ShouldCancel(shouldCancel)
  , m_NumBins(numBins)
  , m_BinRangesStore(binRangesStore)
  , m_HistogramStore(histogramStore)
  , m_Overflow(overflow)
  {
    auto minMax = std::minmax_element(m_InputStore.begin(), m_InputStore.end());
    m_Range = std::make_pair(*minMax.first, *minMax.second + static_cast<Type>(1.0));
  }

  ~GenerateHistogramImpl() = default;

  /**
   * @function operator()
   * @brief This function serves as the execute method
   */
  void operator()() const
  {
    serial::GenerateHistogram(m_InputStore, m_BinRangesStore, m_Range, m_ShouldCancel, m_NumBins, m_HistogramStore, m_Overflow);
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  const int32 m_NumBins = 1;
  std::pair<Type, Type> m_Range = {static_cast<Type>(0.0), static_cast<Type>(0.0)};
  const AbstractDataStore<Type>& m_InputStore;
  AbstractDataStore<Type>& m_BinRangesStore;
  AbstractDataStore<SizeType>& m_HistogramStore;
  std::atomic<usize>& m_Overflow;
};

/**
 * @class InstantiateHistogramImplFunctor
 * @brief This is a compatibility functor that leverages existing typecasting functions to create the appropriately typed GenerateHistogramImpl() cleanly.
 * Designed for compatibility with the existing parallel execution classes.
 */
struct InstantiateHistogramImplFunctor
{
  template <typename T, class... ArgsT>
  auto operator()(const IDataArray* inputArray, IDataArray* binRangesArray, ArgsT&&... args)
  {
    return GenerateHistogramImpl(inputArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), binRangesArray->template getIDataStoreRefAs<AbstractDataStore<T>>(), std::forward<ArgsT>(args)...);
  }
};
} // namespace concurrent
} // namespace nx::core::HistogramUtilities
