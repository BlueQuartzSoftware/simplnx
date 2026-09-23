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
  inputValues.TransformationSelection = filterArgs.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_TransformationType_Key);
  inputValues.InterpolationSelection = filterArgs.value<ChoicesParameter::ValueType>(ApplyTransformationToGeometryFilter::k_InterpolationType_Key);
  inputValues.ComputedTransformationMatrix = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_ComputedTransformationMatrix_Key);
  inputValues.ManualMatrixTableData = filterArgs.value<DynamicTableParameter::ValueType>(ApplyTransformationToGeometryFilter::k_ManualTransformationMatrix_Key);
  inputValues.Rotation = filterArgs.value<VectorFloat32Parameter::ValueType>(ApplyTransformationToGeometryFilter::k_Rotation_Key);
  inputValues.Translation = filterArgs.value<VectorFloat32Parameter::ValueType>(ApplyTransformationToGeometryFilter::k_Translation_Key);
  inputValues.Scale = filterArgs.value<VectorFloat32Parameter::ValueType>(ApplyTransformationToGeometryFilter::k_Scale_Key);
  inputValues.SelectedGeometryPath = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_SelectedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_CellAttributeMatrixPath_Key);
  inputValues.TranslateGeometryToGlobalOrigin = filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_TranslateGeometryToGlobalOrigin_Key);
  inputValues.RemoveOriginalGeometry = true;
  inputValues.SaveTransformMatrix = filterArgs.value<BoolParameter::ValueType>(ApplyTransformationToGeometryFilter::k_SaveTransformMatrix_Key);
  inputValues.TransformMatrixPath = filterArgs.value<DataPath>(ApplyTransformationToGeometryFilter::k_TransformMatrixOutputPath_Key);
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

  params.insert(std::make_unique<VectorFloat32Parameter>(k_Rotation_Key, "Rotation Axis-Angle", "<xyz> w (w in degrees)", std::vector<float32>{0.0F, 0.0F, 1.0F, 90.0F},
                                                         std::vector<std::string>{"x", "y", "z", "w (Deg)"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Translation_Key, "Translation", "A pure translation vector", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Scale_Key, "Scale", "0>= value < 1: Shrink, value = 1: No transform, value > 1.0 enlarge", std::vector<float32>{1.0F, 1.0F, 1.0F},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_ComputedTransformationMatrix_Key, "Precomputed Transformation Matrix Path", "A precomputed 4x4 transformation matrix", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));

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

  params.linkParameters(k_SaveTransformMatrix_Key, k_TransformMatrixOutputPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ApplyTransformationToGeometryFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ApplyTransformationToGeometryFilter::clone() const
{
  return std::make_unique<ApplyTransformationToGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ApplyTransformationToGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  return PreflightGeometryTransformation(dataStructure, CreateInputValues(filterArgs));
}

//------------------------------------------------------------------------------
Result<> ApplyTransformationToGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  return ApplyGeometryTransformation(dataStructure, CreateInputValues(filterArgs), messageHandler, shouldCancel);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_TransformationMatrixTypeKey = "TransformationMatrixType";
constexpr StringLiteral k_InterpolationTypeKey = "InterpolationType";
constexpr StringLiteral k_UseDataArraySelectionKey = "UseDataArraySelection";
constexpr StringLiteral k_ManualTransformationMatrixKey = "ManualTransformationMatrix";
constexpr StringLiteral k_RotationAngleKey = "RotationAngle";
constexpr StringLiteral k_RotationAxisKey = "RotationAxis";
constexpr StringLiteral k_TranslationKey = "Translation";
constexpr StringLiteral k_ScaleKey = "Scale";
constexpr StringLiteral k_ComputedTransformationMatrixKey = "ComputedTransformationMatrix";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteral k_DataArraySelectionKey = "DataArraySelection";
} // namespace SIMPL
} // namespace

Result<Arguments> ApplyTransformationToGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ApplyTransformationToGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_TransformationMatrixTypeKey, k_TransformationType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_InterpolationTypeKey, k_InterpolationType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseDataArraySelectionKey, "@SIMPLNX_PARAMETER_KEY@"));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DynamicTableFilterParameterConverter>(args, json, SIMPL::k_ManualTransformationMatrixKey, k_ManualTransformationMatrix_Key));
  results.push_back(SIMPLConversion::Convert2Parameters<SIMPLConversion::FloatVec3p1FilterParameterConverter>(args, json, SIMPL::k_RotationAxisKey, SIMPL::k_RotationAngleKey, k_Rotation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_TranslationKey, k_Translation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_ScaleKey, k_Scale_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ComputedTransformationMatrixKey, k_ComputedTransformationMatrix_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_CellAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_DataArraySelectionKey, "@SIMPLNX_PARAMETER_KEY@"));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
