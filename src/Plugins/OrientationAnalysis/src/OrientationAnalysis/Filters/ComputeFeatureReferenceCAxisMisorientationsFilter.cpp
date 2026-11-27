#include "ComputeFeatureReferenceCAxisMisorientationsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceCAxisMisorientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureReferenceCAxisMisorientationsFilter::name() const
{
  return FilterTraits<ComputeFeatureReferenceCAxisMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureReferenceCAxisMisorientationsFilter::className() const
{
  return FilterTraits<ComputeFeatureReferenceCAxisMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureReferenceCAxisMisorientationsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureReferenceCAxisMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureReferenceCAxisMisorientationsFilter::humanName() const
{
  return "Compute Feature Reference C-Axis Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureReferenceCAxisMisorientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureReferenceCAxisMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgCAxesArrayPath_Key, "Average C-Axes", "The direction of the Feature's C-axis in the sample reference frame", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureReferenceCAxisMisorientationsArrayName_Key, "Feature Reference C-Axis Misorientations",
                                                          "Misorientation angle (in degrees) between Cell's C-axis and the C-axis of the Feature that owns that Cell",
                                                          "FeatureRefCAxisMisorientation"));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureAvgCAxisMisorientationsArrayName_Key, "Average C-Axis Misorientations",
                                                          "Average of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature", "AvgCAxisMisorientation"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureStdevCAxisMisorientationsArrayName_Key, "Feature Stdev C-Axis Misorientations",
                                                          "Standard deviation of the Feature Reference CAxis Misorientation values for all of the Cells that belong to the Feature",
                                                          "StdevCAxisMisorientation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureReferenceCAxisMisorientationsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeFeatureReferenceCAxisMisorientationsFilter::clone() const
{
  return std::make_unique<ComputeFeatureReferenceCAxisMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureReferenceCAxisMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                                          const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pAvgCAxesArrayPathValue = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  auto pFeatureAvgCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureAvgCAxisMisorientationsArrayName_Key);
  auto pFeatureStDevCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureStdevCAxisMisorientationsArrayName_Key);
  auto pFeatureReferenceCAxisMisorientationsArrayNameValue = filterArgs.value<std::string>(k_FeatureReferenceCAxisMisorientationsArrayName_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples({pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue, pQuatsArrayPathValue});
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-9800, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, imageGeometryPath, {pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue, pQuatsArrayPathValue}, -9805); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  ShapeType tupleShape = dataStructure.getDataRefAs<Int32ArrayType>(pFeatureIdsArrayPathValue).getTupleShape();
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pFeatureIdsArrayPathValue.replaceName(pFeatureReferenceCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  tupleShape = dataStructure.getDataRefAs<Float32ArrayType>(pAvgCAxesArrayPathValue).getTupleShape();
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.replaceName(pFeatureAvgCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  {
    auto createArrayAction =
        std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{1}, pAvgCAxesArrayPathValue.replaceName(pFeatureStDevCAxisMisorientationsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-9801, "Finding the feature reference c-axis mis orientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureReferenceCAxisMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/,
                                                                        const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeFeatureReferenceCAxisMisorientationsInputValues inputValues;

  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.avg_c_axes_array_path = filterArgs.value<DataPath>(k_AvgCAxesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  // Output Cell data
  inputValues.feature_reference_c_axis_misorientations_array_path = inputValues.feature_ids_array_path.replaceName(filterArgs.value<std::string>(k_FeatureReferenceCAxisMisorientationsArrayName_Key));
  // Output Feature data
  inputValues.feature_avg_c_axis_misorientations_array_path = inputValues.avg_c_axes_array_path.replaceName(filterArgs.value<std::string>(k_FeatureAvgCAxisMisorientationsArrayName_Key));
  inputValues.feature_stdev_c_axis_misorientations_array_path = inputValues.avg_c_axes_array_path.replaceName(filterArgs.value<std::string>(k_FeatureStdevCAxisMisorientationsArrayName_Key));

  return ComputeFeatureReferenceCAxisMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_AvgCAxesArrayPathKey = "AvgCAxesArrayPath";
constexpr StringLiteralType k_FeatureAvgCAxisMisorientationsArrayNameKey = "FeatureAvgCAxisMisorientationsArrayName";
constexpr StringLiteralType k_FeatureStdevCAxisMisorientationsArrayNameKey = "FeatureStdevCAxisMisorientationsArrayName";
constexpr StringLiteralType k_FeatureReferenceCAxisMisorientationsArrayNameKey = "FeatureReferenceCAxisMisorientationsArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeFeatureReferenceCAxisMisorientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureReferenceCAxisMisorientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_AvgCAxesArrayPathKey, k_AvgCAxesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureAvgCAxisMisorientationsArrayNameKey,
                                                                                                                     k_FeatureAvgCAxisMisorientationsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureStdevCAxisMisorientationsArrayNameKey,
                                                                                                                     k_FeatureStdevCAxisMisorientationsArrayName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureReferenceCAxisMisorientationsArrayNameKey,
                                                                                                                     k_FeatureReferenceCAxisMisorientationsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
