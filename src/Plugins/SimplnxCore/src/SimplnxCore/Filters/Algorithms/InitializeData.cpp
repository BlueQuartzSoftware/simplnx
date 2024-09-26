

#include "InitializeData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <chrono>
#include <limits>

using namespace nx::core;

namespace
{

// At the current time this code could be simplified with a bool in the incremental template, HOWEVER,
// it was done this way to allow for expansion of operations down the line multiplication, division, etc.
template <bool UseAddition, bool UseSubtraction>
struct IncrementalOptions
{
  static constexpr bool UsingAddition = UseAddition;
  static constexpr bool UsingSubtraction = UseSubtraction;
};

using AdditionT = IncrementalOptions<true, false>;
using SubtractionT = IncrementalOptions<false, true>;

template <typename T>
void ValueFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& stringValues)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  if(numComp > 1)
  {
    std::vector<T> values;

    for(const auto& str : stringValues)
    {
      values.emplace_back(ConvertTo<T>::convert(str).value());
    }

    usize numTup = dataStore.getNumberOfTuples();

    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];
      }
    }
  }
  else
  {
    Result<T> result = ConvertTo<T>::convert(stringValues[0]);
    T value = result.value();
    dataStore.fill(value);
  }
}

template <typename T, class IncrementalOptions = AdditionT>
void IncrementalFill(AbstractDataStore<T>& dataStore, const std::vector<std::string>& startValues, const std::vector<std::string>& stepValues)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<T> values(numComp);
  std::vector<T> steps(numComp);

  for(usize comp = 0; comp < numComp; comp++)
  {
    Result<T> result = ConvertTo<T>::convert(startValues[comp]);
    values[comp] = result.value();
    if constexpr(!std::is_same_v<T, bool>)
    {
      result = ConvertTo<T>::convert(stepValues[comp]);
      steps[comp] = result.value();
    }
  }

  usize numTup = dataStore.getNumberOfTuples();

  if constexpr(std::is_same_v<T, bool>)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      dataStore[comp] = values[comp];

      if constexpr(IncrementalOptions::UsingAddition)
      {
        values[comp] = ConvertTo<uint8>::convert(stepValues[comp]).value() != 0 ? true : values[comp];
      }
      if constexpr(IncrementalOptions::UsingSubtraction)
      {
        values[comp] = ConvertTo<uint8>::convert(stepValues[comp]).value() != 0 ? false : values[comp];
      }
    }

    for(usize tup = 1; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];
      }
    }
  }

  if constexpr(!std::is_same_v<T, bool>)
  {
    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataStore[tup * numComp + comp] = values[comp];

        if constexpr(IncrementalOptions::UsingAddition)
        {
          values[comp] += steps[comp];
        }
        if constexpr(IncrementalOptions::UsingSubtraction)
        {
          values[comp] -= steps[comp];
        }
      }
    }
  }
}

template <typename T, bool Ranged, class DistributionT>
void RandomFill(std::vector<DistributionT>& dist, AbstractDataStore<T>& dataStore, const uint64 seed, const bool standardizeSeed)
{
  usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<std::mt19937_64> generators(numComp, std::mt19937_64{});

  for(usize comp = 0; comp < numComp; comp++)
  {
    generators[comp].seed((standardizeSeed ? seed : seed + comp)); // If standardizing seed all generators use the same else, use modified seeds
  }

  usize numTup = dataStore.getNumberOfTuples();

  for(usize tup = 0; tup < numTup; tup++)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      if constexpr(std::is_floating_point_v<T>)
      {
        if constexpr(Ranged)
        {
          dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
        }
        if constexpr(!Ranged)
        {
          if constexpr(std::is_signed_v<T>)
          {
            dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * (std::numeric_limits<T>::max() - 1) * (((rand() & 1) == 0) ? 1 : -1));
          }
          if constexpr(!std::is_signed_v<T>)
          {
            dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * std::numeric_limits<T>::max());
          }
        }
      }
      if constexpr(!std::is_floating_point_v<T>)
      {
        dataStore[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
      }
    }
  }
}

template <typename T, class... ArgsT>
void FillIncForwarder(const StepType& stepType, ArgsT&&... args)
{
  switch(stepType)
  {
  case StepType::Addition: {
    ::IncrementalFill<T, AdditionT>(std::forward<ArgsT>(args)...);
    return;
  }
  case StepType::Subtraction: {
    ::IncrementalFill<T, SubtractionT>(std::forward<ArgsT>(args)...);
    return;
  }
  }
}

template <typename T, bool Ranged, class... ArgsT>
void FillRandomForwarder(const std::vector<T>& range, usize numComp, ArgsT&&... args)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    std::vector<std::uniform_int_distribution<int64>> dists;
    for(usize comp = 0; comp < numComp * 2; comp += 2)
    {
      dists.emplace_back((range.at(comp) ? 1 : 0), (range.at(comp + 1) ? 1 : 0));
    }
    ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(dists, std::forward<ArgsT>(args)...);
    return;
  }
  if constexpr(!std::is_floating_point_v<T>)
  {
    std::vector<std::uniform_int_distribution<int64>> dists;
    for(usize comp = 0; comp < numComp * 2; comp += 2)
    {
      dists.emplace_back(range.at(comp), range.at(comp + 1));
    }
    ::RandomFill<T, Ranged, std::uniform_int_distribution<int64>>(dists, std::forward<ArgsT>(args)...);
  }
  if constexpr(std::is_floating_point_v<T>)
  {
    if constexpr(Ranged)
    {

      std::vector<std::uniform_real_distribution<float64>> dists;
      for(usize comp = 0; comp < numComp * 2; comp += 2)
      {
        dists.emplace_back(range.at(comp), range.at(comp + 1));
      }
      ::RandomFill<T, Ranged, std::uniform_real_distribution<float64>>(dists, std::forward<ArgsT>(args)...);
    }
    if constexpr(!Ranged)
    {
      std::vector<std::uniform_real_distribution<float64>> dists;
      for(usize comp = 0; comp < numComp * 2; comp += 2)
      {
        dists.emplace_back(0, 1);
      }
      ::RandomFill<T, Ranged, std::uniform_real_distribution<float64>>(dists, std::forward<ArgsT>(args)...);
    }
  }
}

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

struct FillArrayFunctor
{
  template <typename T>
  void operator()(IDataArray& iDataArray, const InitializeDataInputValues& inputValues)
  {
    auto& dataStore = iDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numComp = dataStore.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

    switch(inputValues.initType)
    {
    case InitializeType::FillValue: {
      return ::ValueFill<T>(dataStore, standardizeMultiComponent(numComp, inputValues.stringValues));
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataStore, standardizeMultiComponent(numComp, inputValues.startValues), standardizeMultiComponent(numComp, inputValues.stepValues));
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
      return ::FillRandomForwarder<T, false>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed);
    }
    case InitializeType::RangedRandom: {
      auto randBegin = standardizeMultiComponent(numComp, inputValues.randBegin);
      auto randEnd = standardizeMultiComponent(numComp, inputValues.randEnd);

      std::vector<T> range;
      for(usize comp = 0; comp < numComp; comp++)
      {
        Result<T> result = ConvertTo<T>::convert(randBegin[comp]);
        range.push_back(result.value());
        result = ConvertTo<T>::convert(randEnd[comp]);
        range.push_back(result.value());
      }
      return ::FillRandomForwarder<T, true>(range, numComp, dataStore, inputValues.seed, inputValues.standardizeSeed);
    }
    }
  }
};

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

// -----------------------------------------------------------------------------
InitializeData::InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
InitializeData::~InitializeData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& InitializeData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> InitializeData::operator()()
{

  auto& iDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);

  ExecuteDataFunction(::FillArrayFunctor{}, iDataArray.getDataType(), iDataArray, *m_InputValues);

  return {};
}
