#include "ApplyTransformationToGeometryFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
ApplyTransformationToGeometryInputValues CreateInputValues(const Arguments& filterArgs)
{
  ApplyTransformationToGeometryInputValues inputValues;
  inputValues.transformation_selection = filterArgs.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_TransformationType_Key);
  inputValues.interpolation_selection = filterArgs.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_InterpolationType_Key);
  inputValues.computed_transformation_matrix = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key);
  inputValues.manual_matrix_table_data = filterArgs.value<DynamicTableParameter::ValueType>(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key);
  inputValues.rotation = filterArgs.value<VectorFloat32ParameterType::ValueType>(ApplyTransformationToGeometryFilter::k_Rotation_Key);
  inputValues.translation = filterArgs.value<VectorFloat32ParameterType::ValueType>(ApplyTransformationToGeometryFilter::k_Translation_Key);
  inputValues.scale = filterArgs.value<VectorFloat32ParameterType::ValueType>(ApplyTransformationToGeometryFilter::k_Scale_Key);
  inputValues.selected_geometry_path = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key);
  inputValues.cell_attribute_matrix_path = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key);
  inputValues.translate_geometry_to_global_origin = filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key);
  inputValues.clear_feature_attribute_matrix = filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_ClearFeatureAttributeMatrix_Key);
  inputValues.renumber_features = inputValues.clear_feature_attribute_matrix && filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_RenumberFeatures_Key);
  inputValues.feature_attribute_matrix_path = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(ApplyTransformationToGeometryFilter::k_FeatureAttributeMatrixPath_Key);
  inputValues.feature_ids_path = filterArgs.value<ArraySelectionParameter::ValueType>(ApplyTransformationToGeometryFilter::k_FeatureIdsPath_Key);
  inputValues.remove_original_geometry = true;
  inputValues.save_transform_matrix = filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_SaveTransformMatrix_Key);
  inputValues.transform_matrix_path = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_TransformMatrixOutputPath_Key);
  return inputValues;
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::name() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::className() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ApplyTransformationToGeometryFilter::uuid() const
{
  return FilterTraits<ApplyTransformationToGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ApplyTransformationToGeometryFilter::humanName() const
{
  return "Apply Transformation to Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ApplyTransformationToGeometryFilter::defaultTags() const
{
  return {className(), "Scale", "Flip", "Mirror", "Rotation", "Transforming"};
}

//------------------------------------------------------------------------------
Parameters ApplyTransformationToGeometryFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_TransformationType_Key, "Transformation Type", "The type of transformation to perform.", detail::k_RotationIdx, detail::k_TransformationChoices));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  const DynamicTableInfo::TableDataType defaultTable{{{1.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}};
  params.insert(std::make_unique<DynamicTableParameter>(k_ManualTransformationMatrix_Key, "Transformation Matrix", "The 4x4 Transformation Matrix", defaultTable, tableInfo));

  params.insert(std::make_unique<VectorFloat32ParameterType>(k_Rotation_Key, "Rotation Axis-Angle", "<xyz> w (w in degrees)", std::vector<float32>{0.0F, 0.0F, 1.0F, 90.0F},
                                                         std::vector<std::string>{"x", "y", "z", "w (Deg)"}));
  params.insert(
      std::make_unique<VectorFloat32ParameterType>(k_Translation_Key, "Translation", "A pure translation vector", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32ParameterType>(k_Scale_Key, "Scale", "0>= value < 1: Shrink, value = 1: No transform, value > 1.0 enlarge", std::vector<float32>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Precomputed Transformation Matrix Path", "A precomputed 4x4 transformation matrix", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}));

  params.insert(std::make_unique<BoolParameter>(k_TranslateGeometryToGlobalOrigin_Key, "Translate Geometry To Global Origin Before Transformation",
                                                "Specifies whether to translate the geometry to (0, 0, 0), apply the transformation, and then translate the geometry back to its original origin.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Geometry", "The target geometry on which to perform the transformation", DataPath{},
                                                             IGeometry::GetAllGeomTypes()));

  params.insertSeparator(Parameters::Separator{"Image Geometry Resampling/Interpolation"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Resampling or Interpolation (Image Geometry Only)",
                                                                    "Select the type of interpolation algorithm. (0)Nearest Neighbor, (1)Linear Interpolation, (3)No Interpolation",
                                                                    detail::k_NoInterpolationIdx, detail::k_InterpolationChoices));

  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellAttributeMatrixPath_Key, "Cell Attribute Matrix (Image Geometry Only)",
                                                                    "The path to the Cell level data that should be interpolated. Only applies when selecting an Image Geometry.", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
                                                                 "Removes all arrays from the selected Feature Attribute Matrix. Transformations can invalidate feature positions, directions, and "
                                                                 "sizes. Recompute any feature data needed later in the pipeline.",
                                                                 true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after transformation", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Feature Ids", "Specifies to which feature each element belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Output Transform Matrix"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveTransformMatrix_Key, "Save Transformation Matrix", "Save the generated transform matrix as a Data Array", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformMatrixOutputPath_Key, "Transform Matrix Output Path", "The output array that contains the transformation Matrix.",
                                                         DataPath({"Transformation Matrix"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_TransformationType_Key, k_ComputedTransformationMatrix_Key, detail::k_PrecomputedTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_ManualTransformationMatrix_Key, detail::k_ManualTransformationMatrixIdx);
  params.linkParameters(k_TransformationType_Key, k_Rotation_Key, detail::k_RotationIdx);
  params.linkParameters(k_TransformationType_Key, k_Translation_Key, detail::k_TranslationIdx);
  params.linkParameters(k_TransformationType_Key, k_Scale_Key, detail::k_ScaleIdx);

  params.linkParameters(k_InterpolationType_Key, k_CellAttributeMatrixPath_Key, detail::k_NearestNeighborInterpolationIdx);
  params.linkParameters(k_InterpolationType_Key, k_CellAttributeMatrixPath_Key, detail::k_LinearInterpolationIdx);

  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrixPath_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureIdsPath_Key, true);
  params.linkParameters(k_SaveTransformMatrix_Key, k_TransformMatrixOutputPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ApplyTransformationToGeometryFilter::parametersVersion() const
{
  return 3;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ApplyTransformationToGeometryFilter::clone() const
{
  return std::make_unique<ApplyTransformationToGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApplyTransformationToGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  return PreflightGeometryTransformation(dataStructure, CreateInputValues(filterArgs));
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  return ApplyGeometryTransformation(dataStructure, CreateInputValues(filterArgs), messageHandler, shouldCancel);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_TransformationMatrixTypeKey = "TransformationMatrixType";
constexpr StringLiteralType k_InterpolationTypeKey = "InterpolationType";
constexpr StringLiteralType k_UseDataArraySelectionKey = "UseDataArraySelection";
constexpr StringLiteralType k_ManualTransformationMatrixKey = "ManualTransformationMatrix";
constexpr StringLiteralType k_RotationAngleKey = "RotationAngle";
constexpr StringLiteralType k_RotationAxisKey = "RotationAxis";
constexpr StringLiteralType k_TranslationKey = "Translation";
constexpr StringLiteralType k_ScaleKey = "Scale";
constexpr StringLiteralType k_ComputedTransformationMatrixKey = "ComputedTransformationMatrix";
constexpr StringLiteralType k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteralType k_DataArraySelectionKey = "DataArraySelection";
} // namespace simpl
} // namespace

Result<Arguments> ApplyTransformationToGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ApplyTransformationToGeometryFilter().getDefaultArguments();
  // The legacy filter had no Feature AM handling and preserved Feature data.
  args.insertOrAssign(k_ClearFeatureAttributeMatrix_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_RenumberFeatures_Key, std::make_any<BoolParameter::ValueType>(false));

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedChoicesFilterParameterConverter>(args, json, simpl::k_TransformationMatrixTypeKey, k_TransformationType_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_InterpolationTypeKey, k_InterpolationType_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseDataArraySelectionKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DynamicTableFilterParameterConverter>(args, json, simpl::k_ManualTransformationMatrixKey, k_ManualTransformationMatrix_Key));
  results.push_back(simpl_conversion::Convert2Parameters<simpl_conversion::FloatVec3p1FilterParameterConverterType>(args, json, simpl::k_RotationAxisKey, simpl::k_RotationAngleKey, k_Rotation_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_TranslationKey, k_Translation_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_ScaleKey, k_Scale_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_ComputedTransformationMatrixKey, k_ComputedTransformationMatrix_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixPathKey, k_CellAttributeMatrixPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_DataArraySelectionKey, "@SIMPLNX_PARAMETER_KEY@"));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
