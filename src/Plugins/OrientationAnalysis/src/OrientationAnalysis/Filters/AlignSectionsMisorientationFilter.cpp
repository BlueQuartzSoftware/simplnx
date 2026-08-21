#include "AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

// Error Code constants
constexpr nx::core::int32 k_InputRepresentationTypeError = -68001;
constexpr nx::core::int32 k_InputComponentCountError = -68004;
constexpr nx::core::int32 k_GeometryNotThreeDimensionalError = -68005;
constexpr nx::core::int32 k_TupleCountGeometryMismatchError = -68006;
constexpr nx::core::int32 k_NegativeToleranceError = -68007;
constexpr nx::core::int32 k_InconsistentTupleCount = -68063;

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string AlignSectionsMisorientationFilter::name() const
{
  return FilterTraits<AlignSectionsMisorientationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsMisorientationFilter::className() const
{
  return FilterTraits<AlignSectionsMisorientationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsMisorientationFilter::uuid() const
{
  return FilterTraits<AlignSectionsMisorientationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsMisorientationFilter::humanName() const
{
  return "Align Sections (Misorientation)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsMisorientationFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsMisorientationFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)",
                                                   "Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the "
                                                   "tolerance one would use to define Features (i.e., 2-10 degrees)",
                                                   5.0f));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to remove some Cells from consideration in the alignment process", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Cell Mask Array", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath({"Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

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
IFilter::VersionType AlignSectionsMisorientationFilter::parametersVersion() const
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
IFilter::UniquePointer AlignSectionsMisorientationFilter::clone() const
{
  return std::make_unique<AlignSectionsMisorientationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsMisorientationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pMisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxels = filterArgs.value<bool>(k_UseMask_Key);
  auto pQuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  auto pStoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  // A negative tolerance makes the `angle > misorientationTolerance` test in the algorithm
  // true for every pair, including a pair of identical orientations whose disorientation is
  // exactly 0. Every candidate shift then scores the same maximum mismatch fraction and the
  // reported shifts are meaningless rather than merely imprecise.
  if(pMisorientationTolerance < 0.0F)
  {
    return MakePreflightErrorResult(::k_NegativeToleranceError, fmt::format("Misorientation Tolerance must be greater than or equal to 0 degrees but {} was supplied.", pMisorientationTolerance));
  }

  std::vector<DataPath> dataPaths;

  const auto* quats = dataStructure.getDataAs<Float32Array>(pQuatsArrayPath);
  if(quats->getNumberOfComponents() != 4)
  {
    return MakePreflightErrorResult(::k_InputComponentCountError,
                                    fmt::format("Quaternion Array at path '{}' has {} components but 4 are required.", pQuatsArrayPath.toString(), quats->getNumberOfComponents()));
  }
  dataPaths.push_back(pQuatsArrayPath);

  dataPaths.push_back(pCellPhasesArrayPath);

  if(pUseGoodVoxels)
  {
    dataPaths.push_back(pGoodVoxelsArrayPath);
  }
  // Ensure all DataArrays have the same number of Tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(k_InconsistentTupleCount, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  const auto* inputGeom = dataStructure.getDataAs<ImageGeom>(inputImageGeometry);
  if(inputGeom == nullptr)
  {
    return MakePreflightErrorResult(k_InputRepresentationTypeError, fmt::format("Cannot find selected input Image geometry at path '{}'", inputImageGeometry.toString()));
  }

  if(inputGeom->getCellData() == nullptr)
  {
    return MakePreflightErrorResult(k_InputRepresentationTypeError, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", inputImageGeometry.toString()));
  }

  // The alignment search needs a genuinely 3D volume. With a Z dimension of 1 there is no
  // section pair to align and the filter is a silent no-op. With an X or Y dimension of 1 the
  // algorithm's halfDim bound collapses to 0, which drives the candidate memoization index
  // negative and reads out of bounds. DREAM3D 6.5.171 rejected non-3D geometries in
  // AlignSections::dataCheck with error -3010; this is the port's equivalent guard.
  const SizeVec3 imageDims = inputGeom->getDimensions();
  if(imageDims.getX() <= 1 || imageDims.getY() <= 1 || imageDims.getZ() <= 1)
  {
    return MakePreflightErrorResult(::k_GeometryNotThreeDimensionalError, fmt::format("The selected Image Geometry '{}' is not 3D and cannot be aligned. Its dimensions are ({}, {}, {}); "
                                                                                      "every dimension must be greater than 1.",
                                                                                      inputImageGeometry.toString(), imageDims.getX(), imageDims.getY(), imageDims.getZ()));
  }

  // The tuple check above only cross-checks the selected arrays against EACH OTHER. The
  // algorithm indexes them with positions derived from the selected geometry's dimensions,
  // so a mutually consistent set of arrays that belongs to a different (smaller) geometry
  // passes that check and then reads out of bounds during execute.
  const usize requiredTupleCount = imageDims.getX() * imageDims.getY() * imageDims.getZ();
  for(const auto& dataPath : dataPaths)
  {
    const auto* dataArray = dataStructure.getDataAs<IDataArray>(dataPath);
    if(dataArray != nullptr && dataArray->getNumberOfTuples() != requiredTupleCount)
    {
      return MakePreflightErrorResult(::k_TupleCountGeometryMismatchError,
                                      fmt::format("The array '{}' has {} tuples but the selected Image Geometry '{}' has dimensions ({}, {}, {}) and therefore {} cells. Every selected cell "
                                                  "array must have one tuple per cell of the selected geometry.",
                                                  dataPath.toString(), dataArray->getNumberOfTuples(), inputImageGeometry.toString(), imageDims.getX(), imageDims.getY(), imageDims.getZ(),
                                                  requiredTupleCount));
    }
  }

  // Handle Array Creation
  if(pStoreAlignmentShifts)
  {
    const usize dims = inputGeom->getDimensions().getZ();
    auto pAlignmentAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key);
    const DataPath amPath = inputImageGeometry.createChildPath(pAlignmentAMName);

    // Create Parent AM
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(amPath, ShapeType{dims}));

    // The shift search writes tuples 1 through dims-1; tuple 0 has no slice pair to describe
    // because the topmost section is the registration anchor and is never moved. Pass an
    // explicit fill value so tuple 0 is a deterministic {0, 0} instead of depending on the
    // data store's default initialization.
    const std::string k_ShiftArrayFillValue = "0";

    // Create slices Array
    auto pSlicesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::uint32, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pSlicesName), "", k_ShiftArrayFillValue));

    // Create positioning Array
    auto pRelativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pRelativeShiftsName), "", k_ShiftArrayFillValue));

    // Create shifts Array
    auto pCumulativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pCumulativeShiftsName), "", k_ShiftArrayFillValue));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pQuatsArrayPath.getParent(), {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMisorientationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  AlignSectionsMisorientationInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto* inputGeom = dataStructure.getDataAs<ImageGeom>(inputValues.ImageGeometryPath);
  inputValues.misorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.quatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  inputValues.StoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);
  inputValues.AlignmentAMPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key));
  inputValues.SlicesArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key));
  inputValues.RelativeShiftsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key));
  inputValues.CumulativeShiftsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key));

  return AlignSectionsMisorientation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_WriteAlignmentShiftsKey = "WriteAlignmentShifts";
constexpr StringLiteral k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteral k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> AlignSectionsMisorientationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignSectionsMisorientationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_WriteAlignmentShiftsKey, k_StoreAlignmentShifts_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
