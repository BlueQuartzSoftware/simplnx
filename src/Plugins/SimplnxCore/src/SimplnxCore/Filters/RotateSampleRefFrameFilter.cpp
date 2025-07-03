#include "RotateSampleRefFrameFilter.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>

#include <fmt/core.h>

#include <algorithm>

using namespace nx::core;
using namespace nx::core::ImageRotationUtilities;

namespace
{
const std::string k_TempGeometryName = ".rotated_image_geometry";

using RotationRepresentationType = RotateSampleRefFrameFilter::RotationRepresentation;

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RotateSampleRefFrameFilter::name() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::name;
}

//------------------------------------------------------------------------------
std::string RotateSampleRefFrameFilter::className() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RotateSampleRefFrameFilter::uuid() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RotateSampleRefFrameFilter::humanName() const
{
  return "Rotate Sample Reference Frame";
}

//------------------------------------------------------------------------------
std::vector<std::string> RotateSampleRefFrameFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion", "ReferenceFrame"};
}

//------------------------------------------------------------------------------
Parameters RotateSampleRefFrameFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_RotateSliceBySlice_Key, "Perform Slice By Slice Transform", "This option is specific to EBSD Data and is not generally used.", false));

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_RotationRepresentation_Key, "Rotation Representation", "Which form used to represent rotation (axis angle or rotation matrix)",
                                                                    to_underlying(RotationRepresentation::AxisAngle), ChoicesParameter::Choices{"Axis Angle", "Rotation Matrix"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxisAngle_Key, "Rotation Axis-Angle [<ijk>w]", "Axis-Angle in sample reference frame to rotate about.",
                                                         VectorFloat32Parameter::ValueType{0.0f, 0.0f, 1.0f, 90.0F}, std::vector<std::string>{"i", "j", "k", "w (Deg)"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In-Place Rotation", "Performs the rotation in-place for the given Image Geometry", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_KeepInputGeometryOrigin_Key, "Keep Input Geometry's Origin", "The input geometry's origin is kept instead of the origin resulting from the transform", false));

  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo({"1", "2", "3", "4"}));
  const DynamicTableInfo::TableDataType defaultTable{{{1.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 1.0F}}};
  params.insert(std::make_unique<DynamicTableParameter>(k_RotationMatrix_Key, "Transformation Matrix", "The 4x4 Transformation Matrix", defaultTable, tableInfo));

  params.linkParameters(k_RotationRepresentation_Key, k_RotationAxisAngle_Key, std::make_any<uint64>(to_underlying(RotationRepresentation::AxisAngle)));
  params.linkParameters(k_RotationRepresentation_Key, k_RotationMatrix_Key, std::make_any<uint64>(to_underlying(RotationRepresentation::RotationMatrix)));

  params.insertSeparator(Parameters::Separator{"Input Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the rotation", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Output Geometry and Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Created Image Geometry", "The location of the rotated geometry", DataPath{}));

  params.linkParameters(k_RemoveOriginalGeometry_Key, k_CreatedImageGeometryPath_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RotateSampleRefFrameFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RotateSampleRefFrameFilter::clone() const
{
  return std::make_unique<RotateSampleRefFrameFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RotateSampleRefFrameFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto keepInputGeometryOrigin = filterArgs.value<bool>(k_KeepInputGeometryOrigin_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  ImageRotationUtilities::Matrix4fR rotationMatrix;

  auto rotationRepresentationIndex = filterArgs.value<uint64>(RotateSampleRefFrameFilter::k_RotationRepresentation_Key);
  switch(rotationRepresentationIndex)
  {
  case static_cast<uint64>(RotationRepresentationType::AxisAngle): {
    auto pRotationValue = filterArgs.value<std::vector<float32>>(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key);
    rotationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
    break;
  }
  case static_cast<uint64>(RotationRepresentationType::RotationMatrix): {
    auto rotationMatrixTable = filterArgs.value<DynamicTableParameter::ValueType>(RotateSampleRefFrameFilter::k_RotationMatrix_Key);
    rotationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(rotationMatrixTable);
    break;
  }
  }

  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

  ImageRotationUtilities::RotateArgs rotateArgs = ImageRotationUtilities::CreateRotationArgs(selectedImageGeom, rotationMatrix);
  const std::vector<usize> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
  const std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
  auto origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
  if(!keepInputGeometryOrigin)
  {
    origin[0] += rotateArgs.xMinNew;
    origin[1] += rotateArgs.yMinNew;
    origin[2] += rotateArgs.zMinNew;
  }

  std::vector<usize> dataArrayShape = {dims[2], dims[1], dims[0]}; // The DataArray shape goes slowest to fastest (ZYX)

  std::vector<DataPath> ignorePaths; // already copied over so skip these when collecting child paths to finish copying over later

  if(pRemoveOriginalGeometry)
  {
    // Generate a new name for the current Image Geometry
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = "." + tempPathVector.back();
    tempPathVector.back() = tempName;
    DataPath tempPath(tempPathVector);
    // Rename the current image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(srcImagePath, tempName));
    // After the execute function has been done, delete the moved image geometry
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(tempPath));

    tempPathVector = srcImagePath.getPathVector();
    tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  {
    const AttributeMatrix* selectedCellData = selectedImageGeom.getCellData();
    if(selectedCellData == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5951, fmt::format("'{}' must have cell data attribute matrix", srcImagePath.toString()))};
    }
    std::string cellDataName = selectedCellData->getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(destImagePath, dims, origin, spacing, cellDataName));

    // Create the Cell AttributeMatrix in the Destination Geometry
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);

    for(const auto& [id, object] : *selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      IDataStore::ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
      DataPath dataArrayPath = newCellAttributeMatrixPath.createChildPath(srcArray.getName());
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(dataType, dataArrayShape, std::move(componentShape), dataArrayPath));
    }

    // Store the preflight updated value(s) into the preflightUpdatedValues vector using
    // the appropriate methods.
    // These values should have been updated during the preflightImpl(...) method
    const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(srcImagePath);

    preflightUpdatedValues.push_back({"Input Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(srcImageGeom->getDimensions(), srcImageGeom->getSpacing(),
                                                                                                                          srcImageGeom->getOrigin(), srcImageGeom->getUnits())});
    preflightUpdatedValues.push_back({"Rotated Image Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(
                                                                         dims, CreateImageGeometryAction::SpacingType{spacing[0], spacing[1], spacing[2]}, origin, srcImageGeom->getUnits())});
  }

  // copy over the rest of the data
  auto childPaths = GetAllChildDataPaths(dataStructure, srcImagePath, DataObject::Type::DataObject, ignorePaths);
  if(childPaths.has_value())
  {
    for(const auto& childPath : childPaths.value())
    {
      std::string copiedChildName = nx::core::StringUtilities::replace(childPath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
      DataPath copiedChildPath = DataPath::FromString(copiedChildName).value();
      if(dataStructure.getDataAs<BaseGroup>(childPath) != nullptr)
      {
        std::vector<DataPath> allCreatedPaths = {copiedChildPath};
        auto pathsToBeCopied = GetAllChildDataPathsRecursive(dataStructure, childPath);
        if(pathsToBeCopied.has_value())
        {
          for(const auto& sourcePath : pathsToBeCopied.value())
          {
            std::string createdPathName = nx::core::StringUtilities::replace(sourcePath.toString(), srcImagePath.getTargetName(), destImagePath.getTargetName());
            allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
          }
        }
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, allCreatedPaths));
      }
      else
      {
        resultOutputActions.value().appendAction(std::make_unique<CopyDataObjectAction>(childPath, copiedChildPath, std::vector<DataPath>{copiedChildPath}));
      }
    }
  }

  if(pRemoveOriginalGeometry)
  {
    resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(destImagePath, srcImagePath.getTargetName()));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

Result<> RotateSampleRefFrameFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto sliceBySlice = filterArgs.value<bool>(k_RotateSliceBySlice_Key);
  auto removeOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto keepInputGeometryOrigin = filterArgs.value<bool>(k_KeepInputGeometryOrigin_Key);

  auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);
  auto sourceImageGeomorigin = srcImageGeom.getOrigin();
  if(removeOriginalGeometry)
  {
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& destImageGeom = dataStructure.getDataRefAs<ImageGeom>(destImagePath);

  ImageRotationUtilities::Matrix4fR rotationMatrix;

  auto rotationRepresentationIndex = filterArgs.value<uint64>(RotateSampleRefFrameFilter::k_RotationRepresentation_Key);
  switch(rotationRepresentationIndex)
  {
  case static_cast<uint64>(RotationRepresentationType::AxisAngle): {
    auto pRotationValue = filterArgs.value<std::vector<float32>>(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key);
    rotationMatrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(pRotationValue);
    break;
  }
  case static_cast<uint64>(RotationRepresentationType::RotationMatrix): {
    auto rotationMatrixTable = filterArgs.value<DynamicTableParameter::ValueType>(RotateSampleRefFrameFilter::k_RotationMatrix_Key);
    rotationMatrix = ImageRotationUtilities::GenerateManualTransformationMatrix(rotationMatrixTable);
    break;
  }
  }

  ImageRotationUtilities::RotateArgs rotateArgs = ImageRotationUtilities::CreateRotationArgs(srcImageGeom, rotationMatrix);

  auto selectedCellDataChildren = GetAllChildArrayDataPaths(dataStructure, srcImageGeom.getCellDataPath());
  auto selectedCellArrays = selectedCellDataChildren.has_value() ? selectedCellDataChildren.value() : std::vector<DataPath>{};

  ImageRotationUtilities::FilterProgressCallback filterProgressCallback(messageHandler, shouldCancel);

  // The actual rotating of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  const DataPath srcCelLDataAMPath = srcImageGeom.getCellDataPath();
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();

  const DataPath destCellDataAMPath = destImageGeom.getCellDataPath();

  for(const auto& [dataId, srcDataObject] : srcCellDataAM)
  {
    if(shouldCancel)
    {
      return {};
    }

    const auto* srcDataArray = dataStructure.getDataAs<IDataArray>(srcCelLDataAMPath.createChildPath(srcDataObject->getName()));
    auto* destDataArray = dataStructure.getDataAs<IDataArray>(destCellDataAMPath.createChildPath(srcDataObject->getName()));
    messageHandler(fmt::format("Rotating Volume || Copying Data Array {}", srcDataObject->getName()));

    ExecuteParallelFunction<ImageRotationUtilities::RotateImageGeometryWithNearestNeighbor>(srcDataArray->getDataType(), taskRunner, srcDataArray, destDataArray, rotateArgs, rotationMatrix,
                                                                                            sliceBySlice, &filterProgressCallback);
  }

  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  if(keepInputGeometryOrigin)
  {
    destImageGeom.setOrigin(srcImageGeom.getOrigin());
  }
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_RotationRepresentationChoiceKey = "RotationRepresentationChoice";
constexpr StringLiteral k_RotationAngleKey = "RotationAngle";
constexpr StringLiteral k_RotationAxisKey = "RotationAxis";
constexpr StringLiteral k_RotationTableKey = "RotationTable";
constexpr StringLiteral k_CellAttributeMatrixPathKey = "CellAttributeMatrixPath";
} // namespace SIMPL
} // namespace

Result<Arguments> RotateSampleRefFrameFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RotateSampleRefFrameFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_RotationRepresentationChoiceKey, k_RotationRepresentation_Key));
  results.push_back(
      SIMPLConversion::Convert2Parameters<SIMPLConversion::FloatVec3p1FilterParameterConverter>(args, json, SIMPL::k_RotationAxisKey, SIMPL::k_RotationAngleKey, k_RotationAxisAngle_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DynamicTableFilterParameterConverter>(args, json, SIMPL::k_RotationTableKey, k_RotationMatrix_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
