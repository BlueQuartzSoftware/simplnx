#include "FindFeatureCentroidsFilter.hpp"

#include "Core/Filters/Algorithms/FindFeatureCentroids.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataGroup.hpp"
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
std::string FindFeatureCentroidsFilter::name() const
{
  return FilterTraits<FindFeatureCentroidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindFeatureCentroidsFilter::className() const
{
  return FilterTraits<FindFeatureCentroidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindFeatureCentroidsFilter::uuid() const
{
  return FilterTraits<FindFeatureCentroidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindFeatureCentroidsFilter::humanName() const
{
  return "Find Feature Centroids";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindFeatureCentroidsFilter::defaultTags() const
{
  return {"#Generic", "#Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindFeatureCentroidsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry whose Features' centroids will be calculated", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrix_Key, "Cell Feature Attribute Matrix", "The cell feature attribute matrix",
                                                                    DataPath({"Data Container", "Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentroidsArrayPath_Key, "Centroids", "DataPath to create the 'Centroids' output array", "Centroids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindFeatureCentroidsFilter::clone() const
{
  return std::make_unique<FindFeatureCentroidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindFeatureCentroidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto featureAttrMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);

  auto pCentroidsArrayName = filterArgs.value<std::string>(k_CentroidsArrayPath_Key);
  DataPath pCentroidsArrayPath = featureAttrMatrixPath.createChildPath(pCentroidsArrayName);

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

  // Feature Data:
  // Validating the Feature Attribute Matrix and trying to find a child of the Group
  // that is an IDataArray subclass, so we can get the proper tuple shape
  const auto* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12700, fmt::format("Cannot find the selected feature Attribute Matrix at path '{}'", featureAttrMatrixPath.toString())}})};
  }
  IDataStore::ShapeType tupleShape = featureAttrMatrix->getShape();

  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pCentroidsArrayPath);
    resultOutputActions.value().actions.push_back(std::move(createFeatureCentroidsAction));
  }
  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindFeatureCentroidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  FindFeatureCentroidsInputValues inputValues;

  auto pCentroidsArrayName = filterArgs.value<std::string>(k_CentroidsArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrix_Key);
  inputValues.CentroidsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pCentroidsArrayName);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindFeatureCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
