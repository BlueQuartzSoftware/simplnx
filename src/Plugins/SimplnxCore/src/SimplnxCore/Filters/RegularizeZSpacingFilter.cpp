#include "RegularizeZSpacingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RegularizeZSpacing.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".regularized_image_geometry";

// Rebases 'path' from under 'oldPrefix' to the same relative location under 'newPrefix'.
// A component-wise splice is used instead of string replacement so that a geometry name that
// appears as a substring elsewhere in the path cannot corrupt the rebased path.
nx::core::DataPath RebasePath(const nx::core::DataPath& path, const nx::core::DataPath& oldPrefix, const nx::core::DataPath& newPrefix)
{
  auto pathVector = path.getPathVector();
  std::vector<std::string> newVector = newPrefix.getPathVector();
  newVector.insert(newVector.end(), pathVector.begin() + static_cast<std::ptrdiff_t>(oldPrefix.getLength()), pathVector.end());
  return nx::core::DataPath(newVector);
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RegularizeZSpacingFilter::name() const
{
  return FilterTraits<RegularizeZSpacingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RegularizeZSpacingFilter::className() const
{
  return FilterTraits<RegularizeZSpacingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RegularizeZSpacingFilter::uuid() const
{
  return FilterTraits<RegularizeZSpacingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RegularizeZSpacingFilter::humanName() const
{
  return "Regularize Z Spacing";
}

//------------------------------------------------------------------------------
std::vector<std::string> RegularizeZSpacingFilter::defaultTags() const
{
  return {className(), "Sampling", "Spacing", "Image Geometry", "Resolution", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters RegularizeZSpacingFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  // These extensions are the file types the Z-positions reader accepts. Keep acceptAllExtensions off
  // so DREAM3D-NX only offers this filter for those extensions on file drop.
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Current Z Positions File",
                                                          "Path to a whitespace-delimited text file that contains the current Z position of each slice boundary (ZPoints + 1 float values).",
                                                          fs::path(""), FileSystemPathParameter::ExtensionsType{".txt", ".dat", ".csv"}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<Float32Parameter>(k_NewZRes_Key, "New Z Spacing", "The new, regular spacing to use along the Z axis. Must be greater than 0.", 1.0F));

  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In Place", "Removes the original Image Geometry after the filter is complete and renames the result to its name.", true));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The Image Geometry to resample along the Z axis.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Feature Attribute Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_ClearFeatureAttributeMatrix_Key, "Clear Feature Attribute Matrix",
      "Recreates the selected Feature Attribute Matrix in the resampled geometry with no arrays. Resampling changes which cells belong to each feature, so every feature value computed before this "
      "filter is invalid. Recompute any feature data needed later in the pipeline.",
      true));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrix_Key, "Feature Attribute Matrix", "DataPath to the feature Attribute Matrix", DataPath({"Cell Feature Data"})));
  params.insert(std::make_unique<BoolParameter>(k_RenumberFeatures_Key, "Renumber Features",
                                                "Renumber Feature Ids to 0..N and resize the cleared Feature Attribute Matrix to the features that remain after resampling", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Image Geometry", "The path to the resampled Image Geometry.", DataPath()));

  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_FeatureAttributeMatrix_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_RenumberFeatures_Key, true);
  params.linkParameters(k_ClearFeatureAttributeMatrix_Key, k_CellFeatureIdsArrayPath_Key, true);
  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometry_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RegularizeZSpacingFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RegularizeZSpacingFilter::clone() const
{
  return std::make_unique<RegularizeZSpacingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RegularizeZSpacingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto inputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto newZRes = filterArgs.value<float32>(k_NewZRes_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  const bool clearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  const bool renumberFeatures = clearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  const auto cellFeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrix_Key);
  const auto featureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(newZRes <= 0.0F)
  {
    return MakePreflightErrorResult(-5555, fmt::format("The new Z spacing ({}) must be greater than 0.", newZRes));
  }

  const auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  const SizeVec3 srcDimensions = srcImageGeom.getDimensions();
  const FloatVec3 srcSpacing = srcImageGeom.getSpacing();
  auto srcOrigin = srcImageGeom.getOrigin().toContainer<std::vector<float32>>();
  const usize origZDim = srcDimensions[2];

  // Read and validate the Z boundary positions file (ZPoints + 1 values).
  Result<std::vector<float32>> zBoundsResult = ReadZBoundsFile(inputFile, origZDim + 1);
  if(zBoundsResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(zBoundsResult)), {})};
  }
  const std::vector<float32> zBoundValues = zBoundsResult.value();

  // The Z boundary positions must be monotonically non-decreasing.
  for(usize i = 1; i < zBoundValues.size(); i++)
  {
    if(zBoundValues[i] < zBoundValues[i - 1])
    {
      return MakePreflightErrorResult(-5558, fmt::format("The Z boundary positions in '{}' must be monotonically non-decreasing, but value {} ({}) is less than value {} ({}).", inputFile.string(), i,
                                                         zBoundValues[i], i - 1, zBoundValues[i - 1]));
    }
  }

  const float32 lastZBound = zBoundValues[origZDim];
  if(lastZBound <= 0.0F)
  {
    return MakePreflightErrorResult(-5559, fmt::format("The total Z extent (last value in '{}') must be greater than 0, but was {}.", inputFile.string(), lastZBound));
  }

  const usize newZDim = ComputeRegularizedZDim(lastZBound, newZRes);

  std::vector<usize> geomDims = {srcDimensions[0], srcDimensions[1], newZDim}; // ImageGeometry dimensions go fastest to slowest, XYZ.
  std::vector<usize> dataArrayShape = {geomDims[2], geomDims[1], geomDims[0]}; // DataArray shape goes slowest to fastest, ZYX.
  const FloatVec3 destSpacing = {srcSpacing[0], srcSpacing[1], newZRes};

  std::vector<DataPath> ignorePaths; // already handled, so skip these when copying remaining child paths

  if(pRemoveOriginalGeometry)
  {
    // Rename the current Image Geometry out of the way, then delete it after execute.
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcImagePath, tempName));
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcImagePath.getPathVector();
    tempPathVector.back() = k_TempGeometryName;
    destImagePath = DataPath({tempPathVector});
  }

  // Create the destination Image Geometry and the resampled cell data arrays.
  {
    const AttributeMatrix* selectedCellData = srcImageGeom.getCellData();
    if(selectedCellData == nullptr)
    {
      return MakePreflightErrorResult(-5560, fmt::format("'{}' must have a cell data Attribute Matrix.", srcImagePath.toString()));
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(
        std::make_unique<CreateImageGeometryAction>(destImagePath, geomDims, srcOrigin, CreateImageGeometryAction::SpacingType{destSpacing[0], destSpacing[1], destSpacing[2]}, cellDataName));

    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);
    for(const auto& [identifier, object] : *selectedCellData)
    {
      // Guard instead of a reference cast: a cell AttributeMatrix may hold non-DataArray members
      // (StringArray, NeighborList) which this filter cannot remap; fail cleanly rather than throw.
      const auto* srcArrayPtr = dynamic_cast<const IDataArray*>(object.get());
      if(srcArrayPtr == nullptr)
      {
        return MakePreflightErrorResult(
            -5561, fmt::format("Cell Attribute Matrix member '{}' is not a DataArray and cannot be resampled by this filter. Remove it from '{}' or move it out of the cell Attribute Matrix.",
                               object->getName(), srcImagePath.createChildPath(cellDataName).toString()));
      }
      DataType dataType = srcArrayPtr->getDataType();
      ShapeType componentShape = srcArrayPtr->getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArrayPtr->getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }

    preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom.getDimensions(), srcImageGeom.getSpacing(),
                                                                                                                          srcImageGeom.getOrigin(), srcImageGeom.getUnits())});
    preflightUpdatedValues.push_back({"Regularized Image Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(geomDims, destSpacing, srcOrigin, srcImageGeom.getUnits())});
  }

  // The cleared Feature AM keeps its source shape until optional renumbering.
  if(clearFeatureAttributeMatrix)
  {
    const auto* srcCellFeatureData = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAttributeMatrixPath);
    if(srcCellFeatureData == nullptr || !srcImageGeom.contains(srcCellFeatureData))
    {
      return MakePreflightErrorResult(-67431, fmt::format("The Feature Attribute Matrix '{}' must be a direct child of the selected Image Geometry '{}'. Select an Attribute Matrix that is a direct "
                                                          "child of the geometry, or turn off 'Clear Feature Attribute Matrix'.",
                                                          cellFeatureAttributeMatrixPath.toString(), srcImagePath.toString()));
    }
    if(srcImageGeom.getCellData() == srcCellFeatureData)
    {
      return MakePreflightErrorResult(
          -67432,
          fmt::format("The Feature Attribute Matrix '{}' is the Cell Attribute Matrix of '{}'. Select the Attribute Matrix that holds Feature data, or turn off 'Clear Feature Attribute Matrix'.",
                      cellFeatureAttributeMatrixPath.toString(), srcImagePath.toString()));
    }

    auto elementDataResult = ValidateFeatureAttributeMatrixNotElementData(dataStructure, cellFeatureAttributeMatrixPath);
    if(elementDataResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(elementDataResult))};
    }
    if(renumberFeatures)
    {
      auto locationResult = ValidateArrayInGeometryCellData(dataStructure, srcImagePath, featureIdsArrayPath, -67433, "Cell Feature Ids");
      if(locationResult.invalid())
      {
        return {ConvertResultTo<OutputActions>(std::move(locationResult), {})};
      }
      const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
      if(featureIds.getNumberOfTuples() != srcImageGeom.getNumberOfCells())
      {
        return MakePreflightErrorResult(-67434,
                                        fmt::format("**Cell Feature Ids** DataArray '{}' has {} tuples, but the selected Image Geometry '{}' has {} cells. Select an array with one tuple per cell.",
                                                    featureIdsArrayPath.toString(), featureIds.getNumberOfTuples(), srcImagePath.toString(), srcImageGeom.getNumberOfCells()));
      }
    }

    // Ignore every path to the Feature AM, including selections through another parent.
    const auto featureAmPaths = srcCellFeatureData->getDataPaths();
    ignorePaths.insert(ignorePaths.end(), featureAmPaths.begin(), featureAmPaths.end());
    const DataPath destCellFeatureAttributeMatrixPath = destImagePath.createChildPath(cellFeatureAttributeMatrixPath.getTargetName());
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(destCellFeatureAttributeMatrixPath, srcCellFeatureData->getShape()));

    std::string clearedList;
    for(const auto& [identifier, object] : *srcCellFeatureData)
    {
      clearedList += "\n" + cellFeatureAttributeMatrixPath.createChildPath(object->getName()).toString();
    }
    std::string message = fmt::format("The Feature Attribute Matrix '{}' is recreated in the resampled geometry with no arrays.", cellFeatureAttributeMatrixPath.toString());
    if(!clearedList.empty())
    {
      message += fmt::format(" Its values describe the original geometry and are not copied. Recompute any of the following Data Arrays:\n{}", clearedList);
    }
    if(renumberFeatures)
    {
      message += "\n\nFeature Ids are renumbered.\nThe Attribute Matrix is resized to the number of features remaining after the resample.";
    }
    preflightUpdatedValues.push_back({"Cleared Feature Data", std::move(message)});
  }
  else
  {
    preflightUpdatedValues.push_back({"Feature Data Not Cleared", "Attribute Matrices other than the Cell Attribute Matrix are copied to the resampled geometry unchanged. If one of them holds "
                                                                  "Feature data, its values describe the original geometry and are not valid for the resampled geometry."});
  }

  // Copy any remaining loose data groups or data arrays from the source geometry.
  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      DataPath copiedChildPath = ::RebasePath(childPath, srcImagePath, destImagePath);
      AppendCopyDataObjectAction(dataStructure, resultOutputActions.value(), childPath, copiedChildPath, clearFeatureAttributeMatrix ? cellFeatureAttributeMatrixPath : DataPath{});
    }
  }

  if(pRemoveOriginalGeometry)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));

    // Inform downstream filters that the cell data arrays are modified in place.
    nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, srcImageGeom.getCellDataPath(), {});
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RegularizeZSpacingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter*, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RegularizeZSpacingInputValues inputValues;

  inputValues.SelectedImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.NewZRes = filterArgs.value<float32>(k_NewZRes_Key);
  inputValues.RemoveOriginalImageGeom = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.CreatedImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  inputValues.ClearFeatureAttributeMatrix = filterArgs.value<BoolParameter::ValueType>(k_ClearFeatureAttributeMatrix_Key);
  inputValues.RenumberFeatures = inputValues.ClearFeatureAttributeMatrix && filterArgs.value<BoolParameter::ValueType>(k_RenumberFeatures_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellFeatureAttributeMatrix = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_FeatureAttributeMatrix_Key);

  if(inputValues.RemoveOriginalImageGeom)
  {
    auto tempPathVector = inputValues.SelectedImageGeometryPath.getPathVector();
    tempPathVector.back() = k_TempGeometryName;
    inputValues.CreatedImageGeometryPath = DataPath({tempPathVector});
  }

  return RegularizeZSpacing(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_NewZResKey = "NewZRes";
} // namespace SIMPL
} // namespace

Result<Arguments> RegularizeZSpacingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RegularizeZSpacingFilter().getDefaultArguments();
  // The legacy filter had no Feature parameters and copied every non-Cell Attribute Matrix unchanged.
  args.insertOrAssign(k_ClearFeatureAttributeMatrix_Key, std::make_any<BoolParameter::ValueType>(false));

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey,
                                                                                                                                      k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_NewZResKey, k_NewZRes_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
