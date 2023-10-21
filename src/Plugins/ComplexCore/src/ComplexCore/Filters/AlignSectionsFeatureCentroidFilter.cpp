#include "AlignSectionsFeatureCentroidFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AlignSectionsFeatureCentroid.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

// Error Code constants
constexpr complex::int32 k_InputRepresentationTypeError = -68001;
constexpr complex::int32 k_OutputRepresentationTypeError = -68002;
constexpr complex::int32 k_InputComponentDimensionError = -68003;
constexpr complex::int32 k_InputComponentCountError = -68004;
constexpr complex::int32 k_InconsistentTupleCount = -68063;
constexpr complex::int32 k_OutOfRangeReferenceSliceValue = -68064;

} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::name() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::className() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsFeatureCentroidFilter::uuid() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::humanName() const
{
  return "Align Sections (Feature Centroid)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsFeatureCentroidFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsFeatureCentroidFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseReferenceSlice_Key, "Use Reference Slice",
                                                                 "Whether the centroids of each section should be compared to a reference slice instead of their neighboring section", false));
  params.insert(std::make_unique<Int32Parameter>(k_ReferenceSlice_Key, "Reference Slice", "Slice number to use as reference", 0));
  params.linkParameters(k_UseReferenceSlice_Key, k_ReferenceSlice_Key, true);

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Attribute Matrix", "Cell Data Attribute Matrix", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional File Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "Whether to write the shifts applied to each section to a file", false));
  params.insert(std::make_unique<FileSystemPathParameter>(
      k_AlignmentShiftFileName_Key, "Alignment File Path", "The output file path where the user would like the shifts applied to the section to be written.",
      fs::path("Data/Output/Alignment_By_Feature_Centroid_Shifts.txt"), FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputFile));
  params.linkParameters(k_WriteAlignmentShifts_Key, k_AlignmentShiftFileName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsFeatureCentroidFilter::clone() const
{
  return std::make_unique<AlignSectionsFeatureCentroidFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsFeatureCentroidFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pWriteAlignmentShifts = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pReferenceSliceValue = filterArgs.value<int32>(k_ReferenceSlice_Key);
  auto pGoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  if(pReferenceSliceValue < 0)
  {
    return {MakeErrorResult<OutputActions>(k_OutOfRangeReferenceSliceValue, "Reference Slice value must be ZERO or greater.")};
  }

  std::vector<DataPath> dataPaths;
  dataPaths.push_back(pGoodVoxelsArrayPath);

  // Ensure all DataArrays have the same number of Tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(k_InconsistentTupleCount,
                                           fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Ensure the file path is not blank if they user wants to write the alignment shifts.
  if(pWriteAlignmentShifts && pAlignmentShiftFileName.empty())
  {
    return {MakeErrorResult<OutputActions>(-3425, "Write Alignment Shifts is TRUE but the output file path is empty. Please ensure the file path is set for the alignment file.")};
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  complex::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, cellDataGroupPath, {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroidFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  AlignSectionsFeatureCentroidInputValues inputValues;

  inputValues.WriteAlignmentShifts = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  inputValues.AlignmentShiftFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  inputValues.UseReferenceSlice = filterArgs.value<bool>(k_UseReferenceSlice_Key);
  inputValues.ReferenceSlice = filterArgs.value<int32>(k_ReferenceSlice_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  return AlignSectionsFeatureCentroid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
