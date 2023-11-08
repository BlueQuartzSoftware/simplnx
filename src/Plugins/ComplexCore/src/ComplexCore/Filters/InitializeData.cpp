#include "InitializeData.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/AbstractDataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <fmt/core.h>

#include <array>
#include <chrono>
#include <limits>
#include <random>
#include <thread>

using namespace complex;

namespace
{
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
  std::string initValue;
  std::string startValue;
  std::string stepValue;
  uint64 seed;
  std::vector<float64> range;
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
void ValueFill(DataArray<T>& dataArray, const std::string& initValue)
{
  Result<T> result = ConvertTo<T>::convert(initValue);
  T value = result.value();
  dataArray.fill(value);
}

template <typename T, class IncrementalOptions = AdditionT>
void IncrementalFill(DataArray<T>& dataArray, const std::string& startValue, const std::string& stepValue)
{
  Result<T> result = ConvertTo<T>::convert(startValue);
  T value = result.value();
  result = ConvertTo<T>::convert(stepValue);
  T step = result.value();

  for(auto& cell : dataArray)
  {
    cell = value;

    if constexpr(IncrementalOptions::UsingAddition)
    {
      value += step;
    }
    if constexpr(IncrementalOptions::UsingSubtraction)
    {
      value -= step;
    }
  }
}

template <typename T, class DistributionT>
void RandomFill(DistributionT& dist, DataArray<T>& dataArray, const uint64 seed)
{
  std::mt19937_64 gen(seed);

  for(auto& cell : dataArray)
  {
    cell = dist(gen);
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
void FillRandomForwarder(const std::vector<float64>& range, ArgsT&&... args)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<T> dist(static_cast<T>(range.at(0)), static_cast<T>(range.at(1)));
    ::RandomFill<T, std::uniform_real_distribution<T>>(dist, std::forward<ArgsT>(args)...);
  }
  if constexpr(!std::is_floating_point_v<T>)
  {
    std::uniform_int_distribution<T> dist(static_cast<T>(range.at(0)), static_cast<T>(range.at(1)));
    ::RandomFill<T, std::uniform_int_distribution<T>>(dist, std::forward<ArgsT>(args)...);
  }
}

struct FillArrayFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const InitializeDataInputValues& inputValues, const DataPath& path)
  {
    auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(path);

    switch(inputValues.initType)
    {
    case InitializeType::FillValue: {
      return ::ValueFill<T>(dataArray, inputValues.initValue);
    }
    case InitializeType::Incremental: {
      return ::FillIncForwarder<T>(inputValues.stepType, dataArray, inputValues.startValue, inputValues.stepValue);
    }
    case InitializeType::Random: {
      return ::FillRandomForwarder<T>({std::numeric_limits<float64>::min(), std::numeric_limits<float64>::max()}, dataArray, inputValues.seed);
    }
    case InitializeType::RangedRandom: {
      return ::FillRandomForwarder<T>(inputValues.range, dataArray, inputValues.seed);
    }
    }
  }
};

template <typename T>
Result<> FillMultiComponentArray(DataArray<T>& dataArray, const std::vector<std::string>& stringValues)
{
  std::vector<T> values;

  for(const auto& str : stringValues)
  {
    auto result = ConvertTo<T>::convert(str);
    if(result.invalid())
    {
      return MakeErrorResult(fmt::format("{} was unable to be translated to the data arrays type", str));
    }
    values.emplace_back(ConvertTo<T>::convert(str).value())
  }

  usize numComp = dataArray.getNumberOfComponents();

  if(numComp > )

}
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
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMultiCompArrays_Key, "Use Multiple Component Arrays", "When true options for multiple component arrays will be visible", false));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SingleCompArrayPaths_Key, "Single Component Arrays", "The data arrays in which to initialize the data", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllNumericTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_MultiCompArrayPaths_Key, "Multi Component Arrays", "The data arrays in which to initialize the data", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Data Initialization"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Method for detemining the what values of the data in the array should be initialized to",
                                                                    0ULL, ChoicesParameter::Choices{"Fill Value", "Incremental", "Random", "Random With Range"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<StringParameter>(k_InitValue_Key, "Initialization Value", "This value will be used to fill the new array", "0"));
  params.insert(std::make_unique<StringParameter>(k_MultiFillValue_Key, "Multi Component Fill Values [Seperated with ;]",
                                                  "Specify values for each component. Ex: A 3-component array would be 6;8;12 and every tuple would have these same component values", "1;1;1"));

  params.insert(std::make_unique<StringParameter>(k_StartingFillValue_Key, "Starting Value", "The value to start incrementing from", "0"));
  params.insert(std::make_unique<ChoicesParameter>(k_StepOperation_Key, "Starting Value", "The type of step operation to preform", 0ULL, ChoicesParameter::Choices{"Addition", "Subtraction"}));
  params.insert(std::make_unique<StringParameter>(k_StepValue_Key, "Increment/Step Value", "The number to increment/decrement the fill value by", "1"));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeData SeedValue"));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_InitRange_Key, "Initialization Range", "The initialization range if Random With Range Initialization Type is selected",
                                                         VectorFloat64Parameter::ValueType{0.0, 0.0}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  /* Using Multiple Components */
  params.linkParameters(k_UseMultiCompArrays_Key, k_MultiCompArrayPaths_Key, true);
  params.linkParameters(k_UseMultiCompArrays_Key, k_SingleCompArrayPaths_Key, false);
  params.linkParameters(k_UseMultiCompArrays_Key, k_MultiFillValue_Key, true);

  /* Using Fill Value */
  params.linkParameters(k_InitType_Key, k_InitValue_Key, 0ULL);

  /* Using Incremental */
  params.linkParameters(k_InitType_Key, k_StartingFillValue_Key, 1ULL);
  params.linkParameters(k_InitType_Key, k_StepOperation_Key, 1ULL);
  params.linkParameters(k_InitType_Key, k_StepValue_Key, 1ULL);

  /* Random */
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);
  /* Random - Using Random */
  params.linkParameters(k_InitType_Key, k_UseSeed_Key, 2ULL);
  /* Random - Using Random With Range */
  params.linkParameters(k_InitType_Key, k_UseSeed_Key, 3ULL);
  params.linkParameters(k_InitType_Key, k_InitRange_Key, 3ULL);

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
  auto cellArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto initRangeVec = args.value<std::vector<float64>>(k_InitRange_Key);
  auto seedArrayNameValue = args.value<std::string>(k_SeedArrayName_Key);
  auto initializeTypeValue = static_cast<InitializeType>(args.value<uint64>(k_InitType_Key));

  if(cellArrayPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(-5550, "At least one data array must be selected.")};
  }

if(multiCompArrays)
{
  std::vector<T> values;

  for(const auto& str : stringValues)
  {
    auto result = ConvertTo<T>::convert(str);
    if(result.invalid())
    {
      return MakePreflightErrorResult(fmt::format("{} was unable to be translated to the data arrays type", str));
    }
    values.emplace_back(ConvertTo<T>::convert(str).value())
  }

  usize numComp = dataArray.getNumberOfComponents();

  if(numComp > values.size() - 1)
  {
    MakePreflightErrorResult(fmt::format("one of the arrays ", str));
  }
}

  complex::Result<OutputActions> resultOutputActions;

  if(initializeTypeValue == InitializeType::Random)
  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
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
  StepType stepType = static_cast<StepType>(args.value<uint64>(k_StepOperation_Key));
  inputValues.initValue = args.value<std::string>(k_InitValue_Key);
  std::string startValue = args.value<std::string>(k_StartingFillValue_Key);
  std::string stepValue = args.value<std::string>(k_StepValue_Key);
  inputValues.seed = seed;
  inputValues.range = args.value<std::vector<float64>>(k_InitRange_Key);

  auto cellArrayPaths = args.value<bool>(k_UseMultiCompArrays_Key) ? args.value<MultiArraySelectionParameter::ValueType>(k_MultiCompArraysPaths_Key) :
                                                                     args.value<MultiArraySelectionParameter::ValueType>(k_SingleCompArraysPaths_Key);

  std::vector<std::string> cellArrayPaths = args.value<bool>(k_UseMultiCompArrays_Key) ? StringUtilities::split(StringUtilities::trimmed(args.value<std::string>(k_MultiFillValue_Key)), ';') : {""};

  for(const DataPath& path : cellArrayPaths)
  {
    auto& iDataArray = data.getDataRefAs<IDataArray>(path);

    if(iDataArray.getNumberOfComponents() > 1)
    {
      // Work out multi component arrays
    }
    else
    {
      ExecuteNeighborFunction(::FillArrayFunctor{}, iDataArray.getDataType(), data, inputValues, path); // NO BOOL

      // Avoid the exact same seeding for each array
      inputValues.seed++;
    }
  }

  return {};
}
} // namespace complex
