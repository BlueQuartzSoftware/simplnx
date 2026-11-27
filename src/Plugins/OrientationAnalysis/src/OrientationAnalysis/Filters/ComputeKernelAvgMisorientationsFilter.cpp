#include "ComputeKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace nx::core
{
std::string ComputeKernelAvgMisorientationsFilter::name() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::name.str();
}

std::string ComputeKernelAvgMisorientationsFilter::className() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::className;
}

Uuid ComputeKernelAvgMisorientationsFilter::uuid() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::uuid;
}

std::string ComputeKernelAvgMisorientationsFilter::humanName() const
{
  return "Compute Kernel Average Misorientations";
}

std::vector<std::string> ComputeKernelAvgMisorientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Misorientation", "KAM"};
}

Parameters ComputeKernelAvgMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the filter parameter descriptors.
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorInt32ParameterType>(k_KernelSize_Key, "Kernel Radius", "Size of the kernel in the X, Y and Z directions (in number of Cells)", std::vector<int32>{1, 1, 1},
                                                       std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<BoolParameter>(
      k_UseFeatureIds_Key, "Use Feature Ids",
      "When checked (default), only kernel Cells belonging to the same Feature as the central Cell are included in the average (per-grain KAM). When unchecked, every in-bounds kernel Cell with a "
      "Feature Id greater than 0 and the same phase as the central Cell is included, allowing the average to cross Feature boundaries (per-voxel KAM).",
      true));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "Path to the target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Cell Data", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation",
                                                          DataPath({"Cell Data", "Quats"}), ArraySelectionParameter::AllowedTypeSetType{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypeSetType{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelAverageMisorientationsArrayName_Key, "Kernel Average Misorientations",
                                                          "The name of the array containing the average  misorientation (in Degrees) for all Cells within the kernel and the central Cell",
                                                          "KernelAverageMisorientations"));

  return params;
}

IFilter::VersionType ComputeKernelAvgMisorientationsFilter::parametersVersion() const
{
  return 2;
  // Version 1 -> 2
  // Change 1:
  // Added k_UseFeatureIds_Key = "use_feature_ids" (default true preserves the
  // original per-grain behavior; false enables per-voxel KAM per issue #1613)
}

IFilter::UniquePointerType ComputeKernelAvgMisorientationsFilter::clone() const
{
  return std::make_unique<ComputeKernelAvgMisorientationsFilter>();
}

IFilter::PreflightResult ComputeKernelAvgMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                              const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pKernelSizeValue = filterArgs.value<VectorInt32ParameterType::ValueType>(k_KernelSize_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pKernelAverageMisorientationsArrayNameValue = pCellPhasesArrayPathValue.replaceName(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  if(auto tupleResult = ValidateCellTupleCount(dataStructure, inputImageGeometry, {pFeatureIdsArrayPathValue, pCellPhasesArrayPathValue, pQuatsArrayPathValue}, -67205); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  const auto& cellPhases = dataStructure.getDataRefAs<Int32ArrayType>(pCellPhasesArrayPathValue);

  // Create output Kernel Average Misorientations
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellPhases.getIDataStore()->getTupleShape(), std::vector<usize>{1}, pKernelAverageMisorientationsArrayNameValue);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return actions and updated preflight values.
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

Result<> ComputeKernelAvgMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeKernelAvgMisorientationsInputValues inputValues;

  inputValues.kernel_size = filterArgs.value<VectorInt32ParameterType::ValueType>(k_KernelSize_Key);
  inputValues.use_feature_ids = filterArgs.value<bool>(k_UseFeatureIds_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.kernel_average_misorientations_array_name = inputValues.cell_phases_array_path.replaceName(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  inputValues.input_image_geometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeKernelAvgMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_KernelSizeKey = "KernelSize";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_KernelAverageMisorientationsArrayNameKey = "KernelAverageMisorientationsArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeKernelAvgMisorientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeKernelAvgMisorientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntVec3FilterParameterConverterType>(args, json, simpl::k_KernelSizeKey, k_KernelSize_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_KernelAverageMisorientationsArrayNameKey,
                                                                                                                   k_KernelAverageMisorientationsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
