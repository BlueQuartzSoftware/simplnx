#include "AlignSectionsMutualInformationFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/AlignSectionsMutualInformation.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
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
  return {className(), "#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsMutualInformationFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance",
                                                   "Tolerance used to decide if Cells above/below one another should be considered to be the same. The value selected should be similar to the "
                                                   "tolerance one would use to define Features (i.e., 2-10 degrees).",
                                                   5.0f));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "Whether to remove some Cells from consideration in the alignment process.", true));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Specifies if the Cell is to be counted in the algorithm. Only required if Use Mask Array is checked.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Cell belongs.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional File Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "Whether to write the shifts applied to each section to a file.", false));
  params.insert(std::make_unique<FileSystemPathParameter>(
      k_AlignmentShiftFileName_Key, "Alignment File Path",
      "The output file path where the user would like the shifts applied to the section to be written. Only needed if Write Alignment Shifts File is checked.",
      fs::path("Data/Output/Alignment_By_Mutual_Information_Shifts.txt"), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.linkParameters(k_WriteAlignmentShifts_Key, k_AlignmentShiftFileName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsMutualInformationFilter::clone() const
{
  return std::make_unique<AlignSectionsMutualInformationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsMutualInformationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto pWriteAlignmentShiftsValue = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  if(imageGeom.getCellData() == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-3540, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", imageGeometryPath.toString()))};
  }

  if(imageGeom.getNumXCells() <= 1 || imageGeom.getNumYCells() <= 1 || imageGeom.getNumZCells() <= 1)
  {
    return {MakeErrorResult<OutputActions>(-3541, fmt::format("The Image Geometry is not 3D and cannot be run through this filter. The dimensions are ({},{},{})", imageGeom.getNumXCells(),
                                                              imageGeom.getNumYCells(), imageGeom.getNumZCells()))};
  }

  std::vector<DataPath> dataArrayPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue, pGoodVoxelsArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-3542, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsMutualInformationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  AlignSectionsMutualInformationInputValues inputValues;

  inputValues.WriteAlignmentShifts = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  inputValues.AlignmentShiftFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return AlignSectionsMutualInformation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
