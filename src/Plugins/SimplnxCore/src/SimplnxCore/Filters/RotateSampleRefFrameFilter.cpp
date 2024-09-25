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
#include "simplnx/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>

#include <fmt/core.h>

#include <algorithm>
#include <array>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <stdexcept>

using namespace nx::core;
using namespace nx::core::ImageRotationUtilities;

namespace
{
const std::string k_TempGeometryName = ".rotated_image_geometry";

using RotationRepresentationType = RotateSampleRefFrameFilter::RotationRepresentation;

constexpr float32 k_Threshold = 0.01f;

const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();
#if 0
void DetermineMinMax(const Matrix3fR& rotationMatrix, const FloatVec3& spacing, usize col, usize row, usize plane, float32& xMin, float32& xMax, float32& yMin, float32& yMax, float32& zMin,
                     float32& zMax)
{
  Eigen::Vector3f coords(static_cast<float32>(col) * spacing[0], static_cast<float32>(row) * spacing[1], static_cast<float32>(plane) * spacing[2]);

  Eigen::Vector3f newCoords = rotationMatrix * coords;

  xMin = std::min(newCoords[0], xMin);
  xMax = std::max(newCoords[0], xMax);

  yMin = std::min(newCoords[1], yMin);
  yMax = std::max(newCoords[1], yMax);

  zMin = std::min(newCoords[2], zMin);
  zMax = std::max(newCoords[2], zMax);
}

float32 CosBetweenVectors(const Eigen::Vector3f& vectorA, const Eigen::Vector3f& vectorB)
{
  float32 normA = vectorA.norm();
  float32 normB = vectorB.norm();

  if(normA == 0.0f || normB == 0.0f)
  {
    return 1.0f;
  }

  return vectorA.dot(vectorB) / (normA * normB);
}

float32 DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew)
{
  float32 xAngle = std::abs(CosBetweenVectors(k_XAxis, axisNew));
  float32 yAngle = std::abs(CosBetweenVectors(k_YAxis, axisNew));
  float32 zAngle = std::abs(CosBetweenVectors(k_ZAxis, axisNew));

  std::array<float32, 3> axes = {xAngle, yAngle, zAngle};

  auto result = std::max_element(axes.cbegin(), axes.cend());

  usize index = std::distance(axes.cbegin(), result);

  return spacing[index];
}

RotateArgs CreateRotateArgs(const ImageGeom& imageGeom, const Matrix3fR& rotationMatrix)
{
  const SizeVec3 origDims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();

  float32 xMin = std::numeric_limits<float32>::max();
  float32 xMax = std::numeric_limits<float32>::min();
  float32 yMin = std::numeric_limits<float32>::max();
  float32 yMax = std::numeric_limits<float32>::min();
  float32 zMin = std::numeric_limits<float32>::max();
  float32 zMax = std::numeric_limits<float32>::min();

  const std::vector<std::array<usize, 3>> coords{{0, 0, 0},
                                                 {origDims[0] - 1, 0, 0},
                                                 {0, origDims[1] - 1, 0},
                                                 {origDims[0] - 1, origDims[1] - 1, 0},
                                                 {0, 0, origDims[2] - 1},
                                                 {origDims[0] - 1, 0, origDims[2] - 1},
                                                 {0, origDims[1] - 1, origDims[2] - 1},
                                                 {origDims[0] - 1, origDims[1] - 1, origDims[2] - 1}};

  for(const auto& item : coords)
  {
    DetermineMinMax(rotationMatrix, spacing, item[0], item[1], item[2], xMin, xMax, yMin, yMax, zMin, zMax);
  }

  Eigen::Vector3f xAxisNew = rotationMatrix * k_XAxis;
  Eigen::Vector3f yAxisNew = rotationMatrix * k_YAxis;
  Eigen::Vector3f zAxisNew = rotationMatrix * k_ZAxis;

  float32 xResNew = DetermineSpacing(spacing, xAxisNew);
  float32 yResNew = DetermineSpacing(spacing, yAxisNew);
  float32 zResNew = DetermineSpacing(spacing, zAxisNew);

  ImageGeom::MeshIndexType xpNew = static_cast<int64>(std::nearbyint((xMax - xMin) / xResNew) + 1);
  ImageGeom::MeshIndexType ypNew = static_cast<int64>(std::nearbyint((yMax - yMin) / yResNew) + 1);
  ImageGeom::MeshIndexType zpNew = static_cast<int64>(std::nearbyint((zMax - zMin) / zResNew) + 1);

  RotateArgs params;

  params.xp = static_cast<int64>(origDims[0]);
  params.xRes = spacing[0];
  params.yp = static_cast<int64>(origDims[1]);
  params.yRes = spacing[1];
  params.zp = static_cast<int64>(origDims[2]);
  params.zRes = spacing[2];

  params.xpNew = static_cast<int64>(xpNew);
  params.xResNew = xResNew;
  params.xMinNew = xMin;
  params.ypNew = static_cast<int64>(ypNew);
  params.yResNew = yResNew;
  params.yMinNew = yMin;
  params.zpNew = static_cast<int64>(zpNew);
  params.zResNew = zResNew;
  params.zMinNew = zMin;

  return params;
}

template <typename K>
bool closeEnough(const K& a, const K& b, const K& epsilon = std::numeric_limits<K>::epsilon())
{
  return (epsilon > fabs(a - b));
}

constexpr RotationRepresentationType CastIndexToRotationRepresentation(uint64 index)
{
  switch(index)
  {
  case to_underlying(RotationRepresentationType::AxisAngle): {
    return RotationRepresentationType::AxisAngle;
  }
  case to_underlying(RotationRepresentationType::RotationMatrix): {
    return RotationRepresentationType::RotationMatrix;
  }
  default: {
    throw std::runtime_error(fmt::format("RotateSampleRefFrameFilter: Failed to cast index {} to RotationRepresentation", index));
  }
  }
}

Result<Matrix3fR> ConvertAxisAngleToRotationMatrix(const std::vector<float32>& pRotationValue)
{
  Matrix3fR transformationMatrix;

  // Convert Degrees to Radians for the last element
  const float rotAngle = pRotationValue[3] * Constants::k_PiOver180F;
  // Ensure the axis part is normalized
  FloatVec3 normalizedAxis(pRotationValue[0], pRotationValue[1], pRotationValue[2]);
  MatrixMath::Normalize3x1<float32>(normalizedAxis.data());

  const float cosTheta = cos(rotAngle);
  const float oneMinusCosTheta = 1 - cosTheta;
  const float sinTheta = sin(rotAngle);
  const float l = normalizedAxis[0];
  const float m = normalizedAxis[1];
  const float n = normalizedAxis[2];

  // First Row:
  transformationMatrix(0, 0) = l * l * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(0, 1) = m * l * (oneMinusCosTheta) - (n * sinTheta);
  transformationMatrix(0, 2) = n * l * (oneMinusCosTheta) + (m * sinTheta);

  // Second Row:
  transformationMatrix(1, 0) = l * m * (oneMinusCosTheta) + (n * sinTheta);
  transformationMatrix(1, 1) = m * m * (oneMinusCosTheta) + cosTheta;
  transformationMatrix(1, 2) = n * m * (oneMinusCosTheta) - (l * sinTheta);

  // Third Row:
  transformationMatrix(2, 0) = l * n * (oneMinusCosTheta) - (m * sinTheta);
  transformationMatrix(2, 1) = m * n * (oneMinusCosTheta) + (l * sinTheta);
  transformationMatrix(2, 2) = n * n * (oneMinusCosTheta) + cosTheta;

  return {transformationMatrix};
}

Result<Matrix3fR> ConvertRotationTableToRotationMatrix(const std::vector<std::vector<float64>>& rotationMatrixTable)
{
  if(rotationMatrixTable.size() != 3)
  {
    return MakeErrorResult<Matrix3fR>(-45004, "Rotation Matrix must be 3 x 3");
  }

  for(const auto& row : rotationMatrixTable)
  {
    if(row.size() != 3)
    {
      return MakeErrorResult<Matrix3fR>(-45005, "Rotation Matrix must be 3 x 3");
    }
  }
  Matrix3fR rotationMatrix;
  const usize numTableRows = rotationMatrixTable.size();
  const usize numTableCols = rotationMatrixTable[0].size();
  for(size_t rowIndex = 0; rowIndex < numTableRows; rowIndex++)
  {
    std::vector<double> row = rotationMatrixTable[rowIndex];
    for(size_t colIndex = 0; colIndex < numTableCols; colIndex++)
    {
      rotationMatrix(rowIndex, colIndex) = static_cast<float>(row[colIndex]);
    }
  }

  float32 determinant = rotationMatrix.determinant();

  if(!closeEnough(determinant, 1.0f, k_Threshold))
  {
    return MakeErrorResult<Matrix3fR>(-45006, fmt::format("Rotation Matrix must have a determinant of 1 (is {})", determinant));
  }

  Matrix3fR transpose = rotationMatrix.transpose();
  Matrix3fR inverse = rotationMatrix.inverse();

  if(!transpose.isApprox(inverse, k_Threshold))
  {
    return MakeErrorResult<Matrix3fR>(-45007, "Rotation Matrix's inverse and transpose must be equal");
  }

  return {rotationMatrix};
}
#endif

// Result<Matrix3fR> ComputeRotationMatrix(const Arguments& args)
//{
//   auto rotationRepresentationIndex = args.value<uint64>(RotateSampleRefFrameFilter::k_RotationRepresentation_Key);
//
//   RotationRepresentationType rotationRepresentation = CastIndexToRotationRepresentation(rotationRepresentationIndex);
//
//   switch(rotationRepresentation)
//   {
//   case RotationRepresentationType::AxisAngle: {
//     auto pRotationValue = args.value<std::vector<float32>>(RotateSampleRefFrameFilter::k_RotationAxisAngle_Key);
//     return ConvertAxisAngleToRotationMatrix(pRotationValue);
//   }
//   case RotationRepresentationType::RotationMatrix: {
//     auto rotationMatrixTable = args.value<DynamicTableParameter::ValueType>(RotateSampleRefFrameFilter::k_RotationMatrix_Key);
//     return ConvertRotationTableToRotationMatrix(rotationMatrixTable);
//   }
//   }
// }

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
IFilter::UniquePointer RotateSampleRefFrameFilter::clone() const
{
  return std::make_unique<RotateSampleRefFrameFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RotateSampleRefFrameFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler&, const std::atomic_bool&) const
{

  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pRemoveOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);

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
  origin[0] += rotateArgs.xMinNew;
  origin[1] += rotateArgs.yMinNew;
  origin[2] += rotateArgs.zMinNew;

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
                                                 const std::atomic_bool& shouldCancel) const
{
  auto srcImagePath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto destImagePath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto sliceBySlice = filterArgs.value<bool>(k_RotateSliceBySlice_Key);
  auto removeOriginalGeometry = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);

  auto& srcImageGeom = dataStructure.getDataRefAs<ImageGeom>(srcImagePath);

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
