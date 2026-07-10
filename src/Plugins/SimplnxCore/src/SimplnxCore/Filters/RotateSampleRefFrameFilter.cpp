#include "RotateSampleRefFrameFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.hpp"

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
#include <array>
#include <cmath>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".rotated_image_geometry";
using RotationRepresentationType = RotateSampleRefFrame::RotationRepresentation;

// Tolerance for treating a rotation-matrix entry as one of {-1, 0, +1}. The axis-angle path
// builds the matrix from cos/sin in float32, so a 90-degree rotation yields entries like
// cos(90) ~= -4.4e-8 (treated as 0) and sin(90) == 1. A tolerance of 1e-4 cleanly separates the
// principal-90 rotations from any off-axis or non-90-degree rotation (e.g. 1 degree gives ~0.017).
constexpr float32 k_Rotation90Tolerance = 1.0e-4f;

constexpr int32 k_NonPrincipalRotation_Error = -6850;
constexpr int32 k_SliceBySliceReordersSlices_Error = -6851;

// -----------------------------------------------------------------------------
// Returns true if the 3x3 rotation block is a proper signed axis-permutation matrix (determinant +1
// with exactly one +/-1 per row and column). This is precisely the octahedral rotation group — the 24
// rotations that map the cubic voxel grid exactly onto itself. It includes the 90/180/270-degree
// rotations about X/Y/Z, but ALSO the 180-degree rotations about a face-diagonal (110) axis and the
// 120/240-degree rotations about a body-diagonal (111) axis. Every such rotation is a lossless
// permutation of the voxels; any other rotation is a lossy nearest-neighbor resample (see the
// RotateSampleRefFrame V&V report).
bool IsLosslessGridRotation(const ImageRotationUtilities::Matrix3fR& rotation, float32 tol)
{
  std::array<int32, 3> rowOnesCount = {0, 0, 0};
  std::array<int32, 3> colOnesCount = {0, 0, 0};
  for(int32 row = 0; row < 3; row++)
  {
    for(int32 col = 0; col < 3; col++)
    {
      const float32 absValue = std::fabs(rotation(row, col));
      const bool isZero = absValue < tol;
      const bool isOne = std::fabs(absValue - 1.0f) < tol;
      if(!isZero && !isOne)
      {
        return false; // entry is not near -1, 0, or +1 -> not a signed permutation matrix
      }
      if(isOne)
      {
        rowOnesCount[row]++;
        colOnesCount[col]++;
      }
    }
  }
  // Exactly one +/-1 per row and per column (a signed permutation matrix)
  for(int32 i = 0; i < 3; i++)
  {
    if(rowOnesCount[i] != 1 || colOnesCount[i] != 1)
    {
      return false;
    }
  }
  // A proper rotation has determinant +1; determinant -1 would be a reflection, not a rotation.
  return std::fabs(rotation.determinant() - 1.0f) < tol;
}
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
                                                                    to_underlying(RotationRepresentationType::AxisAngle), ChoicesParameter::Choices{"Axis Angle", "Rotation Matrix"}));
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

  params.linkParameters(k_RotationRepresentation_Key, k_RotationAxisAngle_Key, std::make_any<uint64>(to_underlying(RotationRepresentationType::AxisAngle)));
  params.linkParameters(k_RotationRepresentation_Key, k_RotationMatrix_Key, std::make_any<uint64>(to_underlying(RotationRepresentationType::RotationMatrix)));

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

  // V&V guard: RotateSampleRefFrame is only a lossless reference-frame rotation when the rotation maps
  // the cubic voxel grid exactly onto itself (a signed axis-permutation / octahedral-group rotation).
  // For any other rotation the nearest-neighbor resample drops/duplicates voxels and introduces
  // background fill, so it is rejected here. Arbitrary rotations belong to "Apply Transformation To Geometry".
  const ImageRotationUtilities::Matrix3fR rotationBlock = rotationMatrix.block(0, 0, 3, 3);
  if(!IsLosslessGridRotation(rotationBlock, k_Rotation90Tolerance))
  {
    return MakePreflightErrorResult(k_NonPrincipalRotation_Error,
                                    "Rotate Sample Reference Frame only supports rotations that map the voxel grid exactly onto itself: the 90/180/270-degree rotations about the X, Y, or Z axis "
                                    "(and, more generally, any rotation of the octahedral symmetry group, such as 120 degrees about (111)). The requested rotation would produce a lossy resampled "
                                    "result. For an arbitrary rotation use the 'Apply Transformation To Geometry' filter instead.");
  }

  // The slice-by-slice option pins each output slice to the same input slice index (planeOld = k). That is
  // only valid when the rotation preserves the Z (slice) axis (rotation about Z, or a 180-degree flip about
  // X/Y). A 90/270-degree rotation about X or Y remaps Z to another axis, so slice-by-slice would index out
  // of bounds and silently zero-fill data.
  auto sliceBySlice = filterArgs.value<bool>(k_RotateSliceBySlice_Key);
  if(sliceBySlice && (std::fabs(std::fabs(rotationBlock(2, 2)) - 1.0f) >= k_Rotation90Tolerance))
  {
    return MakePreflightErrorResult(k_SliceBySliceReordersSlices_Error,
                                    "The 'Perform Slice By Slice Transform' option requires a rotation that preserves the Z (slice) axis: a rotation about the Z axis, or a 180-degree rotation about "
                                    "the X or Y axis. The requested rotation reorders slices, which is incompatible with a slice-by-slice transform.");
  }

  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

  ImageRotationUtilities::RotateArgs rotateArgs = ImageRotationUtilities::CreateRotationArgs(selectedImageGeom, rotationMatrix);
  const std::vector<usize> dims = {static_cast<usize>(rotateArgs.outputDims[0]), static_cast<usize>(rotateArgs.outputDims[1]), static_cast<usize>(rotateArgs.outputDims[2])};
  const std::vector<float32> spacing = {rotateArgs.outputSpacing[0], rotateArgs.outputSpacing[1], rotateArgs.outputSpacing[2]};
  auto origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
  if(!keepInputGeometryOrigin)
  {
    // outputXMin/YMin/ZMin are the ABSOLUTE min corner of the transformed bounding box: DetermineMinMaxCoords
    // transforms ImageGeom::getBoundingBoxf(), which already includes the input origin. Assign (do not add) so
    // the persisted geometry origin equals the RotateArgs::TransformedOrigin the resample worker samples
    // against; adding the input origin again double-counts it for any non-zero input origin.
    origin[0] = rotateArgs.outputXMin;
    origin[1] = rotateArgs.outputYMin;
    origin[2] = rotateArgs.outputZMin;
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
    const AttributeMatrix& selectedCellData = selectedImageGeom.getCellDataRef();
    std::string cellDataName = selectedCellData.getName();
    ignorePaths.push_back(srcImagePath.createChildPath(cellDataName));

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(destImagePath, dims, origin, spacing, cellDataName));

    // Create the Cell AttributeMatrix in the Destination Geometry
    DataPath newCellAttributeMatrixPath = destImagePath.createChildPath(cellDataName);

    for(const auto& [id, object] : selectedCellData)
    {
      const auto& srcArray = dynamic_cast<const IDataArray&>(*object);
      DataType dataType = srcArray.getDataType();
      ShapeType componentShape = srcArray.getIDataStoreRef().getComponentShape();
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
  RotateSampleRefFrameInputValues inputValues;

  inputValues.SourceGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.DestGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);

  auto removeOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  inputValues.KeepInputGeometryOrigin = filterArgs.value<bool>(k_KeepInputGeometryOrigin_Key);

  auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues.SourceGeometryPath);
  auto sourceImageGeomorigin = srcImageGeom.getOrigin();
  if(removeOriginalGeometry)
  {
    auto tempPathVector = inputValues.SourceGeometryPath.getPathVector();
    std::string tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    inputValues.DestGeometryPath = DataPath({tempPathVector});
  }

  inputValues.SliceBySlice = filterArgs.value<bool>(k_RotateSliceBySlice_Key);
  inputValues.RotationRepresentationIndex = filterArgs.value<ChoicesParameter::ValueType>(k_RotationRepresentation_Key);
  inputValues.RotationAxisAngle = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
  inputValues.RotationMatrixTable = filterArgs.value<DynamicTableParameter::ValueType>(k_RotationMatrix_Key);

  return RotateSampleRefFrame(dataStructure, messageHandler, shouldCancel, &inputValues)();
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

  Result<> rotateResult = SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_RotationRepresentationChoiceKey, k_RotationRepresentation_Key);
  if(rotateResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(rotateResult));
  }
  results.push_back(
      SIMPLConversion::Convert2Parameters<SIMPLConversion::FloatVec3p1FilterParameterConverter>(args, json, SIMPL::k_RotationAxisKey, SIMPL::k_RotationAngleKey, k_RotationAxisAngle_Key));
  Result<> tableResult = SIMPLConversion::ConvertParameter<SIMPLConversion::DynamicTableFilterParameterConverter>(args, json, SIMPL::k_RotationTableKey, k_RotationMatrix_Key);
  if(tableResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(tableResult));
  }
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
