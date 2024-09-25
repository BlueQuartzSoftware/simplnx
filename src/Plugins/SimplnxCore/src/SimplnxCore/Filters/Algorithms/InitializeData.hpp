#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

namespace nx::core
{
constexpr char k_DelimiterChar = ';';

enum InitializeType : uint64
{
  FillValue,
  Incremental,
  Random,
  RangedRandom
};

enum StepType : uint64
{
  Addition,
  Subtraction
};

inline std::string CreateComponentValuesString(const std::vector<int64>& componentValues, usize numComps)
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

inline std::string CreateComponentValuesString(const std::vector<std::string>& componentValuesStrs, usize numComps)
{
  std::vector<int64> componentValues;
  componentValues.reserve(componentValues.size());
  std::transform(componentValuesStrs.begin(), componentValuesStrs.end(), std::back_inserter(componentValues), [](const std::string& s) -> int64 {
    return (StringUtilities::toLower(s) == "true") ? 1 : (StringUtilities::toLower(s) == "false") ? 0 : std::stoll(s);
  });
  return CreateComponentValuesString(componentValues, numComps);
}

inline void CreateFillValuesInitPreflightValues(const std::string& initFillValueStr, usize numComps, std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
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

  updatedValStrm << CreateComponentValuesString(initFillTokens, numComps);

  preflightUpdatedValues.push_back({"Tuple Details", updatedValStrm.str()});
};

inline void CreateIncrementalValuesInitPreflightValues(const std::string& initFillValueStr, usize stepOperation, const std::string& stepValueStr, usize numTuples, usize numComps,
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

    ss << CreateComponentValuesString(initFillTokens, numComps);

    if(stepOperation == StepType::Addition)
    {
      ss << fmt::format("\nThe components in each tuple will increment by the following: {}.", CreateComponentValuesString(stepValueTokens, numComps));
    }
    else
    {
      ss << fmt::format("\nThe components in each tuple will decrement by the following: {}.", CreateComponentValuesString(stepValueTokens, numComps));
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
    ss << fmt::format("{}\n", CreateComponentValuesString(initFillValues, numComps));
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

inline void CreateRandomValuesInitPreflightValues(bool standardizeSeed, InitializeType initType, const std::string& initStartRange, const std::string& initEndRange, usize numTuples, usize numComps,
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
      ss << "\n" << CreateComponentValuesString(startRangeTokens, numComps);
      ss << "\nto these ending values:";
      auto endRangeTokens = StringUtilities::split(initEndRange, ";", false);
      ss << "\n" << CreateComponentValuesString(endRangeTokens, numComps);
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

struct SIMPLNXCORE_EXPORT InitializeDataInputValues
{
  DataPath InputArrayPath;
  InitializeType initType;
  StepType stepType;
  std::vector<std::string> stringValues;
  std::vector<std::string> startValues;
  std::vector<std::string> stepValues;
  uint64 seed;
  std::vector<std::string> randBegin;
  std::vector<std::string> randEnd;
  bool standardizeSeed;
};

struct SIMPLNXCORE_EXPORT ValidateMultiInputFunctor
{
  // The single comp size validation defaults to off as size 0 is checked earlier in the function
  template <typename T>
  IFilter::PreflightResult operator()(const usize expectedComp, const std::string& unfilteredStr, const usize singleCompSize = 0)
  {
    std::vector<std::string> splitVals = StringUtilities::split(StringUtilities::trimmed(unfilteredStr), k_DelimiterChar);

    if(splitVals.empty())
    {
      return IFilter::MakePreflightErrorResult(-11610, fmt::format("A required parameter is unable to be processed with '{}' delimiter. Input: {}", k_DelimiterChar, unfilteredStr));
    }

    for(usize comp = 0; comp < splitVals.size(); comp++)
    {
      if(splitVals[comp].empty())
      {
        return IFilter::MakePreflightErrorResult(-11611, fmt::format("Empty value found after '{}' components were converted. Check for duplicate '{}' next to one another.", comp, k_DelimiterChar));
      }

      Result<T> result = ConvertTo<T>::convert(splitVals[comp]);

      if(result.invalid())
      {
        return IFilter::MakePreflightErrorResult(-11612, fmt::format("Unable to process '{}' into a {} value.", splitVals[comp], DataTypeToString(GetDataType<T>())));
      }
    }

    if(splitVals.size() == expectedComp)
    {
      return {}; // Valid
    }

    if(splitVals.size() == singleCompSize)
    {
      return {}; // Valid
    }

    if(splitVals.size() == expectedComp + 1)
    {
      if(unfilteredStr.back() == k_DelimiterChar)
      {
        return IFilter::MakePreflightErrorResult(-11613, fmt::format("Remove the extra delimiter '{}' at the end of your value sequence: {}.", k_DelimiterChar, unfilteredStr));
      }
    }

    return IFilter::MakePreflightErrorResult(-11614,
                                             fmt::format("Using '{}' as a delimiter we are unable to break '{}' into the required {} components.", k_DelimiterChar, unfilteredStr, expectedComp));
  }
};

class InitializeData
{
public:
  InitializeData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeDataInputValues* inputValues);
  ~InitializeData() noexcept;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;
  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InitializeDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
