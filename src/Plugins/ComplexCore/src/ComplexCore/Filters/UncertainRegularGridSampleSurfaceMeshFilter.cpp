#include "UncertainRegularGridSampleSurfaceMeshFilter.hpp"

#include "ComplexCore/Filters/Algorithms/UncertainRegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::name() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::className() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid UncertainRegularGridSampleSurfaceMeshFilter::uuid() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::humanName() const
{
  return "Sample Triangle Geometry on Uncertain Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> UncertainRegularGridSampleSurfaceMeshFilter::defaultTags() const
{
  return {"Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters UncertainRegularGridSampleSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{},
                                                          complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<Int32Parameter>(k_XPoints_Key, "X Points", "", 0 #error Check default values));
  params.insert(std::make_unique<Int32Parameter>(k_YPoints_Key, "Y Points", "", 0 #error Check default values));
  params.insert(std::make_unique<Int32Parameter>(k_ZPoints_Key, "Z Points", "", 0 #error Check default values));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>{0.0F, 0.0F, 0.0F},
                                                         std::vector<std::string>{"label1", "label2", "label3"} #error Check default values));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"label1", "label2", "label3"} #error Check default values));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Uncertainty_Key, "Uncertainty", "", std::vector<float32>{0.0F, 0.0F, 0.0F},
                                                         std::vector<std::string>{"label1", "label2", "label3"} #error Check default values));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer UncertainRegularGridSampleSurfaceMeshFilter::clone() const
{
  return std::make_unique<UncertainRegularGridSampleSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult UncertainRegularGridSampleSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pXPointsValue = filterArgs.value<int32>(k_XPoints_Key);
  auto pYPointsValue = filterArgs.value<int32>(k_YPoints_Key);
  auto pZPointsValue = filterArgs.value<int32>(k_ZPoints_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

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
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{

  UncertainRegularGridSampleSurfaceMeshInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.XPoints = filterArgs.value<int32>(k_XPoints_Key);
  inputValues.YPoints = filterArgs.value<int32>(k_YPoints_Key);
  inputValues.ZPoints = filterArgs.value<int32>(k_ZPoints_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Uncertainty = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Uncertainty_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  return UncertainRegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
