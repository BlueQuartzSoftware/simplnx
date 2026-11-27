#include "AlignSectionsMutualInformationFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/AlignSectionsMutualInformation.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <filesystem>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformationFilter::name() const
{
  return FilterTraits<AlignSectionsMutualInformationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformationFilter::className() const
{
  return FilterTraits<AlignSectionsMutualInformationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsMutualInformationFilter::uuid() const
{
  return FilterTraits<AlignSectionsMutualInformationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsMutualInformationFilter::humanName() const
{
  return "Align Sections (Mutual Information)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsMutualInformationFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsMutualInformationFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32ParameterType>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)",
                                                       "Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the "
                                                       "tolerance one would use to define Features (i.e., 2-10 degrees).",
                                                       5.0f));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to remove some Cells from consideration in the alignment process.", false));
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_MaskArrayPath_Key, "Cell Mask Array", "Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked.", DataPath({"Mask"}), GetDataArrayOnlyTypes(),
      ArraySelectionParameter::AllowedDataTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation.", DataPath({"Quats"}),
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs.", DataPath({"Phases"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble.",
                                                          DataPath({"CrystalStructures"}), GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Alignment Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAlignmentShifts_Key, "Store Alignment Shifts",
                                                                 "Whether to store the shifts applied to each section to a collection of Arrays in a new Attribute Matrix", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentAMName_Key, "Alignment Attribute Matrix Name",
                                                          "The output attribute matrix where the shifts applied to the section to be stored as DataArrays.", "Alignment Shifts Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SlicesArrayName_Key, "Alignment Slices Data Array Name",
                                                          "The output array name where the slice information related to shifts will be stored.", "Slice Indices"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_RelativeShiftsArrayName_Key, "Alignment Relative Shifts Data Array Name",
                                                          "The output array name where the new shifts relative to previous slice information will be stored.", "Relative Shifts"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CumulativeShiftsArrayName_Key, "Alignment Cumulative Shifts Data Array Name",
                                                          "The output array name where the accumulated shift information will be stored.", "Cumulative Shifts"));

  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentAMName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_SlicesArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_RelativeShiftsArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_CumulativeShiftsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType AlignSectionsMutualInformationFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Description:
  // Swapped Writing Shifts to a file to storing them in Data Arrays
  //
  // Change 1:
  // Replaced - k_WriteAlignmentShifts_Key = "write_alignment_shifts" -> k_StoreAlignmentShifts_Key = "store_alignment_shifts";
  // Solution - `k_StoreAlignmentShifts_Key Value` = `k_WriteAlignmentShifts_Key Value`;
  //
  // Change 2:
  // Replaced - k_AlignmentShiftFileName_Key = "alignment_shift_file_name" -> k_AlignmentAMName_Key = "alignment_attribute_matrix_name";
  // Solution: (For backwards pipeline conversion, else just use default)
  // Steps:
  //  1. Read File Name
  //  2. Strip extension
  //  3. `k_AlignmentShiftArrayName_Key Value` = cleaned up file name
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType AlignSectionsMutualInformationFilter::clone() const
{
  return std::make_unique<AlignSectionsMutualInformationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsMutualInformationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                             const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  auto useMask = filterArgs.value<bool>(k_UseMask_Key);
  auto pStoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  if(imageGeom.getCellData() == nullptr)
  {
    return MakePreflightErrorResult(-3540, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", imageGeometryPath.toString()));
  }

  if(imageGeom.getNumXCells() <= 1 || imageGeom.getNumYCells() <= 1 || imageGeom.getNumZCells() <= 1)
  {
    return MakePreflightErrorResult(
        -3541, fmt::format("The Image Geometry is not 3D and cannot be run through this filter. The dimensions are ({})", string_utilities::FormatDimensions3D(imageGeom.getDimensions())));
  }

  std::vector<DataPath> dataArrayPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue};
  if(useMask)
  {
    dataArrayPaths.push_back(pGoodVoxelsArrayPathValue);
  }
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(-3542, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  std::vector<DataPath> cellArrayPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue};
  if(useMask)
  {
    cellArrayPaths.emplace_back(pGoodVoxelsArrayPathValue);
  }
  if(auto tupleResult = ValidateCellTupleCount(dataStructure, imageGeometryPath, cellArrayPaths, -3543); tupleResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleResult), {})};
  }

  // Handle Array Creation
  if(pStoreAlignmentShifts)
  {
    const usize dims = imageGeom.getDimensions().getZ();
    auto pAlignmentAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key);
    const DataPath amPath = imageGeometryPath.createChildPath(pAlignmentAMName);

    // Create Parent AM
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(amPath, ShapeType{dims}));

    // Create slices Array
    auto pSlicesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint32, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pSlicesName)));

    // Create positioning Array
    auto pRelativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pRelativeShiftsName)));

    // Create shifts Array
    auto pCumulativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pCumulativeShiftsName)));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modified_actions, pQuatsArrayPathValue.getParent(), {});

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMutualInformationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  AlignSectionsMutualInformationInputValues inputValues;

  inputValues.misorientation_tolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  inputValues.store_alignment_shifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);
  inputValues.alignment_am_path = inputValues.image_geometry_path.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key));
  inputValues.slices_array_path = inputValues.alignment_am_path.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key));
  inputValues.relative_shifts_array_path = inputValues.alignment_am_path.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key));
  inputValues.cumulative_shifts_array_path = inputValues.alignment_am_path.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key));

  return AlignSectionsMutualInformation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_WriteAlignmentShiftsKey = "WriteAlignmentShifts";
constexpr StringLiteralType k_AlignmentShiftFileNameKey = "AlignmentShiftFileName";
constexpr StringLiteralType k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> AlignSectionsMutualInformationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignSectionsMutualInformationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_WriteAlignmentShiftsKey, k_StoreAlignmentShifts_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatFilterParameterConverter<float32>>(args, json, simpl::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
