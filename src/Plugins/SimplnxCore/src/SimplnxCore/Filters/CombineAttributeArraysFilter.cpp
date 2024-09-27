#include "CombineAttributeArraysFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CombineAttributeArrays.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::name() const
{
  return FilterTraits<CombineAttributeArraysFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::className() const
{
  return FilterTraits<CombineAttributeArraysFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineAttributeArraysFilter::uuid() const
{
  return FilterTraits<CombineAttributeArraysFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineAttributeArraysFilter::humanName() const
{
  return "Combine Attribute Arrays";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineAttributeArraysFilter::defaultTags() const
{
  return {className(), "Core",
          "Memory Management"
          "Combine",
          "Arrays"};
}

//------------------------------------------------------------------------------
Parameters CombineAttributeArraysFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<BoolParameter>(k_NormalizeData_Key, "Normalize Data", "Whether to normalize the combine data on the interval [0, 1]", false));
  params.insert(std::make_unique<BoolParameter>(k_MoveValues_Key, "Move Data", "Whether to remove the original arrays after combining the data", false));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Attribute Arrays to Combine", "The complete path to each of the Attribute Arrays to combine",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_StackedDataArrayName_Key, "Created Data Array", "This is the name of the created output array of the combined attribute arrays.",
                                                          "Combined DataArray"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CombineAttributeArraysFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineAttributeArraysFilter::clone() const
{
  return std::make_unique<CombineAttributeArraysFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineAttributeArraysFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  // auto normalizeDataValue = filterArgs.value<bool>(k_NormalizeData_Key);
  auto moveValuesValue = filterArgs.value<bool>(k_MoveValues_Key);
  auto selectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto stackedDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_StackedDataArrayName_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(selectedDataArrayPathsValue.empty())
  {
    return {MakeErrorResult<OutputActions>(-66005, "Please select arrays to combine")};
  }

  bool result = nx::core::CheckArraysAreSameType(dataStructure, selectedDataArrayPathsValue);
  if(!result)
  {
    return {MakeErrorResult<OutputActions>(-66006, "All selected arrays must be of the same type")};
  }
  result = nx::core::CheckArraysHaveSameTupleCount(dataStructure, selectedDataArrayPathsValue);
  if(!result)
  {
    return {MakeErrorResult<OutputActions>(-66007, "All selected arrays must have the same number of tuples")};
  }

  // Figure out the total number of components
  size_t numComps = 0;
  for(const auto& dataPath : selectedDataArrayPathsValue)
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
    numComps += dataArray->getNumberOfComponents();
  }

  // Create the output array
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(selectedDataArrayPathsValue[0]);
    auto tupleShape = dataArray->getTupleShape();
    auto action = std::make_unique<CreateArrayAction>(dataArray->getDataType(), tupleShape, std::vector<usize>{numComps}, selectedDataArrayPathsValue[0].replaceName(stackedDataArrayName));
    resultOutputActions.value().appendAction(std::move(action));
  }

  // If we are MOVING the data arrays, then we need to delete the data arrays at
  // the end of the execute method.
  if(moveValuesValue)
  {
    for(const auto& dataPath : selectedDataArrayPathsValue)
    {
      auto action = std::make_unique<DeleteDataAction>(dataPath, DeleteDataAction::DeleteType::JustObject);
      resultOutputActions.value().appendDeferredAction(std::move(action));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CombineAttributeArraysFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  CombineAttributeArraysInputValues inputValues;

  inputValues.NormalizeData = filterArgs.value<bool>(k_NormalizeData_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);

  inputValues.StackedDataArrayPath = inputValues.SelectedDataArrayPaths[0].replaceName(filterArgs.value<DataObjectNameParameter::ValueType>(k_StackedDataArrayName_Key));

  return CombineAttributeArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_NormalizeDataKey = "NormalizeData";
constexpr StringLiteral k_MoveValuesKey = "MoveValues";
constexpr StringLiteral k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
constexpr StringLiteral k_StackedDataArrayNameKey = "StackedDataArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> CombineAttributeArraysFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CombineAttributeArraysFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_NormalizeDataKey, k_NormalizeData_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_MoveValuesKey, k_MoveValues_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_StackedDataArrayNameKey, k_StackedDataArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
