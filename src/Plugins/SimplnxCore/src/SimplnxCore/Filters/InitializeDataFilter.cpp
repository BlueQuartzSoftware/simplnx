#include "InitializeDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/InitializeData.hpp"

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

#include <random>
#include <sstream>
#include <thread>

using namespace nx::core;

namespace
{

}

namespace nx::core
{
//------------------------------------------------------------------------------
std::string InitializeDataFilter::name() const
{
  return FilterTraits<InitializeDataFilter>::name;
}

//------------------------------------------------------------------------------
std::string InitializeDataFilter::className() const
{
  return FilterTraits<InitializeDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeDataFilter::uuid() const
{
  return FilterTraits<InitializeDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeDataFilter::humanName() const
{
  return "Initialize Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeDataFilter::defaultTags() const
{
  return {className(), "Memory Management", "Initialize", "Create", "Generate", "Data"};
}

//------------------------------------------------------------------------------
Parameters InitializeDataFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Method for determining the what values of the data in the array should be initialized to",
                                                                    static_cast<ChoicesParameter::ValueType>(0),
                                                                    ChoicesParameter::Choices{"Fill Value", "Incremental", "Random", "Random With Range"})); // sequence dependent DO NOT REORDER

  params.insert(std::make_unique<StringParameter>(k_InitValue_Key, "Fill Values [Seperated with ;]",
                                                  "Specify values for each component. Ex: A 3-component array would be 6;8;12 and every tuple would have these same component values", "1;1;1"));

  params.insert(std::make_unique<StringParameter>(k_StartingFillValue_Key, "Starting Value [Seperated with ;]", "The value to start incrementing from", "0;1;2"));
  params.insert(std::make_unique<ChoicesParameter>(k_StepOperation_Key, "Step Operation", "The type of step operation to preform", static_cast<ChoicesParameter::ValueType>(0),
                                                   ChoicesParameter::Choices{"Addition", "Subtraction"}));
  params.insert(std::make_unique<StringParameter>(k_StepValue_Key, "Step Value [Seperated with ;]", "The number to add/subtract the fill value by", "1;1;1"));

  params.insert(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the Seed Value will be used to seed the generator", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeDataFilter SeedValue"));
  params.insert(std::make_unique<BoolParameter>(k_StandardizeSeed_Key, "Use the Same Seed for Each Component",
                                                "When true the same seed will be used for each component's generator in a multi-component array", false));

  params.insert(
      std::make_unique<StringParameter>(k_InitStartRange_Key, "Initialization Start Range [Seperated with ;]", "[Inclusive] The lower bound initialization range for random values", "0;0;0"));
  params.insert(std::make_unique<StringParameter>(k_InitEndRange_Key, "Initialization End Range [Seperated with ;]", "[Inclusive]  The upper bound initialization range for random values", "1;1;1"));

  params.insertSeparator(Parameters::Separator{"Input Data Array"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ArrayPath_Key, "Any Component Array", "The data array in which to initialize the data", DataPath{}, nx::core::GetAllDataTypes()));

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
IFilter::VersionType InitializeDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeDataFilter::clone() const
{
  return std::make_unique<InitializeDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto seedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);
  auto initializeTypeValue = static_cast<InitializeType>(filterArgs.value<uint64>(k_InitType_Key));
  auto initFillValue = filterArgs.value<std::string>(k_InitValue_Key);
  auto initIncFillValue = filterArgs.value<std::string>(k_StartingFillValue_Key);
  auto stepValue = filterArgs.value<std::string>(k_StepValue_Key);
  auto stepOperation = filterArgs.value<ChoicesParameter::ValueType>(k_StepOperation_Key);
  auto initStartRange = filterArgs.value<std::string>(k_InitStartRange_Key);
  auto initEndRange = filterArgs.value<std::string>(k_InitEndRange_Key);
  auto standardizeSeed = filterArgs.value<bool>(k_StandardizeSeed_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto& iDataArray = dataStructure.getDataRefAs<IDataArray>(filterArgs.value<DataPath>(k_ArrayPath_Key));
  usize numComp = iDataArray.getNumberOfComponents(); // check that the values string is greater than max comps

  if(iDataArray.getDataType() == DataType::boolean)
  {
    std::stringstream ss;

    ss << "We detected that you are doing an operation on a boolean array.\n";
    ss << "The ONLY two ways to specify a 'false' boolean value are as follows:\n";
    ss << "- boolean value string types as follows ignoring apostrophe marks: 'False', 'FALSE', 'false'\n";
    ss << "- all well formed integers and well formed floating point definitions of 0\n\n";

    ss << "ANY OTHER string or number WILL BE 'true', although it is good practice to define true values as follows:\n";
    ss << "- boolean value string types as follows ignoring apostrophe marks: 'True', 'TRUE', 'true'\n";
    ss << "- all well formed integers and well formed floating point definitions of 1";

    preflightUpdatedValues.push_back({"Boolean Note", ss.str()});
  }

  usize numTuples = iDataArray.getNumberOfTuples();

  switch(initializeTypeValue)
  {
  case InitializeType::FillValue: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, filterArgs.value<std::string>(k_InitValue_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    CreateFillPreflightVals(initFillValue, numComp, preflightUpdatedValues);

    break;
  }
  case InitializeType::Incremental: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, filterArgs.value<std::string>(k_StartingFillValue_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    if(iDataArray.getDataType() == DataType::boolean)
    {
      // custom bool checks here
      std::stringstream ss;

      ss << "A boolean array is being initialized so please enter unsigned 8-bit integer values, preferably a 0 or 1 only.\n";

      switch(static_cast<StepType>(filterArgs.value<uint64>(k_StepOperation_Key)))
      {
      case Addition: {
        ss << "You have currently selected the addition operation.\nAny step value that is greater than 0 will cause all values to be 'true' after the first tuple, 'true' "
              "values will remain unchanged.\n";
        ss << "The two possibilities:\n";
        ss << "- If your start value is 'false' and step value > 0, the array will initialize to | false | true | true | ... |\n";
        ss << "- If your start value is 'true' and step value > 0, the array will initialize to | true | true | true | ... |";
        break;
      }
      case Subtraction: {
        ss << "You have currently selected the subtraction operation.\nAny step value that is greater than 0 will cause all values to be 'false' after the first tuple, 'false' "
              "values will remain unchanged.\n";
        ss << "The two possibilities:\n";
        ss << "- If your start value is 'true' and step value > 0, the array will initialize to | true | false | false | ... |\n";
        ss << "- If your start value is 'false' and step value > 0, the array will initialize to | false | false | false | ... |";
        break;
      }
      }

      preflightUpdatedValues.push_back({"Boolean Incremental Nuances", ss.str()});

      result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, DataType::uint8, numComp, filterArgs.value<std::string>(k_StepValue_Key), 1);
      if(result.outputActions.invalid())
      {
        return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
      }
    }
    else
    {
      result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, filterArgs.value<std::string>(k_StepValue_Key), 1);
      if(result.outputActions.invalid())
      {
        return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
      }
    }

    CreateIncrementalPreflightVals(initIncFillValue, stepOperation, stepValue, numTuples, numComp, preflightUpdatedValues);

    break;
  }
  case InitializeType::RangedRandom: {
    auto result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, filterArgs.value<std::string>(k_InitStartRange_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    result = ExecuteDataFunction(::ValidateMultiInputFunctor{}, iDataArray.getDataType(), numComp, filterArgs.value<std::string>(k_InitEndRange_Key), 1);
    if(result.outputActions.invalid())
    {
      return {MergeResults(result.outputActions, std::move(resultOutputActions)), std::move(preflightUpdatedValues)};
    }

    [[fallthrough]];
  }
  case InitializeType::Random: {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({seedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));

    CreateRandomPreflightVals(standardizeSeed, initializeTypeValue, initStartRange, initEndRange, numTuples, numComp, preflightUpdatedValues);

    break;
  }
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> InitializeDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto initType = static_cast<InitializeType>(filterArgs.value<uint64>(k_InitType_Key));

  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  if(initType == InitializeType::Random || initType == InitializeType::RangedRandom)
  {
    // Store Seed Value in Top Level Array
    dataStructure.getDataRefAs<UInt64Array>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;
  }

  InitializeDataInputValues inputValues;
  inputValues.InputArrayPath = filterArgs.value<DataPath>(k_ArrayPath_Key);
  inputValues.initType = initType;
  inputValues.stepType = static_cast<StepType>(filterArgs.value<uint64>(k_StepOperation_Key));
  inputValues.stringValues = StringUtilities::split(StringUtilities::trimmed(filterArgs.value<std::string>(k_InitValue_Key)), k_DelimiterChar);
  inputValues.startValues = StringUtilities::split(StringUtilities::trimmed(filterArgs.value<std::string>(k_StartingFillValue_Key)), k_DelimiterChar);
  inputValues.stepValues = StringUtilities::split(StringUtilities::trimmed(filterArgs.value<std::string>(k_StepValue_Key)), k_DelimiterChar);
  inputValues.seed = seed;
  inputValues.randBegin = StringUtilities::split(StringUtilities::trimmed(filterArgs.value<std::string>(k_InitStartRange_Key)), k_DelimiterChar);
  inputValues.randEnd = StringUtilities::split(StringUtilities::trimmed(filterArgs.value<std::string>(k_InitEndRange_Key)), k_DelimiterChar);
  inputValues.standardizeSeed = filterArgs.value<bool>(k_StandardizeSeed_Key);

  return InitializeData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
