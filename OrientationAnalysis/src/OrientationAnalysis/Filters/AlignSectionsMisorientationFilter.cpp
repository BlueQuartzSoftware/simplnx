#include "AlignSectionsMisorientationFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

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
constexpr complex::int32 k_IncorrectInputArrayType = -68063;

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
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsMisorientationFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "", 5.0f));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_GoodVoxels_Key, "Use Mask Array", "", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{AbstractGeometry::Type::Image}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Group", "Data Group that contains *only* cell data", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath({"Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(
      std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32}));

  params.insertSeparator(Parameters::Separator{"Optional File Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_AlignmentShiftFileName_Key, "Alignment File Path", "", fs::path("Data/Output/Alignment_By_Misorientation_Shifts.txt"),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
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
  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

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
  if(!dataStructure.validateNumberOfTuples(dataPaths))
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType, "All input arrays must have the same number of tuples")};
  }
  // Ensure the file path is not blank if they user wants to write the alignment shifts.
  if(pWriteAlignmentShifts && pAlignmentShiftFileName.empty())
  {
    return {MakeErrorResult<OutputActions>(k_IncorrectInputArrayType, "Write Alignment Shifts is TRUE but the output file path is empty. Please ensure the file path is set for the alignment file.")};
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMisorientationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  AlignSectionsMisorientationInputValues inputValues;
  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);
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
