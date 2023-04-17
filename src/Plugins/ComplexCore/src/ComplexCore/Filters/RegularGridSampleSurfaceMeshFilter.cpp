#include "RegularGridSampleSurfaceMeshFilter.hpp"

#include "ComplexCore/Filters/Algorithms/RegularGridSampleSurfaceMesh.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <sstream>

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::name() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::className() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RegularGridSampleSurfaceMeshFilter::uuid() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::humanName() const
{
  return "Sample Triangle Geometry on Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> RegularGridSampleSurfaceMeshFilter::defaultTags() const
{
  return {"Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters RegularGridSampleSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "", std::vector<int32>{0, 0, 0}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Units (For Description Only)", "", 0,
                                                   ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"} /* Change this to the proper choices */));

  params.insertSeparator(Parameters::Separator{"Required Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}, ArraySelectionParameter::AllowedTypes{complex::DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Data Container", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RegularGridSampleSurfaceMeshFilter::clone() const
{
  return std::make_unique<RegularGridSampleSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RegularGridSampleSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;

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

  std::vector<PreflightValue> preflightUpdatedValues;
  std::stringstream boxDimensions = std::stringstream();

  std::string lengthUnit = IGeometry::LengthUnitToString(static_cast<IGeometry::LengthUnit>(pLengthUnitValue));

  boxDimensions << "X Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[0] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[0] + (pDimensionsValue[0] * pSpacingValue[0])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[0] * pSpacingValue[0]) << ") "
                << lengthUnit << "\n";
  boxDimensions << "Y Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[1] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[1] + (pDimensionsValue[1] * pSpacingValue[1])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[1] * pSpacingValue[1]) << ") "
                << lengthUnit << "\n";
  boxDimensions << "Z Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[2] << " to " << std::setprecision(8) << std::noshowpoint
                << (pOriginValue[2] + (pDimensionsValue[2] * pSpacingValue[2])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[2] * pSpacingValue[2]) << ") "
                << lengthUnit << "\n";

  float vol = (pDimensionsValue[0] * pSpacingValue[0]) * (pDimensionsValue[1] * pSpacingValue[1]) * (pDimensionsValue[2] * pSpacingValue[2]);

  boxDimensions << "Volume: " << std::setprecision(8) << std::noshowpoint << vol << " " << lengthUnit << "s ^3"
                << "\n";

  preflightUpdatedValues.push_back({"BoxDimensions", boxDimensions.str()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  RegularGridSampleSurfaceMeshInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);

  return RegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
