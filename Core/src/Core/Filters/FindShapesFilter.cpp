#include "FindShapesFilter.hpp"

#include "Core/Filters/Algorithms/FindShapes.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindShapesFilter::name() const
{
  return FilterTraits<FindShapesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindShapesFilter::className() const
{
  return FilterTraits<FindShapesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindShapesFilter::uuid() const
{
  return FilterTraits<FindShapesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindShapesFilter::humanName() const
{
  return "Find Feature Shapes";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindShapesFilter::defaultTags() const
{
  return {"#Statistics", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindShapesFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "", DataPath({"FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::ComponentTypes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Feature Centroids", "", DataPath({"Centroids"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::ComponentTypes{{3}}));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega3sArrayName_Key, "Omega3s", "", "Omega3s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisLengthsArrayName_Key, "Axis Lengths", "", "AxisLengths"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisEulerAnglesArrayName_Key, "Axis Euler Angles", "", "AxisEulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AspectRatiosArrayName_Key, "Aspect Ratios", "", "AspectRatios"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesArrayName_Key, "Volumes", "", "Shape Volumes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindShapesFilter::clone() const
{
  return std::make_unique<FindShapesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindShapesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pOmega3sArrayName = filterArgs.value<std::string>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayName = filterArgs.value<std::string>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayName = filterArgs.value<std::string>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayName = filterArgs.value<std::string>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayName = filterArgs.value<std::string>(k_VolumesArrayName_Key);
  auto featureAttrMatrixPath = pCentroidsArrayPath.getParent();
  auto pOmega3sArrayPath = featureAttrMatrixPath.createChildPath(pOmega3sArrayName);
  auto pAxisLengthsArrayPath = featureAttrMatrixPath.createChildPath(pAxisLengthsArrayName);
  auto pAxisEulerAnglesArrayPath = featureAttrMatrixPath.createChildPath(pAxisEulerAnglesArrayName);
  auto pAspectRatiosArrayPath = featureAttrMatrixPath.createChildPath(pAspectRatiosArrayName);
  auto pVolumesArrayPath = featureAttrMatrixPath.createChildPath(pVolumesArrayName);

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

  const AttributeMatrix* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12801, fmt::format("Could not find selected cell feature Attibute Matrix at path '{}'", featureAttrMatrixPath.toString())}})};
  }

  // Get the Centroids Feature Array and get its TupleShape
  const auto* centroids = dataStructure.getDataAs<Float32Array>(pCentroidsArrayPath);
  if(nullptr == centroids)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12802, "Centroids Feature Data Array is not of the correct type"}})};
  }

  IDataStore::ShapeType tupleShape = featureAttrMatrix->getShape();

  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1ULL}, pOmega3sArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pAxisLengthsArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pAxisEulerAnglesArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{2ULL}, pAspectRatiosArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1ULL}, pVolumesArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindShapesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  FindShapesInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = inputValues.CentroidsArrayPath.getParent();
  auto pOmega3sArrayName = filterArgs.value<std::string>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayName = filterArgs.value<std::string>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayName = filterArgs.value<std::string>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayName = filterArgs.value<std::string>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayName = filterArgs.value<std::string>(k_VolumesArrayName_Key);
  inputValues.Omega3sArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pOmega3sArrayName);
  inputValues.AxisLengthsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pAxisLengthsArrayName);
  inputValues.AxisEulerAnglesArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pAxisEulerAnglesArrayName);
  inputValues.AspectRatiosArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pAspectRatiosArrayName);
  inputValues.VolumesArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pVolumesArrayName);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
