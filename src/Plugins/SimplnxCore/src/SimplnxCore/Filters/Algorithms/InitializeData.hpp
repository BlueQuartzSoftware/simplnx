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
