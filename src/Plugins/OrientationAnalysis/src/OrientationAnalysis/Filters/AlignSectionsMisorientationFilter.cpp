#include "AlignSectionsMisorientationFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace
{

// Error Code constants
constexpr complex::int32 k_InputRepresentationTypeError = -68001;
constexpr complex::int32 k_OutputRepresentationTypeError = -68002;
constexpr complex::int32 k_InputComponentDimensionError = -68003;
constexpr complex::int32 k_InputComponentCountError = -68004;
constexpr complex::int32 k_InconsistentTupleCount = -68063;
constexpr complex::int32 k_OutputFilePathEmpty = -68063;

} // namespace

namespace complex
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
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)",
                                                   "Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the "
                                                   "tolerance one would use to define Features (i.e., 2-10 degrees)",
                                                   5.0f));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_GoodVoxels_Key, "Use Mask Array", "Whether to remove some Cells from consideration in the alignment process", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath({"Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional File Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "Whether to write the shifts applied to each section to a file", false));
  params.insert(std::make_unique<FileSystemPathParameter>(
      k_AlignmentShiftFileName_Key, "Alignment File Path", "The output file path where the user would like the shifts applied to the section to be written.",
      fs::path("Data/Output/Alignment_By_Misorientation_Shifts.txt"), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.linkParameters(k_WriteAlignmentShifts_Key, k_AlignmentShiftFileName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsMisorientationFilter::clone() const
{
  return std::make_unique<AlignSectionsMisorientationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsMisorientationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{
  auto pWriteAlignmentShifts = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pMisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxels = filterArgs.value<bool>(k_GoodVoxels_Key);
  auto pQuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths;

  const auto* quats = dataStructure.getDataAs<Float32Array>(pQuatsArrayPath);
  if(quats->getNumberOfComponents() != 4)
  {
    return {MakeErrorResult<OutputActions>(::k_InputComponentCountError, fmt::format("Quaternion Array does not have 4 components."))};
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
    return {MakeErrorResult<OutputActions>(k_InconsistentTupleCount,
                                           fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto* inputGeom = dataStructure.getDataAs<ImageGeom>(inputImageGeometry);
  if(inputGeom == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_InputRepresentationTypeError, fmt::format("Cannot find selected input Image geometry at path '{}'", inputImageGeometry.toString()))};
  }

  if(inputGeom->getCellData() == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_InputRepresentationTypeError, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", inputImageGeometry.toString()))};
  }
  // Ensure the file path is not blank if they user wants to write the alignment shifts.
  if(pWriteAlignmentShifts && pAlignmentShiftFileName.empty())
  {
    return {MakeErrorResult<OutputActions>(k_OutputFilePathEmpty, "Write Alignment Shifts is TRUE but the output file path is empty. Please ensure the file path is set for the alignment file.")};
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  complex::AppendDataModifiedActions(dataStructure, resultOutputActions.value().modifiedActions, pQuatsArrayPath.getParent(), {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMisorientationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  AlignSectionsMisorientationInputValues inputValues;
  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto* inputGeom = dataStructure.getDataAs<ImageGeom>(inputValues.inputImageGeometry);
  inputValues.cellDataGroupPath = inputGeom->getCellDataPath();
  inputValues.writeAlignmentShifts = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  inputValues.alignmentShiftFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  inputValues.misorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.useGoodVoxels = filterArgs.value<bool>(k_GoodVoxels_Key);
  inputValues.quatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.goodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return AlignSectionsMisorientation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
