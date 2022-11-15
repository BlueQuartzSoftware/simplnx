#include "RotateSampleRefFrameFilter.hpp"

#include "Core/Utilities/CoreUtilities.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/Range.hpp"
#include "complex/Common/Range3D.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelData3DAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <Eigen/Dense>

#include <fmt/core.h>

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

#ifdef COMPLEX_ENABLE_MULTICORE
#define RUN_TASK g->run
#else
#define RUN_TASK
#endif

using namespace complex;

namespace
{
using RotationRepresentation = RotateSampleRefFrameFilter::RotationRepresentation;
using Matrix3fR = Eigen::Matrix<float32, 3, 3, Eigen::RowMajor>;

constexpr float32 k_Threshold = 0.01f;

const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

/**
 * @brief Internal struct to pass around the arguments
 */
struct RotateArgs
{
  int64 xp = 0;
  int64 yp = 0;
  int64 zp = 0;
  float32 xRes = 0.0f;
  float32 yRes = 0.0f;
  float32 zRes = 0.0f;
  int64 xpNew = 0;
  int64 ypNew = 0;
  int64 zpNew = 0;
  float32 xResNew = 0.0f;
  float32 yResNew = 0.0f;
  float32 zResNew = 0.0f;
  float32 xMinNew = 0.0f;
  float32 yMinNew = 0.0f;
  float32 zMinNew = 0.0f;
};

/**
 * @brief Class that implements the actual rotation
 */
class SampleRefFrameRotator
{
public:
  SampleRefFrameRotator(nonstd::span<int64> newIndices, const RotateArgs& args, const Matrix3fR& rotationMatrix, bool sliceBySlice)
  : m_NewIndices(newIndices)
  , m_SliceBySlice(sliceBySlice)
  , m_Params(args)
  {
    // We have to inline the 3x3 Matrix transpose here because of the "const" nature of the 'convert' function
    Matrix3fR transpose = rotationMatrix.transpose();
    // Need to use row based Eigen matrix so that the values get mapped to the right place in the raw array
    // Raw array is faster than Eigen
    Eigen::Map<Matrix3fR>(&m_RotMatrixInv[0][0], transpose.rows(), transpose.cols()) = transpose;
  }
  ~SampleRefFrameRotator() = default;

  SampleRefFrameRotator(const SampleRefFrameRotator&) = default;
  SampleRefFrameRotator(SampleRefFrameRotator&&) noexcept = delete;
  SampleRefFrameRotator& operator=(const SampleRefFrameRotator&) = delete;
  SampleRefFrameRotator& operator=(SampleRefFrameRotator&&) noexcept = delete;

  void convert(int64 zStart, int64 zEnd, int64 yStart, int64 yEnd, int64 xStart, int64 xEnd) const
  {
    for(int64 k = zStart; k < zEnd; k++)
    {
      int64 ktot = (m_Params.xpNew * m_Params.ypNew) * k;
      for(int64 j = yStart; j < yEnd; j++)
      {
        int64 jtot = (m_Params.xpNew) * j;
        for(int64 i = xStart; i < xEnd; i++)
        {
          int64 index = ktot + jtot + i;
          m_NewIndices[index] = -1;

          std::array<float32, 3> coords = {0.0f, 0.0f, 0.0f};
          std::array<float32, 3> coordsNew = {0.0f, 0.0f, 0.0f};

          coords[0] = (static_cast<float32>(i) * m_Params.xResNew) + m_Params.xMinNew;
          coords[1] = (static_cast<float32>(j) * m_Params.yResNew) + m_Params.yMinNew;
          coords[2] = (static_cast<float32>(k) * m_Params.zResNew) + m_Params.zMinNew;

          MatrixMath::Multiply3x3with3x1(m_RotMatrixInv, coords.data(), coordsNew.data());

          auto colOld = static_cast<int64>(std::nearbyint(coordsNew[0] / m_Params.xRes));
          auto rowOld = static_cast<int64>(std::nearbyint(coordsNew[1] / m_Params.yRes));
          auto planeOld = static_cast<int64>(std::nearbyint(coordsNew[2] / m_Params.zRes));

          if(m_SliceBySlice)
          {
            planeOld = k;
          }

          if(colOld >= 0 && colOld < m_Params.xp && rowOld >= 0 && rowOld < m_Params.yp && planeOld >= 0 && planeOld < m_Params.zp)
          {
            m_NewIndices[index] = (m_Params.xp * m_Params.yp * planeOld) + (m_Params.xp * rowOld) + colOld;
          }
        }
      }
    }
  }

  void operator()(const Range3D& range) const
  {
    convert(range[4], range[5], range[2], range[3], range[0], range[1]);
  }

private:
  nonstd::span<int64> m_NewIndices;
  float32 m_RotMatrixInv[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  bool m_SliceBySlice = false;
  RotateArgs m_Params;
};

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

// Requires table to be 3 x 3
Matrix3fR ConvertTableToMatrix(const std::vector<std::vector<float64>>& table)
{
  Matrix3fR matrix;

  for(usize i = 0; i < table.size(); i++)
  {
    const auto& row = table[i];
    for(usize j = 0; j < row.size(); j++)
    {
      matrix(i, j) = row[j];
    }
  }

  return matrix;
}

constexpr RotationRepresentation CastIndexToRotationRepresentation(uint64 index)
{
  switch(index)
  {
  case to_underlying(RotationRepresentation::AxisAngle): {
    return RotationRepresentation::AxisAngle;
  }
  case to_underlying(RotationRepresentation::RotationMatrix): {
    return RotationRepresentation::RotationMatrix;
  }
  default: {
    throw std::runtime_error(fmt::format("RotateSampleRefFrameFilter: Failed to cast index {} to RotationRepresentation", index));
  }
  }
}

Result<Matrix3fR> ConvertAxisAngleToRotationMatrix(const std::vector<float32>& rotationAxisVec, float32 rotationAngle)
{
  const Eigen::Vector3f rotationAxis(rotationAxisVec.data());
  float32 norm = rotationAxis.norm();
  std::vector<Warning> warnings;
  if(!closeEnough(norm, 1.0f, k_Threshold))
  {
    warnings.push_back(Warning{-45003, fmt::format("Axis angle is not normalized (norm is {}). Filter will automatically normalize the value.", norm)});
  }

  float32 rotationAngleRadians = rotationAngle * (numbers::pi / 180.0);

  Eigen::AngleAxisf axisAngle(rotationAngleRadians, rotationAxis.normalized());

  Matrix3fR rotationMatrix = axisAngle.toRotationMatrix();

  return {rotationMatrix, std::move(warnings)};
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

  Matrix3fR rotationMatrix = ConvertTableToMatrix(rotationMatrixTable);

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

Result<Matrix3fR> ComputeRotationMatrix(const Arguments& args)
{
  auto rotationRepresentationIndex = args.value<uint64>(RotateSampleRefFrameFilter::k_RotationRepresentation_Key);

  RotationRepresentation rotationRepresentation = CastIndexToRotationRepresentation(rotationRepresentationIndex);

  switch(rotationRepresentation)
  {
  case RotationRepresentation::AxisAngle: {
    auto rotationAxisVec = args.value<std::vector<float32>>(RotateSampleRefFrameFilter::k_RotationAxis_Key);
    auto rotationAngle = args.value<float32>(RotateSampleRefFrameFilter::k_RotationAngle_Key);

    return ConvertAxisAngleToRotationMatrix(rotationAxisVec, rotationAngle);
  }
  case RotationRepresentation::RotationMatrix: {
    auto rotationMatrixTable = args.value<DynamicTableParameter::ValueType>(RotateSampleRefFrameFilter::k_RotationMatrix_Key);

    return ConvertRotationTableToRotationMatrix(rotationMatrixTable);
  }
  default: {
    throw std::runtime_error("RotateSampleRefFrameFilter: Unsupported RotationRepresentation");
  }
  }
}

} // namespace

namespace complex
{
std::string RotateSampleRefFrameFilter::name() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::name;
}

std::string RotateSampleRefFrameFilter::className() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::className;
}

Uuid RotateSampleRefFrameFilter::uuid() const
{
  return FilterTraits<RotateSampleRefFrameFilter>::uuid;
}

std::string RotateSampleRefFrameFilter::humanName() const
{
  return "Rotate Sample Reference Frame";
}

Parameters RotateSampleRefFrameFilter::parameters() const
{
  Parameters params;

  params.insertSeparator({"Filter Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_RotateSliceBySlice_Key, "Perform Slice By Slice Transform", "This option is specific to EBSD Data and is not generally used.", false));

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_RotationRepresentation_Key, "Rotation Representation", "Which form used to represent rotation (axis angle or rotation matrix)",
                                                                    to_underlying(RotationRepresentation::AxisAngle), ChoicesParameter::Choices{"Axis Angle", "Rotation Matrix"}));
  params.insert(std::make_unique<Float32Parameter>(k_RotationAngle_Key, "Rotation Angle (Degrees)", "Magnitude of rotation (in degrees) about the rotation axis", 0.0f));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_RotationAxis_Key, "Rotation Axis (ijk)", "Axis in sample reference frame to rotate about", VectorFloat32Parameter::ValueType{0.0f, 0.0f, 0.0f}));
  DynamicTableInfo tableInfo;
  tableInfo.setColsInfo(DynamicTableInfo::StaticVectorInfo(3));
  tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(3));
  params.insert(std::make_unique<DynamicTableParameter>(k_RotationMatrix_Key, "Rotation Matrix", "Axis in sample reference frame to rotate about", tableInfo));
  params.linkParameters(k_RotationRepresentation_Key, k_RotationAngle_Key, std::make_any<uint64>(to_underlying(RotationRepresentation::AxisAngle)));
  params.linkParameters(k_RotationRepresentation_Key, k_RotationAxis_Key, std::make_any<uint64>(to_underlying(RotationRepresentation::AxisAngle)));
  params.linkParameters(k_RotationRepresentation_Key, k_RotationMatrix_Key, std::make_any<uint64>(to_underlying(RotationRepresentation::RotationMatrix)));

  params.insertSeparator({"Input Geometry and Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "The target geometry on which to perform the rotation", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator({"Output Geometry and Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometry_Key, "Created Image Geometry", "The location of the rotated geometry", DataPath{}));

  return params;
}

IFilter::UniquePointer RotateSampleRefFrameFilter::clone() const
{
  return std::make_unique<RotateSampleRefFrameFilter>();
}

IFilter::PreflightResult RotateSampleRefFrameFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler&, const std::atomic_bool&) const
{
  Result<Matrix3fR> matrixResult = ComputeRotationMatrix(filterArgs);

  if(matrixResult.invalid())
  {
    Result<OutputActions> result = {nonstd::make_unexpected(std::move(matrixResult.errors()))};
    result.warnings() = std::move(matrixResult.warnings());
    return {std::move(result)};
  }

  Matrix3fR rotationMatrix = matrixResult.value();
  auto selectedImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(selectedImageGeomPath);

  RotateArgs rotateArgs = CreateRotateArgs(selectedImageGeom, rotationMatrix);
  std::vector<usize> dims = {static_cast<usize>(rotateArgs.xpNew), static_cast<usize>(rotateArgs.ypNew), static_cast<usize>(rotateArgs.zpNew)};
  std::vector<float32> spacing = {rotateArgs.xResNew, rotateArgs.yResNew, rotateArgs.zResNew};
  auto origin = selectedImageGeom.getOrigin().toContainer<std::vector<float32>>();
  origin[0] += rotateArgs.xMinNew;
  origin[1] += rotateArgs.yMinNew;
  origin[2] += rotateArgs.zMinNew;

  OutputActions actions;
  Result<OutputActions> actionResults;

  auto createdImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);
  const auto* selectedCellData = selectedImageGeom.getCellData();
  actionResults.value().actions.push_back(std::make_unique<CreateImageGeometryAction>(createdImageGeomPath, dims, origin, spacing, selectedCellData->getName()));
  auto selectedCellDataChildren = GetAllChildArrayDataPaths(dataStructure, selectedCellData->getDataPaths()[0]);
  if(selectedCellDataChildren.has_value())
  {
    std::vector<usize> cellArrayDims(dims.crbegin(), dims.crend());
    usize totalDims = std::accumulate(cellArrayDims.begin(), cellArrayDims.end(), static_cast<usize>(1), std::multiplies<>{});
    auto selectedCellArrays = selectedCellDataChildren.value();
    for(const auto& cellArrayPath : selectedCellArrays)
    {
      std::string aPath = cellArrayPath.toString();
      aPath = complex::StringUtilities::replace(aPath, selectedImageGeomPath.toString(), createdImageGeomPath.toString());
      DataPath createdArrayPath = DataPath::FromString(aPath).value();

      if(const auto* cellArray = dataStructure.getDataAs<IDataArray>(cellArrayPath); cellArray != nullptr)
      {
        actionResults.value().actions.push_back(std::make_unique<CreateArrayAction>(cellArray->getDataType(), cellArrayDims, cellArray->getIDataStoreRef().getComponentShape(), createdArrayPath));
      }
      else if(const auto* neighborArray = dataStructure.getDataAs<INeighborList>(cellArrayPath); cellArray != nullptr)
      {
        // TODO : actionResults.value().actions.push_back(std::make_unique<CreateNeighborListAction>(neighborArray->getDataType(), totalDims, createdArrayPath));
        actionResults.warnings().push_back(Warning{-88970, fmt::format("Skipping NeighborList array at path '{}'. Support for NeighborList arrays in the cell data Attribute Matrix during "
                                                                       "RotateSampleRefFrame has not yet been implemented. Please contact the developer.",
                                                                       cellArrayPath.toString())});
      }
      else
      {
        // TODO : StringArray --> we don't have an action for this yet
        actionResults.warnings().push_back(Warning{-88970, fmt::format("Skipping StringArray at path '{}'. Support for StringArrays in the cell data Attribute Matrix during "
                                                                       "RotateSampleRefFrame has not yet been implemented. Please contact the developer.",
                                                                       cellArrayPath.toString())});
      }
    }
  }

  return {std::move(actionResults)};
}

Result<> RotateSampleRefFrameFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  auto selectedImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(selectedImageGeomPath);
  auto sliceBySlice = filterArgs.value<bool>(k_RotateSliceBySlice_Key);

  Result<Matrix3fR> matrixResult = ComputeRotationMatrix(filterArgs);

  Matrix3fR rotationMatrix = matrixResult.value();

  RotateArgs rotateArgs = CreateRotateArgs(selectedImageGeom, rotationMatrix);

  int64 newNumCellTuples = rotateArgs.xpNew * rotateArgs.ypNew * rotateArgs.zpNew;

  // Compute the mapping of old indices from the original geometry to the new indices in the new geometry.
  std::vector<int64> newIndices(newNumCellTuples, -1);

  ParallelData3DAlgorithm parallelAlgorithm;
  parallelAlgorithm.setRange(Range3D(0, rotateArgs.xpNew, 0, rotateArgs.ypNew, 0, rotateArgs.zpNew));
  parallelAlgorithm.setParallelizationEnabled(true);
  parallelAlgorithm.execute(SampleRefFrameRotator(newIndices, rotateArgs, rotationMatrix, sliceBySlice));

  auto selectedCellDataChildren = GetAllChildArrayDataPaths(dataStructure, selectedImageGeom.getCellDataPath());
  auto selectedCellArrays = selectedCellDataChildren.has_value() ? selectedCellDataChildren.value() : std::vector<DataPath>{};

  auto createdImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometry_Key);

#ifdef COMPLEX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  // C++11 RIGHT HERE....
  int32_t nthreads = static_cast<int32_t>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  int32_t threadCount = 0;
#endif

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(shouldCancel)
    {
      return {};
    }

    messageHandler(fmt::format("Rotating {}", cellArrayPath.toString()));
    std::string aPath = cellArrayPath.toString();
    aPath = complex::StringUtilities::replace(aPath, selectedImageGeomPath.toString(), createdImageGeomPath.toString());
    DataPath createdArrayPath = DataPath::FromString(aPath).value();
    if(dataStructure.getDataAs<IDataArray>(cellArrayPath) != nullptr)
    {
      const auto& oldCellArray = dataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      auto& newCellArray = dataStructure.getDataRefAs<IDataArray>(createdArrayPath);
      DataType type = oldCellArray.getDataType();

      switch(type)
      {
      case DataType::boolean: {
        RUN_TASK(CopyTupleUsingIndexList<bool>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::int8: {
        RUN_TASK(CopyTupleUsingIndexList<int8>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::int16: {
        RUN_TASK(CopyTupleUsingIndexList<int16>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::int32: {
        RUN_TASK(CopyTupleUsingIndexList<int32>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::int64: {
        RUN_TASK(CopyTupleUsingIndexList<int64>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::uint8: {
        RUN_TASK(CopyTupleUsingIndexList<uint8>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::uint16: {
        RUN_TASK(CopyTupleUsingIndexList<uint16>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::uint32: {
        RUN_TASK(CopyTupleUsingIndexList<uint32>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::uint64: {
        RUN_TASK(CopyTupleUsingIndexList<uint64>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::float32: {
        RUN_TASK(CopyTupleUsingIndexList<float32>(oldCellArray, newCellArray, newIndices));
        break;
      }
      case DataType::float64: {
        RUN_TASK(CopyTupleUsingIndexList<float64>(oldCellArray, newCellArray, newIndices));
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
    }
    else
    {
      // TODO: NeighborList arrays or StringArrays
      continue;
    }
#ifdef COMPLEX_ENABLE_MULTICORE
    threadCount++;
    if(threadCount == nthreads)
    {
      g->wait();
      threadCount = 0;
    }
#else

#endif
  }

#ifdef COMPLEX_ENABLE_MULTICORE
  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  g->wait();
#endif

  return {};
}
} // namespace complex
