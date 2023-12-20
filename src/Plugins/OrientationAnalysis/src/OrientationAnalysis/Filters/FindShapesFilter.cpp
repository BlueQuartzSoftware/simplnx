#include "FindShapesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindShapes.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
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
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindShapesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Feature Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath({"Centroids"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Created Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_Omega3sArrayName_Key, "Omega3s",
                                                          "3rd invariant of the second-order moment matrix for the Feature, does not assume a shape type (i.e., ellipsoid)", "Omega3s"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AxisLengthsArrayName_Key, "Axis Lengths", "Semi-axis lengths (a, b, c) for best-fit ellipsoid to Feature", "AxisLengths"));
  params.insert(std::make_unique<DataObjectNameParameter>(
      k_AxisEulerAnglesArrayName_Key, "Axis Euler Angles",
      "Euler angles (in radians) necessary to rotate the sample reference frame to the reference frame of the Feature, where the principal axes of the best-fit ellipsoid are (X, Y, Z)",
      "AxisEulerAngles"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AspectRatiosArrayName_Key, "Aspect Ratios", "Ratio of semi-axis lengths (b/a and c/a) for best-fit ellipsoid to Feature", "AspectRatios"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesArrayName_Key, "Volumes", "The volume of each Feature", "Shape Volumes"));

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
  auto pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
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

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

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
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pAxisLengthsArrayPath);
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pAxisEulerAnglesArrayPath);
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{2ULL}, pAspectRatiosArrayPath);
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }
  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1ULL}, pVolumesArrayPath);
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindShapesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  FindShapesInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
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

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteral k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteral k_Omega3sArrayNameKey = "Omega3sArrayName";
constexpr StringLiteral k_AxisLengthsArrayNameKey = "AxisLengthsArrayName";
constexpr StringLiteral k_AxisEulerAnglesArrayNameKey = "AxisEulerAnglesArrayName";
constexpr StringLiteral k_AspectRatiosArrayNameKey = "AspectRatiosArrayName";
constexpr StringLiteral k_VolumesArrayNameKey = "VolumesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> FindShapesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindShapesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  // Cell Feature Attribute Matrix parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_Omega3sArrayNameKey, k_Omega3sArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_AxisLengthsArrayNameKey, k_AxisLengthsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_AxisEulerAnglesArrayNameKey, k_AxisEulerAnglesArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_AspectRatiosArrayNameKey, k_AspectRatiosArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VolumesArrayNameKey, k_VolumesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
