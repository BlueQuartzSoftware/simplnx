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
  params.insert(std::make_unique<ChoicesParameter>(k_InputArrayType_Key, "Input Array Type",
                                                   "This selection determines how the input array was produced, Relative refers to the case where the shifts were calculated relative to "
                                                   "the previous slice's new position, Cumulative refers to the case where the shifts are the direct change in position",
                                                   static_cast<ChoicesParameter::ValueType>(to_underlying(AlignSectionsInputType::RelativeShifts)),
                                                   ChoicesParameter::Choices{"Relative", "Cumulative"}));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ShiftsArrayPath_Key, "Shifts Array", "The array containing the relative or cumulative shifts for each slice",
                                                          DataPath({"Data Container", "Alignment Shifts Data", "Shifts"}), ArraySelectionParameter::AllowedTypes{DataType::int64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{2}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType AlignSectionsListFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Description:
  // Swapped Reading Shifts from a file to reading them from Data Arrays
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
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pUseFileValue = filterArgs.value<ChoicesParameter::ValueType>(k_InputArrayType_Key);
  auto pSelectedImageGeometryPathValue = filterArgs.value<GeometrySelectionParameter::ValueType>(k_SelectedImageGeometryPath_Key);
  auto pShiftsArrayPathValue = filterArgs.value<ArraySelectionParameter::ValueType>(k_ShiftsArrayPath_Key);

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
    const auto* shiftsArray = dataStructure.getDataAs<IDataArray>(pShiftsArrayPathValue);
    if(shiftsArray == nullptr)
    {
      return MakePreflightErrorResult(-8942, "Shifts array is not valid.");
    }

    if(shiftsArray->getNumberOfTuples() != imageGeom.getNumZCells())
    {
      return MakePreflightErrorResult(-8943, fmt::format("The size of shifts array ({}) does not align with Z Dimension of geometry ({})", shiftsArray->getNumberOfTuples(), imageGeom.getNumZCells()));
    }
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, imageGeom.getCellDataPath(), {});

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsListFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  AlignSectionsListInputValues inputValues;

  inputValues.AlignSectionsType = filterArgs.value<ChoicesParameter::ValueType>(k_InputArrayType_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.ShiftsArrayPath = filterArgs.value<DataPath>(k_ShiftsArrayPath_Key);

  return AlignSectionsList(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
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

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversionCustom::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
