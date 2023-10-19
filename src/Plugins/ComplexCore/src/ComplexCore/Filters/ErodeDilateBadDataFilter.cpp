#include "ErodeDilateBadDataFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ErodeDilateBadData.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

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
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateBadDataFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate or erode", 0ULL, ::k_OperationChoices));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "The number of iterations to use for erosion/dilation", 2));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{DataPath(), DataPath(), DataPath()},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, MultiArraySelectionParameter::AllowedDataTypes{}));

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
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(pOperationValue != ::k_DilateIndex && pOperationValue != ::k_ErodeIndex)
  {
    MakeErrorResult(-16700, fmt::format("Operation Selection must be 0 (Dilate) or 1 (Erode). {} was passed into the filter. ", pOperationValue));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  complex::AppendDataModifiedActions(dataStructure, resultOutputActions.value().modifiedActions, pFeatureIdsArrayPathValue.getParent(), pIgnoredDataArrayPathsValue);

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
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return ErodeDilateBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
