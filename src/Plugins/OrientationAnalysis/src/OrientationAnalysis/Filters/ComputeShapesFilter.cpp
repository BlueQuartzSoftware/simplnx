#include "ComputeShapesFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeShapes.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeShapesFilter::name() const
{
  return FilterTraits<ComputeShapesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeShapesFilter::className() const
{
  return FilterTraits<ComputeShapesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeShapesFilter::uuid() const
{
  return FilterTraits<ComputeShapesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeShapesFilter::humanName() const
{
  return "Compute Feature Shapes (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeShapesFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Find", "Generate", "Calculate", "Determine", "Omega3", "Axis Length"};
}

//------------------------------------------------------------------------------
Parameters ComputeShapesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CentroidsArrayPath_Key, "Feature Centroids", "X, Y, Z coordinates of Feature center of mass", DataPath({"Centroids"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
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
IFilter::VersionType ComputeShapesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeShapesFilter::clone() const
{
  return std::make_unique<ComputeShapesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeShapesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pFeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  auto pOmega3sArrayName = filterArgs.value<std::string>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayName = filterArgs.value<std::string>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayName = filterArgs.value<std::string>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayName = filterArgs.value<std::string>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayName = filterArgs.value<std::string>(k_VolumesArrayName_Key);
  auto tupleCountResult = ValidateCellTupleCount(dataStructure, filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key), {pFeatureIdsArrayPath}, -12806);
  if(tupleCountResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleCountResult), {})};
  }

  auto featureAttrMatrixPath = pCentroidsArrayPath.getParent();
  auto pOmega3sArrayPath = featureAttrMatrixPath.createChildPath(pOmega3sArrayName);
  auto pAxisLengthsArrayPath = featureAttrMatrixPath.createChildPath(pAxisLengthsArrayName);
  auto pAxisEulerAnglesArrayPath = featureAttrMatrixPath.createChildPath(pAxisEulerAnglesArrayName);
  auto pAspectRatiosArrayPath = featureAttrMatrixPath.createChildPath(pAspectRatiosArrayName);
  auto pVolumesArrayPath = featureAttrMatrixPath.createChildPath(pVolumesArrayName);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const AttributeMatrix* featureAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
  if(featureAttrMatrixPtr == nullptr)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-12801, fmt::format("Could not find selected cell feature Attribute Matrix at path '{}'", featureAttrMatrixPath.toString()))};
  }

  ShapeType tupleShape = featureAttrMatrixPtr->getShape();

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
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeShapesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeShapesInputValues inputValues;

  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.centroids_array_path = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.feature_attribute_matrix_path = inputValues.centroids_array_path.getParent();
  auto pOmega3sArrayName = filterArgs.value<std::string>(k_Omega3sArrayName_Key);
  auto pAxisLengthsArrayName = filterArgs.value<std::string>(k_AxisLengthsArrayName_Key);
  auto pAxisEulerAnglesArrayName = filterArgs.value<std::string>(k_AxisEulerAnglesArrayName_Key);
  auto pAspectRatiosArrayName = filterArgs.value<std::string>(k_AspectRatiosArrayName_Key);
  auto pVolumesArrayName = filterArgs.value<std::string>(k_VolumesArrayName_Key);
  inputValues.omega3s_array_path = inputValues.feature_attribute_matrix_path.createChildPath(pOmega3sArrayName);
  inputValues.axis_lengths_array_path = inputValues.feature_attribute_matrix_path.createChildPath(pAxisLengthsArrayName);
  inputValues.axis_euler_angles_array_path = inputValues.feature_attribute_matrix_path.createChildPath(pAxisEulerAnglesArrayName);
  inputValues.aspect_ratios_array_path = inputValues.feature_attribute_matrix_path.createChildPath(pAspectRatiosArrayName);
  inputValues.volumes_array_path = inputValues.feature_attribute_matrix_path.createChildPath(pVolumesArrayName);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellFeatureAttributeMatrixNameKey = "CellFeatureAttributeMatrixName";
constexpr StringLiteralType k_CentroidsArrayPathKey = "CentroidsArrayPath";
constexpr StringLiteralType k_Omega3sArrayNameKey = "Omega3sArrayName";
constexpr StringLiteralType k_AxisLengthsArrayNameKey = "AxisLengthsArrayName";
constexpr StringLiteralType k_AxisEulerAnglesArrayNameKey = "AxisEulerAnglesArrayName";
constexpr StringLiteralType k_AspectRatiosArrayNameKey = "AspectRatiosArrayName";
constexpr StringLiteralType k_VolumesArrayNameKey = "VolumesArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeShapesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeShapesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  // Cell Feature Attribute Matrix parameter is not applicable in NX
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CentroidsArrayPathKey, k_CentroidsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_Omega3sArrayNameKey, k_Omega3sArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_AxisLengthsArrayNameKey, k_AxisLengthsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_AxisEulerAnglesArrayNameKey, k_AxisEulerAnglesArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_AspectRatiosArrayNameKey, k_AspectRatiosArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_VolumesArrayNameKey, k_VolumesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
