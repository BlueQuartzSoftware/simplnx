#include "CreateAMScanPathsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateAMScanPaths.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::name() const
{
  return FilterTraits<CreateAMScanPathsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::className() const
{
  return FilterTraits<CreateAMScanPathsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateAMScanPathsFilter::uuid() const
{
  return FilterTraits<CreateAMScanPathsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::humanName() const
{
  return "Generate Scan Vectors";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateAMScanPathsFilter::defaultTags() const
{
  return {className(), "AFRLDistributionC", "Scan Path"};
}

//------------------------------------------------------------------------------
Parameters CreateAMScanPathsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_StripeWidth_Key, "Vector Width", "", 1.0f));
  params.insert(std::make_unique<Float32Parameter>(k_HatchSpacing_Key, "Vector Spacing", "", 1.0f));
  params.insert(std::make_unique<Float32Parameter>(k_Power_Key, "Power", "", 10.0f));
  params.insert(std::make_unique<Float32Parameter>(k_Speed_Key, "Speed", "", 20.0f));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CADSliceDataContainerName_Key, "Slice Data Container", "", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::TriangleGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADSliceIdsArrayPath_Key, "Slice Ids", "", DataPath{},
                                                          nx::core::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADRegionIdsArrayPath_Key, "Region Ids", "", DataPath{},
                                                          nx::core::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<StringParameter>(k_HatchDataContainerName_Key, "Scan Vector Data Container", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vector Node Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_HatchAttributeMatrixName_Key, "Vector Attribute Matrix", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vector Node Data"});
  params.insert(std::make_unique<StringParameter>(k_TimeArrayName_Key, "Times", "", "SomeString"));
  params.insertSeparator(Parameters::Separator{"Vector Data"});
  params.insert(std::make_unique<StringParameter>(k_RegionIdsArrayName_Key, "Region Ids", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_PowersArrayName_Key, "Powers", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateAMScanPathsFilter::clone() const
{
  return std::make_unique<CreateAMScanPathsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateAMScanPathsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pStripeWidthValue = filterArgs.value<float32>(k_StripeWidth_Key);
  auto pHatchSpacingValue = filterArgs.value<float32>(k_HatchSpacing_Key);
  auto pPowerValue = filterArgs.value<float32>(k_Power_Key);
  auto pSpeedValue = filterArgs.value<float32>(k_Speed_Key);
  auto pCADSliceDataContainerNameValue = filterArgs.value<DataPath>(k_CADSliceDataContainerName_Key);
  auto pCADSliceIdsArrayPathValue = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  auto pCADRegionIdsArrayPathValue = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  auto pHatchDataContainerNameValue = filterArgs.value<StringParameter::ValueType>(k_HatchDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pHatchAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  auto pTimeArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_TimeArrayName_Key);
  auto pRegionIdsArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_RegionIdsArrayName_Key);
  auto pPowersArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_PowersArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  nx::core::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateAMScanPathsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{

  CreateAMScanPathsInputValues inputValues;

  inputValues.StripeWidth = filterArgs.value<float32>(k_StripeWidth_Key);
  inputValues.HatchSpacing = filterArgs.value<float32>(k_HatchSpacing_Key);
  inputValues.Power = filterArgs.value<float32>(k_Power_Key);
  inputValues.Speed = filterArgs.value<float32>(k_Speed_Key);
  inputValues.CADSliceDataContainerName = filterArgs.value<DataPath>(k_CADSliceDataContainerName_Key);
  inputValues.CADSliceIdsArrayPath = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  inputValues.CADRegionIdsArrayPath = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  inputValues.HatchDataContainerName = filterArgs.value<StringParameter::ValueType>(k_HatchDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.HatchAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  inputValues.TimeArrayName = filterArgs.value<StringParameter::ValueType>(k_TimeArrayName_Key);
  inputValues.RegionIdsArrayName = filterArgs.value<StringParameter::ValueType>(k_RegionIdsArrayName_Key);
  inputValues.PowersArrayName = filterArgs.value<StringParameter::ValueType>(k_PowersArrayName_Key);

  return CreateAMScanPaths(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{

} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> CreateAMScanPathsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateAMScanPathsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
