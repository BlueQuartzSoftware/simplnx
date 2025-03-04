#include "AlignSectionsListFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/AlignSectionsList.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
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
  return {className(), "Reconstruction", "Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsListFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseFile_Key, "Use Alignment File", "Turn this ON if you wish to use a user defined alignment shifts file", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input .txt file path containing the shifts to apply to the sections", fs::path("DefaultInputFileName"),
                                                          FileSystemPathParameter::ExtensionsType{"txt"}, FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PositioningArrayPath_Key, "Positioning Array", "The positioning array output by another alignment filter",
                                                          DataPath({"Data Container", "Alignment Shifts Data", "Positioning"}), ArraySelectionParameter::AllowedTypes{DataType::uint64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{2}}));

  params.linkParameters(k_UseFile_Key, k_InputFile_Key, true);
  params.linkParameters(k_UseFile_Key, k_PositioningArrayPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType AlignSectionsListFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Description:
  // Swapped Reading Dream3D Shifts from a file to reading them from Data Arrays
  //
  // Change 1:
  // Replaced - k_DREAM3DAlignmentFile_Key = "dream3d_alignment_file" -> k_UseFile_Key = "use_file";
  // Solution - `k_UseFile_Key Value` = not (`!`) `k_DREAM3DAlignmentFile_Key Value`;
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
  auto pUseFileValue = filterArgs.value<BoolParameter::ValueType>(k_UseFile_Key);
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pSelectedImageGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedImageGeometryPath_Key);
  auto pPositioningArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_SelectedImageGeometryPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pSelectedImageGeometryPathValue);
  if(imageGeom.getCellData() == nullptr)
  {
    return MakePreflightErrorResult(-8940, fmt::format("Cannot find cell data Attribute Matrix in the selected Image geometry '{}'", pSelectedImageGeometryPathValue.toString()));
  }

  if(imageGeom.getNumXCells() <= 1 || imageGeom.getNumYCells() <= 1 || imageGeom.getNumZCells() <= 1)
  {
    return MakePreflightErrorResult(-8941, fmt::format("The Image Geometry is not 3D and cannot be run through this filter. The dimensions are ({},{},{})", imageGeom.getNumXCells(),
                                                              imageGeom.getNumYCells(), imageGeom.getNumZCells()));
  }

  if(!pUseFileValue)
  {
    const auto* positioningArray = dataStructure.getDataAs<IDataArray>(pPositioningArrayPathValue);
    if(positioningArray == nullptr)
    {
      return MakePreflightErrorResult(-8942, "Use File disabled, but positioning array is not valid.");
    }

    if(positioningArray->getNumberOfTuples() != imageGeom.getNumZCells())
    {
      return MakePreflightErrorResult(-8943, fmt::format("The size of positioning array ({}) does not align with Z Dimension of geometry ({})", positioningArray->getNumberOfTuples(), imageGeom.getNumZCells()));
    }
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, imageGeom.getCellDataPath(), {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsListFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  AlignSectionsListInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.UseFile = filterArgs.value<bool>(k_UseFile_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.PositioningArrayPath = filterArgs.value<DataPath>(k_PositioningArrayPath_Key);

  return AlignSectionsList(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_DREAM3DAlignmentFileKey = "DREAM3DAlignmentFile";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
} // namespace SIMPL

namespace SIMPLConversionCustom
{
struct AttributeMatrixSelectionFilterParameterConverter
{
  using ParameterType = AttributeMatrixSelectionParameter;
  using ValueType = ParameterType::ValueType;

  static Result<ValueType> convert(const nlohmann::json& json)
  {
    auto dataContainerNameResult = SIMPLConversion::ReadDataContainerName(json, "AttributeMatrixSelectionFilterParameter");
    if(dataContainerNameResult.invalid())
    {
      return ConvertInvalidResult<ValueType>(std::move(dataContainerNameResult));
    }

    DataPath dataPath({std::move(dataContainerNameResult.value())});

    return {std::move(dataPath)};
  }
};
} // namespace SIMPLConversionCustom
} // namespace

Result<Arguments> AlignSectionsListFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignSectionsListFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InvertedBooleanFilterParameterConverter>(args, json, SIMPL::k_DREAM3DAlignmentFileKey, k_UseFile_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversionCustom::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
