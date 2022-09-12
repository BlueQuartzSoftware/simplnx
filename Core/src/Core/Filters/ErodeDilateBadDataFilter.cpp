#include "ErodeDilateBadDataFilter.hpp"

#include "Core/Filters/Algorithms/ErodeDilateBadData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::name() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::className() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateBadDataFilter::uuid() const
{
  return FilterTraits<ErodeDilateBadDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateBadDataFilter::humanName() const
{
  return "Erode/Dilate Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateBadDataFilter::defaultTags() const
{
  return {"#Processing", "#Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateBadDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "", 0ULL, ::k_OperationChoices));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "", 2));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "", false));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "", false));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()}, MultiArraySelectionParameter::AllowedTypes{}));

  params.insertSeparator(Parameters::Separator{"Required Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedFeatureDataGroup_Key, "Feature Data Attribute Matrix", "Feature data Attribute Matrix", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateBadDataFilter::clone() const
{
  return std::make_unique<ErodeDilateBadDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  if(pOperationValue != ::k_ErodeIndex && pOperationValue != ::k_DilateIndex)
  {
    MakeErrorResult(-16700, fmt::format("Operation Selection must be 0 (Erode) or 1 (Dilate). {} was passed into the filter. ", pOperationValue));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  ErodeDilateBadDataInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.XDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.YDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.ZDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.FeatureDataPath = filterArgs.value<DataPath>(k_SelectedFeatureDataGroup_Key);

  return ErodeDilateBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
