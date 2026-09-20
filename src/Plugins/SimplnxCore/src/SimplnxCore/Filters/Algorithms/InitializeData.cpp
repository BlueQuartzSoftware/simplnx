#include "InitializeData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @struct IncrementalOptions
 * @brief Selects compile-time incremental operations.
 * @tparam UseAddition Enables addition after each generated tuple.
 * @tparam UseSubtraction Enables subtraction after each generated tuple.
 */
template <bool UseAddition, bool UseSubtraction>
struct IncrementalOptions
{
  static constexpr bool UsingAddition = UseAddition;
  static constexpr bool UsingSubtraction = UseSubtraction;
};

using AdditionT = IncrementalOptions<true, false>;
using SubtractionT = IncrementalOptions<false, true>;

// Generated buffers target 65,536 values and retain at least one complete tuple.
constexpr usize k_InitializationChunkValues = 65536;

/**
 * @brief Generates typed values in tuple and component order through reusable chunks.
 * @tparam T Specifies the output scalar type.
 * @tparam ValueGenerator Generates one value from tuple and component indexes.
 * @param dataStore Receives generated values.
 * @param generateValue Generates one tuple component.
 * @param shouldCancel Stops before later chunks when true.
 * @return Error from bulk write, or success after cancellation.
 *
 * All modes share this sink and avoid array-sized resident buffers. A tuple
 * wider than the target creates a larger one-tuple buffer. Bool uses a raw array.
 */
template <typename T, typename ValueGenerator>
Result<> WriteGeneratedValues(AbstractDataStore<T>& dataStore, ValueGenerator&& generateValue, const std::atomic_bool& shouldCancel)
{
  const usize numComponents = dataStore.getNumberOfComponents();
  const usize numTuples = dataStore.getNumberOfTuples();
  if(numComponents == 0 || numTuples == 0)
  {
    return {};
  }

  const usize tuplesPerChunk = std::max<usize>(1, k_InitializationChunkValues / numComponents);
  auto buffer = std::make_unique<T[]>(tuplesPerChunk * numComponents);
  for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += tuplesPerChunk)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize tupleCount = std::min(tuplesPerChunk, numTuples - tupleOffset);
    for(usize localTuple = 0; localTuple < tupleCount; localTuple++)
    {
      const usize tupleIndex = tupleOffset + localTuple;
      for(usize component = 0; component < numComponents; component++)
      {
        buffer[localTuple * numComponents + component] = generateValue(tupleIndex, component);
      }
    }
    auto writeResult = dataStore.copyFromBuffer(tupleOffset * numComponents, nonstd::span<const T>(buffer.get(), tupleCount * numComponents));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }
  return {};
}

/**
 * @brief Repeats parsed component values through bounded writes.
 * @tparam T Specifies the output scalar type.
 * @param dataStore Receives fill values.
 * @param stringValues Provides one value for each component.
 * @param shouldCancel Stops before later chunks when true.
 * @return Bulk-write result, or success after cancellation.
 * @pre Every string converts to T and stringValues has one value per component.
 */
template <typename T>
Result<> ValueFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& stringValues, const std::atomic_bool& shouldCancel)
{
  const usize numComponents = dataStore.getNumberOfComponents();
  std::vector<T> values;
  values.reserve(numComponents);
  for(const auto& stringValue : stringValues)
  {
    values.push_back(StringInterpretationUtilities::Convert<T>(stringValue).value());
  }
  return WriteGeneratedValues<T>(dataStore, [&values](usize, usize component) { return values[component]; }, shouldCancel);
}

/**
 * @brief Generates component-wise incremental values through bounded writes.
 * @tparam T Specifies the output scalar type.
 * @tparam IncrementalOptions Selects addition or subtraction.
 * @param dataStore Receives generated values.
 * @param startValues Provides initial component values.
 * @param stepValues Provides per-component increments or decrements.
 * @param shouldCancel Stops before later chunks when true.
 * @return Bulk-write result, or success after cancellation.
 * @pre Value lists have one convertible string per component.
 * @pre Signed generated values remain representable in T.
 */
template <typename T, class IncrementalOptions = AdditionT>
Result<> IncrementalFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& startValues, const std::vector<std::string>& stepValues, const std::atomic_bool& shouldCancel)
{
  const usize numComponents = dataStore.getNumberOfComponents();

  std::vector<T> values(numComponents);
  std::vector<T> steps(numComponents);

  for(usize component = 0; component < numComponents; component++)
  {
    values[component] = StringInterpretationUtilities::Convert<T>(startValues[component]).value();
    steps[component] = StringInterpretationUtilities::Convert<T>(stepValues[component]).value();
  }

  if constexpr(std::is_same_v<T, bool>)
  {
    return WriteGeneratedValues<T>(
        dataStore,
        [&values, &steps](usize tupleIndex, usize component) {
          const bool value = values[component];
          if(tupleIndex == 0 && steps[component])
          {
            values[component] = IncrementalOptions::UsingAddition;
          }
          return value;
        },
        shouldCancel);
  }
  else
  {
    return WriteGeneratedValues<T>(
        dataStore,
        [&values, &steps](usize, usize component) {
          const T value = values[component];
          if constexpr(IncrementalOptions::UsingAddition)
          {
            values[component] += steps[component];
          }
          if constexpr(IncrementalOptions::UsingSubtraction)
          {
            values[component] -= steps[component];
          }
          return value;
        },
        shouldCancel);
  }
}

/**
 * @brief Generates seeded random values through bounded writes.
 * @tparam T Specifies the output scalar type.
 * @tparam Ranged Selects configured or full-type distributions.
 * @tparam DistributionT Specifies the component distribution type.
 * @param distributions Provides one distribution per component.
 * @param dataStore Receives generated values.
 * @param seed Specifies the initial random seed.
 * @param standardizeSeed Reuses one seed for every component when true.
 * @param shouldCancel Stops before later chunks when true.
 * @return Error from bulk write, or success after cancellation.
 *
 * Unranged floating output uses global rand() state to choose signs. The supplied
 * seed controls magnitude engines but does not fully determine that output.
 */
template <typename T, bool Ranged, class DistributionT>
Result<> RandomFill(std::vector<DistributionT>& distributions, AbstractDataStore<T>& dataStore, const uint64 seed, const bool standardizeSeed, const std::atomic_bool& shouldCancel)
{
  const usize numComponents = dataStore.getNumberOfComponents();

  std::vector<std::mt19937_64> generators(numComponents, std::mt19937_64{});

  for(usize component = 0; component < numComponents; component++)
  {
    generators[component].seed(standardizeSeed ? seed : seed + component);
  }

  return WriteGeneratedValues<T>(
      dataStore,
      [&distributions, &generators](usize, usize component) -> T {
        if constexpr(std::is_floating_point_v<T>)
        {
          if constexpr(Ranged)
          {
            return static_cast<T>(distributions[component](generators[component]));
          }
          else if constexpr(std::is_signed_v<T>)
          {
            return static_cast<T>(distributions[component](generators[component]) * (std::numeric_limits<T>::max() - 1) * (((rand() & 1) == 0) ? 1 : -1));
          }
          else
          {
            return static_cast<T>(distributions[component](generators[component]) * std::numeric_limits<T>::max());
          }
        }
        else
        {
          return static_cast<T>(distributions[component](generators[component]));
        }
      },
      shouldCancel);
}

/**
 * @brief Selects an incremental generator specialization.
 * @tparam T Specifies the output scalar type.
 * @tparam ArgsT Specifies generator arguments.
 * @param stepType Specifies addition or subtraction.
 * @param args Forwards generator arguments.
 * @return Generator result, or an invalid-operation error.
 */
template <typename T, class... ArgsT>
Result<> FillIncForwarder(const StepType& stepType, ArgsT&&... args)
{
  switch(stepType)
  {
  case StepType::Addition: {
    return ::IncrementalFill<T, AdditionT>(std::forward<ArgsT>(args)...);
  }
  case StepType::Subtraction: {
    return ::IncrementalFill<T, SubtractionT>(std::forward<ArgsT>(args)...);
  }
  }
  return MakeErrorResult(-11620, "InitializeData received an invalid incremental operation.");
}

/**
 * @brief Builds component distributions for bounded random generation.
 * @tparam T Specifies the output scalar type.
 * @tparam Ranged Selects configured or full-type distributions.
 * @tparam ArgsT Specifies random-generator arguments.
 * @param range Provides lower and upper values for each component.
 * @param numComponents Specifies the number of output components.
 * @param args Forwards random-generator arguments.
 * @return Random-generator result.
 *
 * All integral types use uniform_int_distribution<int64>. UInt64 bounds above
 * INT64_MAX are not representable by that distribution.
 */
template <typename T, bool Ranged, class... ArgsT>
Result<> FillRandomForwarder(const std::vector<T>& range, usize numComponents, ArgsT&&... args)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    std::vector<std::uniform_int_distribution<int64>> distributions;
    for(usize component = 0; component < numComponents * 2; component += 2)
    {
      distributions.emplace_back((range.at(component) ? 1 : 0), (range.at(component + 1) ? 1 : 0));
    }
    return ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(distributions, std::forward<ArgsT>(args)...);
  }
  else if constexpr(!std::is_floating_point_v<T>)
  {
    std::vector<std::uniform_int_distribution<int64>> distributions;
    for(usize component = 0; component < numComponents * 2; component += 2)
    {
      distributions.emplace_back(range.at(component), range.at(component + 1));
    }
    return ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(distributions, std::forward<ArgsT>(args)...);
  }
  else
  {
    std::vector<std::uniform_real_distribution<float64>> distributions;
    for(usize component = 0; component < numComponents * 2; component += 2)
    {
      distributions.emplace_back(Ranged ? static_cast<float64>(range.at(component)) : 0.0, Ranged ? static_cast<float64>(range.at(component + 1)) : 1.0);
    }
    return ::RandomFill<T, Ranged, std::uniform_real_distribution<float64>>(distributions, std::forward<ArgsT>(args)...);
  }
}

/**
 * @brief Expands one component value to a full component list.
 * @param numComps Specifies the number of output components.
 * @param componentValues Provides one or all component values.
 * @return Explicit component values.
 */
std::vector<std::string> standardizeMultiComponent(const usize numComps, const std::vector<std::string>& componentValues)
{
  if(componentValues.size() == numComps)
  {
    return {componentValues};
  }
  else
  {
    std::vector<std::string> standardized(numComps);
    for(usize comp = 0; comp < numComps; comp++)
    {
      standardized[comp] = componentValues[0];
    }
    return standardized;
  }
}

/**
 * @struct FillArrayFunctor
 * @brief Dispatches a runtime array type to a bounded initializer.
 */
struct FillArrayFunctor
{
  /**
   * @brief Initializes one typed target array.
   * @tparam T Specifies the target scalar type.
   * @param iDataArray Receives initialized values.
   * @param inputValues Specifies initialization mode and values.
   * @param shouldCancel Stops before later chunks when true.
   * @return Bulk-write or invalid-mode result, or success after cancellation.
   * @pre Mode-specific strings convert to T and match the component count.
   */
  template <typename T>
  Result<> operator()(IDataArray& iDataArray, const InitializeDataInputValues& inputValues, const std::atomic_bool& shouldCancel)
  {
    auto& dataStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComp = dataStore.getNumberOfComponents();

    switch(inputValues.initType)
    {
    case InitializeType::FillValue: {
      return ::ValueFill<T>(dataStore, standardizeMultiComponent(numComp, inputValues.stringValues), shouldCancel);
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataStore, standardizeMultiComponent(numComp, inputValues.startValues), standardizeMultiComponent(numComp, inputValues.stepValues),
                                   shouldCancel);
    }
    case InitializeType::Random: {
      std::vector<T> range;
      if constexpr(!std::is_same_v<T, bool>)
      {
        for(usize comp = 0; comp < numComp; comp++)
        {
          range.push_back(std::numeric_limits<T>::min());
          range.push_back(std::numeric_limits<T>::max());
        }
      }
      if constexpr(std::is_same_v<T, bool>)
      {
        for(usize comp = 0; comp < numComp; comp++)
        {
          range.push_back(false);
          range.push_back(true);
        }
      }
      return ::FillRandomForwarder<T, false>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed, shouldCancel);
    }
    case InitializeType::RangedRandom: {
      auto randBegin = standardizeMultiComponent(numComp, inputValues.randBegin);
      auto randEnd = standardizeMultiComponent(numComp, inputValues.randEnd);

      std::vector<T> range;
      for(usize comp = 0; comp < numComp; comp++)
      {
        Result<T> result = StringInterpretationUtilities::Convert<T>(randBegin[comp]);
        range.push_back(result.value());
        result = StringInterpretationUtilities::Convert<T>(randEnd[comp]);
        range.push_back(result.value());
      }
      return ::FillRandomForwarder<T, true>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed, shouldCancel);
    }
    }
    return MakeErrorResult(-11621, "InitializeData received an invalid initialization type.");
  }
};

/**
 * @brief Converts Boolean text or integer text for a preflight preview.
 * @param s Value text.
 * @return One for true, zero for false, or the std::stoll result.
 */
int64 CreateCompValFromStr(const std::string& s)
{
  return (StringUtilities::toLower(s) == "true") ? 1 : (StringUtilities::toLower(s) == "false") ? 0 : std::stoll(s);
}
} // namespace

namespace nx
{
namespace core
{
std::string CreateCompValsStr(const std::vector<int64>& componentValues, usize numComps)
{
  const usize compValueVisibilityThresholdCount = 10;
  const usize startEndEllipseValueCount = compValueVisibilityThresholdCount / 2;

  std::stringstream updatedValStrm;
  auto cValueTokens = componentValues;
  if(cValueTokens.size() == 1)
  {
    cValueTokens = std::vector<int64>(numComps, cValueTokens[0]);
  }

  if(numComps <= compValueVisibilityThresholdCount)
  {
    auto initFillTokensStr = fmt::format("{}", fmt::join(cValueTokens, ","));
    updatedValStrm << fmt::format("|{}|", initFillTokensStr, numComps);
  }
  else
  {
    auto initFillTokensBeginStr = fmt::format("{}", fmt::join(cValueTokens.begin(), cValueTokens.begin() + startEndEllipseValueCount, ","));
    auto initFillTokensEndStr = fmt::format("{}", fmt::join(cValueTokens.end() - startEndEllipseValueCount, cValueTokens.end(), ","));
    updatedValStrm << fmt::format("|{} ... {}|", initFillTokensBeginStr, initFillTokensEndStr, numComps);
  }

  return updatedValStrm.str();
}

std::string CreateCompValsStr(const std::vector<std::string>& componentValuesStrs, usize numComps)
{
  std::vector<int64> componentValues;
  componentValues.reserve(componentValues.size());
  std::transform(componentValuesStrs.begin(), componentValuesStrs.end(), std::back_inserter(componentValues), CreateCompValFromStr);
  return CreateCompValsStr(componentValues, numComps);
}

void CreateFillPreflightVals(const std::string& initFillValueStr, usize numComps, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  if(numComps <= 1)
  {
    return;
  }

  std::stringstream updatedValStrm;

  auto initFillTokens = StringUtilities::split(initFillValueStr, std::vector<char>{';'}, false);
  if(initFillTokens.size() == 1)
  {
    updatedValStrm << "Each tuple will contain the same values for all components: ";
  }
  else
  {
    updatedValStrm << "Each tuple will contain different values for all components: ";
  }

  updatedValStrm << CreateCompValsStr(initFillTokens, numComps);

  preflightUpdatedValues.push_back({"Tuple Details", updatedValStrm.str()});
};

void CreateIncrementalPreflightVals(const std::string& initFillValueStr, usize stepOperation, const std::string& stepValueStr, usize numTuples, usize numComps,
                                    std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  std::stringstream ss;

  auto initFillTokens = StringUtilities::split(initFillValueStr, std::vector<char>{';'}, false);
  auto stepValueTokens = StringUtilities::split(stepValueStr, ";", false);

  if(numComps > 1)
  {
    if(initFillTokens.size() == 1)
    {
      ss << "The first tuple will contain the same values for all components: ";
    }
    else
    {
      ss << "The first tuple will contain different values for all components: ";
    }

    ss << CreateCompValsStr(initFillTokens, numComps);

    if(stepOperation == StepType::Addition)
    {
      ss << fmt::format("\nThe components in each tuple will increment by the following: {}.", CreateCompValsStr(stepValueTokens, numComps));
    }
    else
    {
      ss << fmt::format("\nThe components in each tuple will decrement by the following: {}.", CreateCompValsStr(stepValueTokens, numComps));
    }
  }
  else if(stepOperation == StepType::Addition)
  {
    ss << fmt::format("\nThe single component tuples will increment by {}.", stepValueTokens[0]);
  }
  else
  {

    ss << fmt::format("\nThe single component tuples will decrement by {}.", stepValueTokens[0]);
  }

  std::vector<int64> initFillValues;
  initFillValues.reserve(initFillTokens.size());
  std::transform(initFillTokens.begin(), initFillTokens.end(), std::back_inserter(initFillValues), [](const std::string& s) -> int64 { return std::stoll(s); });
  std::vector<int64> stepValues;
  stepValues.reserve(stepValueTokens.size());
  std::transform(stepValueTokens.begin(), stepValueTokens.end(), std::back_inserter(stepValues), [](const std::string& s) -> int64 { return std::stoll(s); });

  ss << "\n\nTuples Preview:\n";
  const usize maxIterations = 3;
  usize actualIterations = std::min(numTuples, maxIterations);
  for(usize i = 0; i < actualIterations; ++i)
  {
    ss << fmt::format("{}\n", CreateCompValsStr(initFillValues, numComps));
    std::transform(initFillValues.begin(), initFillValues.end(), stepValues.begin(), initFillValues.begin(),
                   [stepOperation](int64 a, int64 b) { return (stepOperation == StepType::Addition) ? (a + b) : (a - b); });
  }
  if(numTuples > maxIterations)
  {
    ss << "...";
  }

  std::vector<usize> zeroIdx;
  for(usize i = 0; i < stepValueTokens.size(); i++)
  {
    if(stepValueTokens[i] == "0")
    {
      zeroIdx.push_back(i);
    }
  }
  if(!zeroIdx.empty())
  {

    ss << "\n\nWarning: Component(s) at index(es) " << fmt::format("[{}]", fmt::join(zeroIdx, ","))
       << " have a ZERO value for the step value.  The values at these component indexes will be unchanged from the starting value.";
  }

  preflightUpdatedValues.push_back({"Tuple Details", ss.str()});
}

void CreateRandomPreflightVals(bool standardizeSeed, InitializeType initType, const std::string& initStartRange, const std::string& initEndRange, usize numTuples, usize numComps,
                               std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  std::stringstream ss;

  if(numComps == 1)
  {
    if(initType == InitializeType::Random)
    {
      ss << fmt::format("The 1 component in each of the {} tuples will be filled with random values.", numTuples);
    }
    else if(initType == InitializeType::RangedRandom)
    {
      ss << fmt::format("The 1 component in each of the {} tuples will be filled with random values ranging from {} to {}.", numTuples, std::stoll(initStartRange), std::stoll(initEndRange));
    }

    if(standardizeSeed)
    {
      ss << "\n\nYou chose to standardize the seed for each component, but the array that will be created has a single component so it will not alter the randomization scheme.";
    }
  }
  else
  {
    if(initType == InitializeType::Random)
    {
      ss << fmt::format("All {} components in each of the {} tuples will be filled with random values.", numComps, numTuples);
    }
    else if(initType == InitializeType::RangedRandom)
    {
      ss << fmt::format("All {} components in each of the {} tuples will be filled with random values ranging from these starting values:", numComps, numTuples);
      auto startRangeTokens = StringUtilities::split(initStartRange, ";", false);
      ss << "\n" << CreateCompValsStr(startRangeTokens, numComps);
      ss << "\nto these ending values:";
      auto endRangeTokens = StringUtilities::split(initEndRange, ";", false);
      ss << "\n" << CreateCompValsStr(endRangeTokens, numComps);
    }

    if(standardizeSeed)
    {
      ss << "\n\nThis will generate THE SAME random value for all components in a given tuple, based on one seed.";
      ss << "\nFor example: |1,1,1| |9,9,9| |4,4,4| ...";
    }
    else
    {
      ss << "\n\nThis will generate DIFFERENT random values for each component in a given tuple, based on multiple seeds that are all modified versions of the original seed.";
      ss << "\nFor example: |1,9,5| |7,1,6| |2,12,7| ...";
    }
  }

  preflightUpdatedValues.push_back({"Tuple Details", ss.str()});
}
} // namespace core
} // namespace nx

InitializeData::InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

InitializeData::~InitializeData() noexcept = default;

const std::atomic_bool& InitializeData::getCancel()
{
  return m_ShouldCancel;
}

Result<> InitializeData::operator()()
{
  auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
  const AlgorithmArrayTargets targets({&iDataArray});
  const bool usesOutOfCoreStore = AnyOutOfCore(targets);
  const bool useOutOfCorePath = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOutOfCorePath ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  return ExecuteDataFunction(::FillArrayFunctor{}, iDataArray.getDataType(), iDataArray, *m_InputValues, m_ShouldCancel);
}
