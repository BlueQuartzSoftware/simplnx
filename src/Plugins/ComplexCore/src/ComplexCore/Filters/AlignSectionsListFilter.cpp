#include "AlignSectionsListFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AlignSectionsList.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AlignSectionsListFilter::name() const
{
  return FilterTraits<AlignSectionsListFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsListFilter::className() const
{
  return FilterTraits<AlignSectionsListFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsListFilter::uuid() const
{
  return FilterTraits<AlignSectionsListFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsListFilter::humanName() const
{
  return "Align Sections (List)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsListFilter::defaultTags() const
{
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsListFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .txt file path containing the shifts to apply to the sections", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{"txt"}, FileSystemPathParameter::PathType::InputFile));
  params.insert(
      std::make_unique<BoolParameter>(k_DREAM3DAlignmentFile_Key, "DREAM3D Alignment File Format", "Turn this ON if the alignment file was generated by another DREAM.3D Alignment filter", false));
  params.insertSeparator(Parameters::Separator{"Required Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsListFilter::clone() const
{
  return std::make_unique<AlignSectionsListFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsListFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pDREAM3DAlignmentFileValue = filterArgs.value<bool>(k_DREAM3DAlignmentFile_Key);
  auto pSelectedImageGeometryPathValue = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pSelectedImageGeometryPathValue);
  if(imageGeom.getCellData() == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-8940, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", pSelectedImageGeometryPathValue.toString()))};
  }

  if(imageGeom.getNumXCells() <= 1 || imageGeom.getNumYCells() <= 1 || imageGeom.getNumZCells() <= 1)
  {
    return {MakeErrorResult<OutputActions>(-8941, fmt::format("The Image Geometry is not 3D and cannot be run through this filter. The dimensions are ({},{},{})", imageGeom.getNumXCells(),
                                                              imageGeom.getNumYCells(), imageGeom.getNumZCells()))};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsListFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  AlignSectionsListInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.DREAM3DAlignmentFile = filterArgs.value<bool>(k_DREAM3DAlignmentFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return AlignSectionsList(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
