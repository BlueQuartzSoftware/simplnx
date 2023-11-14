#include "InitializeData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fmt/core.h>

#include <chrono>
#include <limits>
#include <random>
#include <thread>

using namespace complex;

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
    result = ConvertTo<T>::convert(stepValues[comp]);
    steps[comp] = result.value();
  }

  usize numTup = dataArray.getNumberOfTuples();

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

template <typename T, class DistributionT>
void RandomFill(std::vector<DistributionT>& dist, DataArray<T>& dataArray, const uint64 seed)
{
  usize numComp = dataArray.getNumberOfComponents(); // We checked that the values string is greater than max comps size so proceed check free

  std::vector<std::mt19937_64> generators(numComp, std::mt19937_64{});

  for(usize comp = 0; comp < numComp; comp++)
  {
    generators[comp].seed(seed + comp);
  }

  usize numTup = dataArray.getNumberOfTuples();

  for(usize tup = 0; tup < numTup; tup++)
  {
    for(usize comp = 0; comp < numComp; comp++)
    {
      dataArray[tup * numComp + comp] = dist[comp](generators[comp]);
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

template <typename T, class... ArgsT>
void FillRandomForwarder(const std::vector<T>& range, usize numComp, ArgsT&&... args)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    std::vector<std::uniform_real_distribution<T>> dists;
    for(usize comp = 0; comp < numComp; comp++)
    {
      dists.emplace_back(range.at(comp), range.at(comp + 1));
    }
    ::RandomFill<T, std::uniform_real_distribution<T>>(dists, std::forward<ArgsT>(args)...);
  }
  if constexpr(!std::is_floating_point_v<T>)
  {
    std::vector<std::uniform_int_distribution<T>> dists;
    for(usize comp = 0; comp < numComp; comp++)
    {
      dists.emplace_back(range.at(comp), range.at(comp + 1));
    }
    ::RandomFill<T, std::uniform_int_distribution<T>>(dists, std::forward<ArgsT>(args)...);
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
      return ::ValueFill<T>(dataArray, inputValues.stringValues);
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataArray, inputValues.startValues, inputValues.stepValues);
    }
    case InitializeType::Random: {
      std::vector<T> range;
      for(usize comp = 0; comp < numComp; comp++)
      {
        range.push_back(std::numeric_limits<T>::min());
        range.push_back(std::numeric_limits<T>::max());
      }
      return ::FillRandomForwarder<T>(range, numComp, dataArray, inputValues.seed);
    }
    case InitializeType::RangedRandom: {
      std::vector<T> range;
      for(usize comp = 0; comp < numComp; comp++)
      {
        Result<T> result = ConvertTo<T>::convert(inputValues.randBegin[comp]);
        range.push_back(result.value());
        result = ConvertTo<T>::convert(inputValues.randEnd[comp]);
        range.push_back(result.value());
      }
      return ::FillRandomForwarder<T>(range, numComp, dataArray, inputValues.seed);
    }
    }
  }
};

struct ValidateMultiInputFunctor
{
  template <typename T>
  PreflightResult operator()(const usize expectedComp, const std::string& unfilteredStr)
  {
    try
    {
      std::vector<std::string> splitVals = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitValue_Key)), k_DelimiterChar);

      if(splitVals.size() != expectedComp)
      {
        return MakePreflightErrorResult(-11610, fmt::format("Using '{}' as a delimiter we are unable to break '{}' into the required {} components." k_DelimiterChar, unfilteredStr, expectedComp));
      }

      for(usize comp = 0; comp < expectedComp; comp++)
      {
        Result<T> result = ConvertTo<T>::convert(splitVals[comp]);

        if(result.invalid())
        {
          return MakePreflightErrorResult(-11611, fmt::format("Unable to process '{}' into a {} value.", splitVals[comp], DataTypeToString(GetDataType<T>())));
        }
      }
    } catch(const std::exception& e)
    {
      return MakePreflightErrorResult(-11612, fmt::format("While processing '{}' into {} components the following error was encountered: {}", unfilteredStr, expectedComp, e.what()));
    }

    return {};
  }
};
} // namespace

namespace complex
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

  // TODO: restrict types
  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayPath_Key, "Any Component Array", "The data array in which to initialize the data", DataPath{}, complex::GetAllDataTypes()));

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

  /* Random - Using Random With Range */
  params.linkParameters(k_InitType_Key, k_UseSeed_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_SeedValue_Key, static_cast<ChoicesParameter::ValueType>(3));
  params.linkParameters(k_InitType_Key, k_InitRange_Key, static_cast<ChoicesParameter::ValueType>(3));

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

  complex::Result<OutputActions> resultOutputActions;

  auto& iDataArray = data.getDataRefAs<IDataArray>(args.value<DataPath>(k_ArrayPath_Key));
  usize numComp = iDataArray.getNumberOfComponents(); // check that the values string is greater than max comps

  switch(initializeTypeValue)
  {
  case InitializeType::FillValue: {
    auto result = ExecuteNeighborFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitValue_Key)); // NO BOOL
    if(result.invalid())
    {
      return result;
    }
    break;
  }
  case InitializeType::Incremental: {
    auto result = ExecuteNeighborFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_StartingFillValue_Key)); // NO BOOL
    if(result.invalid())
    {
      return result;
    }

    result = ExecuteNeighborFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_StepValue_Key)); // NO BOOL
    if(result.invalid())
    {
      return result;
    }
    break;
  }
  case InitializeType::Random: {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));

    break;
  }
  case InitializeType::RangedRandom: {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));

    auto result = ExecuteNeighborFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitStartRange_Key)); // NO BOOL
    if(result.invalid())
    {
      return result;
    }

    result = ExecuteNeighborFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, args.value<std::string>(k_InitEndRange_Key)); // NO BOOL
    if(result.invalid())
    {
      return result;
    }

    break;
  }
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> InitializeData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto seed = args.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!args.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  data.getDataRefAs<UInt64Array>(DataPath({args.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  InitializeDataInputValues inputValues;

  inputValues.initType = static_cast<InitializeType>(args.value<uint64>(k_InitType_Key));
  inputValues.stepType = static_cast<StepType>(args.value<uint64>(k_StepOperation_Key));
  inputValues.stringValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitValue_Key)), k_DelimiterChar);
  inputValues.startValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_StartingFillValue_Key)), k_DelimiterChar);
  inputValues.stepValues = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_StepValue_Key)), k_DelimiterChar);
  inputValues.seed = seed;
  inputValues.randBegin = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitStartRange_Key)), k_DelimiterChar);
  inputValues.randEnd = StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_InitEndRange_Key)), k_DelimiterChar);

  auto& iDataArray = data.getDataRefAs<IDataArray>(args.value<DataPath>(k_ArrayPath_Key));

  ExecuteNeighborFunction(::FillArrayFunctor{}, iDataArray.getDataType(), iDataArray, inputValues); // NO BOOL

  return {};
}
} // namespace complex
