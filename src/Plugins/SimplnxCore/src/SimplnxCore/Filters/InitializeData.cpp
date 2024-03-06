#include "InitializeData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <chrono>
#include <limits>
#include <random>
#include <sstream>
#include <thread>

using namespace nx::core;

namespace
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

struct InitializeDataInputValues
{
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

template <bool UseAddition, bool UseSubtraction>
struct IncrementalOptions
{
  static inline constexpr bool UsingAddition = UseAddition;
  static inline constexpr bool UsingSubtraction = UseSubtraction;
};

using AdditionT = IncrementalOptions<true, false>;
using SubtractionT = IncrementalOptions<false, true>;

template <typename T>
void ValueFill(DataArray<T>& dataArray, const std::vector<std::string>& stringValues)
{
  usize numComp = dataArray.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  if(numComp > 1)
  {
    std::vector<T> values;

    for(const auto& str : stringValues)
    {
      values.emplace_back(ConvertTo<T>::convert(str).value());
    }

    usize numTup = dataArray.getNumberOfTuples();

    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataArray[tup * numComp + comp] = values[comp];
      }
    }
  }
  else
  {
    Result<T> result = ConvertTo<T>::convert(stringValues[0]);
    T value = result.value();
    dataArray.fill(value);
  }
}

template <typename T, class IncrementalOptions = AdditionT>
void IncrementalFill(DataArray<T>& dataArray, const std::vector<std::string>& startValues, const std::vector<std::string>& stepValues)
{
  usize numComp = dataArray.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

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

  usize numTup = dataArray.getNumberOfTuples();

  if constexpr(std::is_same_v<T, bool>)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      dataArray[comp] = values[comp];

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
        dataArray[tup * numComp + comp] = values[comp];
      }
    }
  }

  if constexpr(!std::is_same_v<T, bool>)
  {
    for(usize tup = 0; tup < numTup; tup++)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        dataArray[tup * numComp + comp] = values[comp];

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
void RandomFill(std::vector<DistributionT>& dist, DataArray<T>& dataArray, const uint64 seed, const bool standardizeSeed)
{
  usize numComp = dataArray.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<std::mt19937_64> generators(numComp, std::mt19937_64{});

  for(usize comp = 0; comp < numComp; comp++)
  {
    generators[comp].seed((standardizeSeed ? seed : seed + comp)); // If standardizing seed all generators use the same else, use modified seeds
  }

  usize numTup = dataArray.getNumberOfTuples();

  for(usize tup = 0; tup < numTup; tup++)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      if constexpr(std::is_floating_point_v<T>)
      {
        if constexpr(Ranged)
        {
          dataArray[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
        }
        if constexpr(!Ranged)
        {
          if constexpr(std::is_signed_v<T>)
          {
            dataArray[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * (std::numeric_limits<T>::max() - 1) * (((rand() & 1) == 0) ? 1 : -1));
          }
          if constexpr(!std::is_signed_v<T>)
          {
            dataArray[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]) * std::numeric_limits<T>::max());
          }
        }
      }
      if constexpr(!std::is_floating_point_v<T>)
      {
        dataArray[tup * numComp + comp] = static_cast<T>(dist[comp](generators[comp]));
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

inline std::vector<std::string> standardizeMultiComponent(const usize numComps, const std::vector<std::string>& componentValues)
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
    auto& dataArray = dynamic_cast<DataArray<T>&>(iDataArray);
    usize numComp = dataArray.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

    switch(inputValues.initType)
    {
    case InitializeType::FillValue: {
      return ::ValueFill<T>(dataArray, standardizeMultiComponent(numComp, inputValues.stringValues));
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataArray, standardizeMultiComponent(numComp, inputValues.startValues), standardizeMultiComponent(numComp, inputValues.stepValues));
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
      return ::FillRandomForwarder<T, false>(range, numComp, dataArray, inputValues.seed, inputValues.standardizeSeed);
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
      return ::FillRandomForwarder<T, true>(range, numComp, dataArray, inputValues.seed, inputValues.standardizeSeed);
    }
    }
  }
};

struct ValidateMultiInputFunctor
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
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string InitializeData::name() const
{
  return FilterTraits<InitializeData>::name;
}

//------------------------------------------------------------------------------
std::string InitializeData::className() const
{
  return FilterTraits<InitializeData>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeData::uuid() const
{
  return FilterTraits<InitializeData>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeData::humanName() const
{
  return "Initialize Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeData::defaultTags() const
{
  return {className(), "Memory Management", "Initialize", "Create", "Generate", "Data"};
}

//------------------------------------------------------------------------------
Parameters InitializeData::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayPath_Key, "Any Component Array", "The data array in which to initialize the data", DataPath{}, nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Data Initialization"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Method for determining the what values of the data in the array should be initialized to",
                                                                    static_cast<ChoicesParameter::ValueType>(0),
                                                                    ChoicesParameter::Choices{"Fill Value", "Incremental", "Random", "Random With Range"})); // sequence dependent DO NOT REORDER

  params.insert(std::make_unique<StringParameter>(k_InitValue_Key, "Fill Values [Seperated with ;]",
                                                  "Specify values for each component. Ex: A 3-component array would be 6;8;12 and every tuple would have these same component values", "1;1;1"));

  params.insert(std::make_unique<StringParameter>(k_StartingFillValue_Key, "Starting Value [Seperated with ;]", "The value to start incrementing from", "0;1;2"));
  params.insert(std::make_unique<ChoicesParameter>(k_StepOperation_Key, "Step Operation", "The type of step operation to preform", static_cast<ChoicesParameter::ValueType>(0),
                                                   ChoicesParameter::Choices{"Addition", "Subtraction"}));
  params.insert(std::make_unique<StringParameter>(k_StepValue_Key, "Increment/Step Value [Seperated with ;]", "The number to increment/decrement the fill value by", "1;1;1"));

  params.insert(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the Seed Value will be used to seed the generator", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeData SeedValue"));
  params.insert(std::make_unique<BoolParameter>(k_StandardizeSeed_Key, "Use the Same Seed for Each Component",
                                                "When true the same seed will be used for each component's generator in a multi-component array", false));

  params.insert(
      std::make_unique<StringParameter>(k_InitStartRange_Key, "Initialization Start Range [Seperated with ;]", "[Inclusive] The lower bound initialization range for random values", "0;0;0"));
  params.insert(std::make_unique<StringParameter>(k_InitEndRange_Key, "Initialization End Range [Seperated with ;]", "[Inclusive]  The upper bound initialization range for random values", "1;1;1"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  /* Using Fill Value */
  params.linkParameters(k_InitType_Key, k_InitValue_Key, static_cast<ChoicesParameter::ValueType>(0));

  /* Using Incremental */
  params.linkParameters(k_InitType_Key, k_StartingFillValue_Key, static_cast<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_InitType_Key, k_StepOperation_Key, static_cast<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_InitType_Key, k_StepValue_Key, static_cast<ChoicesParameter::ValueType>(1));

  /* Random - Using Random */
  params.linkParameters(k_InitType_Key, k_UseSeed_Key, static_cast<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_InitType_Key, k_SeedValue_Key, static_cast<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_InitType_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(2));
  params.linkParameters(k_InitType_Key, k_StandardizeSeed_Key, static_cast<ChoicesParameter::ValueType>(2));

  /* Random - Using Random With Range */
  params.linkParameters(k_InitType_Key, k_UseSeed_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_SeedValue_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_SeedArrayName_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_StandardizeSeed_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_InitStartRange_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_InitEndRange_Key, static_cast<ChoicesParameter::ValueType>(3));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeData::clone() const
{
  return std::make_unique<InitializeData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto seedArrayNameValue = args.value<std::string>(k_SeedArrayName_Key);
  auto initializeTypeValue = static_cast<InitializeType>(args.value<uint64>(k_InitType_Key));

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto& iDataArray = data.getDataRefAs<IDataArray>(args.value<DataPath>(k_ArrayPath_Key));
  usize numComp = iDataArray.getNumberOfComponents(); // check that the values string is greater than max comps

  if(iDataArray.getDataType() == DataType::boolean)
  {
    std::stringstream updatedValStrm;

    updatedValStrm << "We detected that you are doing an operation on a boolean array.\n";
    updatedValStrm << "The ONLY two ways to specify a 'false' boolean value are as follows:\n";
    updatedValStrm << "- boolean value string types as follows ignoring apostrophe marks: 'False', 'FALSE', 'false'\n";
    updatedValStrm << "- all well formed integers and well formed floating point definitions of 0\n\n";

    updatedValStrm << "ANY OTHER string or number WILL BE 'true', although it is good practice to define true values as follows:\n";
    updatedValStrm << "- boolean value string types as follows ignoring apostrophe marks: 'True', 'TRUE', 'true'\n";
    updatedValStrm << "- all well formed integers and well formed floating point definitions of 1";

    preflightUpdatedValues.push_back({"Boolean Note", updatedValStrm.str()});
  }

  if(numComp > 1)
  {
    std::stringstream updatedValStrm;

    updatedValStrm << "We detected that you are doing an operation on a multi-component array.\n";
    updatedValStrm
        << "If you do NOT want to use unique values for each component, you can just supply one value to the input box and we will apply that value to every component for the tuple.\nExample: 1\n\n";

    updatedValStrm << fmt::format("If you DO want to use unique values for each component, you need to supply {} values of type {} seperated by '{}'.\n", numComp,
                                  DataTypeToString(iDataArray.getDataType()), k_DelimiterChar);
    updatedValStrm << "Example: ";

    for(usize comp = 0; comp < numComp; comp++)
    {
      updatedValStrm << "1";

      if(comp != numComp - 1)
      {
        updatedValStrm << k_DelimiterChar;
      }
    }

    preflightUpdatedValues.push_back({"Multi-Component Note", updatedValStrm.str()});
  }

  std::stringstream operationNuancesStrm;

  switch(initializeTypeValue)
  {
  case InitializeType::FillValue: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitValue_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    operationNuancesStrm << "None to note";

    break;
  }
  case InitializeType::Incremental: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_StartingFillValue_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    if(iDataArray.getDataType() == DataType::boolean)
    {
      // custom bool checks here
      std::stringstream updatedValStrm;

      updatedValStrm << "We detected that you are doing an incremental operation on a boolean array.\n";
      updatedValStrm << "For the step values please enter uint8 values, preferably a 0 or 1 only.\n";

      switch(static_cast<StepType>(args.value<uint64>(k_StepOperation_Key)))
      {
      case Addition: {
        updatedValStrm << "You have currently selected the addition operation.\nAny step value that is greater than 0 will cause all values to be 'true' after the first tuple, 'true' "
                          "values will remain unchanged.\n";
        updatedValStrm << "The two possibilities:\n";
        updatedValStrm << "- If your start value is 'false' and step value > 0, the array will initialize to | false | true | true | ... |\n";
        updatedValStrm << "- If your start value is 'true' and step value > 0, the array will initialize to | true | true | true | ... |";
        break;
      }
      case Subtraction: {
        updatedValStrm << "You have currently selected the addition operation.\nAny step value that is greater than 0 will cause all values to be 'false' after the first tuple, 'false' "
                          "values will remain unchanged.\n";
        updatedValStrm << "The two possibilities:\n";
        updatedValStrm << "- If your start value is 'true' and step value > 0, the array will initialize to | true | false | false | ... |\n";
        updatedValStrm << "- If your start value is 'false' and step value > 0, the array will initialize to | false | false | false | ... |";
        break;
      }
      }

      preflightUpdatedValues.push_back({"Boolean Incremental Nuances", updatedValStrm.str()});

      result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, DataType::uint8, numComp, args.value<std::string>(k_StepValue_Key), 1);
      if(result.outputActions.invalid())
      {
        return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
      }
    }
    else
    {
      result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_StepValue_Key), 1);
      if(result.outputActions.invalid())
      {
        return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
      }
    }

    operationNuancesStrm << "When supplying a 0 as the Step value the component at that index in the tuple will be unchanged in the end array.\n";
    operationNuancesStrm << fmt::format("Example: Suppose we have a two component array with a Step Values of '2{}0', Starting Values of '0', and an addition Step Operation\n", k_DelimiterChar);
    operationNuancesStrm << "The output array would look like 0,0 | 2,0 | 4,0 | 6,0 | ...";

    break;
  }
  case InitializeType::RangedRandom: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitStartRange_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitEndRange_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    [[fallthrough]];
  }
  case InitializeType::Random: {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));

    if(numComp == 1)
    {
      if(args.value<bool>(k_StandardizeSeed_Key))
      {
        operationNuancesStrm << fmt::format("You chose to standardize the seed for each component, but the array {} is a single component so it will not alter the randomization scheme.",
                                            iDataArray.getName());
      }
    }
    else
    {
      if(args.value<bool>(k_StandardizeSeed_Key))
      {
        operationNuancesStrm << "This generates THE SAME sequences of random numbers for each component in the array based on one seed.\n";
        operationNuancesStrm << "The resulting array will look like | 1,1,1 | 9,9,9 | ...\n";
      }
      else
      {
        operationNuancesStrm << "This generates DIFFERENT sequences of random numbers for each component in the array based on x seeds all modified versions of an original seed.\n";
        operationNuancesStrm << "The resulting array will look like | 1,9,5 | 7,1,6 | ...\n";
      }
    }

    break;
  }
  }

  preflightUpdatedValues.push_back({"Operation Nuances", operationNuancesStrm.str()});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> InitializeData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto initType = static_cast<InitializeType>(args.value<uint64>(k_InitType_Key));

  auto seed = args.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!args.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  if(initType == InitializeType::Random || initType == InitializeType::RangedRandom)
  {
    // Store Seed Value in Top Level Array
    data.getDataRefAs<UInt64Array>(DataPath({args.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;
  }

  InitializeDataInputValues inputValues;

  inputValues.initType = initType;
  inputValues.stepType = static_cast<StepType>(args.value<uint64>(k_StepOperation_Key));
  inputValues.stringValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitValue_Key)), k_DelimiterChar);
  inputValues.startValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_StartingFillValue_Key)), k_DelimiterChar);
  inputValues.stepValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_StepValue_Key)), k_DelimiterChar);
  inputValues.seed = seed;
  inputValues.randBegin = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitStartRange_Key)), k_DelimiterChar);
  inputValues.randEnd = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitEndRange_Key)), k_DelimiterChar);
  inputValues.standardizeSeed = args.value<bool>(k_StandardizeSeed_Key);

  auto& iDataArray = data.getDataRefAs<IDataArray>(args.value<DataPath>(k_ArrayPath_Key));

  ExecuteDataFunction(::FillArrayFunctor{}, iDataArray.getDataType(), iDataArray, inputValues);

  return {};
}
} // namespace nx::core
