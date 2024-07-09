#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <Eigen/Dense>

#include <chrono>
#include <concepts>
#include <fstream>
#include <iostream>
#include <mutex>

namespace nx::core::ImageRotationUtilities
{
const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

using Matrix3fR = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

using Vector3i64 = Eigen::Array<int64_t, 1, 3>;

struct RotateArgs
{

  USizeVec3 OriginalDims;
  FloatVec3 OriginalSpacing;
  FloatVec3 OriginalOrigin;
  int64_t xp = 0;
  int64_t yp = 0;
  int64_t zp = 0;
  float xRes = 0.0f;
  float yRes = 0.0f;
  float zRes = 0.0f;

  USizeVec3 TransformedDims;
  FloatVec3 TransformedSpacing;
  FloatVec3 TransformedOrigin;
  int64_t xpNew = 0;
  int64_t ypNew = 0;
  int64_t zpNew = 0;
  float xResNew = 0.0f;
  float yResNew = 0.0f;
  float zResNew = 0.0f;
  float xMinNew = 0.0f;
  float yMinNew = 0.0f;
  float zMinNew = 0.0f;
};

/**
 * @brief
 * @param transform
 * @return
 */
SIMPLNX_EXPORT std::string GenerateTransformationMatrixDescription(const ImageRotationUtilities::Matrix4fR& transform);

/**
 * @brief
 * @param precomputed
 * @return
 */
SIMPLNX_EXPORT Matrix4fR CopyPrecomputedToTransformationMatrix(const Float32Array& precomputed);

/**
 * @brief
 * @param tableData
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData);

/**
 * @brief
 * @param pRotationValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue);

/**
 * @brief
 * @param pTranslationValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue);
/**
 * @brief
 * @param pScaleValue
 * @return
 */
SIMPLNX_EXPORT Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue);

/**
 * @brief Function to determine the min and max coordinates (bounding box) of the transformed Image Geometry.
 * @param imageGeometry
 * @param transformationMatrix
 * @return
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix);

/**
 * @brief Finds the Cosine of the angle between 2 vectors
 * @tparam T
 * @param vectorA
 * @param vectorB
 * @return
 */
template <std::floating_point T>
T CosBetweenVectors(const Eigen::Vector3<T>& vectorA, const Eigen::Vector3<T>& vectorB)
{
  const T normA = vectorA.norm();
  const T normB = vectorB.norm();

  if(normA == static_cast<T>(0.0) || normB == static_cast<T>(0.0))
  {
    return static_cast<T>(1.0);
  }

  return vectorA.dot(vectorB) / (normA * normB);
}

/**
 * @brief Function for determining new ImageGeom Spacing between points for scaling
 * @param spacing
 * @param axisNew
 * @return spacing for a given axis.
 */
SIMPLNX_EXPORT float DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew);

/**
 * @brief Determines parameters for image rotation
 * @param imageGeom
 * @param transformationMatrix
 * @return New RotateArgs object
 */
SIMPLNX_EXPORT ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix);

/**
 * @brief
 * @tparam T
 * @param params
 * @param xyzIndex
 * @param sourceArray
 * @param compIndex
 * @return
 */
template <typename T>
T inline GetSourceArrayValue(const RotateArgs& params, Vector3i64 xyzIndex, const DataArray<T>& sourceArray, size_t compIndex)
{
  if(xyzIndex[0] < 0)
  {
    xyzIndex[0] = 0;
  }
  if(xyzIndex[0] >= params.xp)
  {
    xyzIndex[0] = params.xp - 1;
  }

  if(xyzIndex[1] < 0)
  {
    xyzIndex[1] = 0;
  }
  if(xyzIndex[1] >= params.yp)
  {
    xyzIndex[1] = params.yp - 1;
  }

  if(xyzIndex[2] < 0)
  {
    xyzIndex[2] = 0;
  }
  if(xyzIndex[2] >= params.zp)
  {
    xyzIndex[2] = params.zp - 1;
  }

  // Now just compute the proper index
  const usize index = (xyzIndex[2] * params.xp * params.yp) + (xyzIndex[1] * params.xp) + xyzIndex[0];
  return sourceArray[index * sourceArray.getNumberOfComponents() + compIndex];
}

/**
 * @brief FindOctant
 * @param params
 * @param index
 * @param coord
 * @return
 */
SIMPLNX_EXPORT size_t FindOctant(const RotateArgs& params, const Point3Df& centerPoint, const Eigen::Array4f& coord);

using OctantOffsetArrayType = std::array<Vector3i64, 8>;

static const OctantOffsetArrayType k_IndexOffset0 = {Vector3i64{-1, -1, -1}, Vector3i64{0, -1, -1}, Vector3i64{0, 0, -1}, Vector3i64{-1, 0, -1},
                                                     Vector3i64{-1, -1, 0},  Vector3i64{0, -1, 0},  Vector3i64{0, 0, 0},  Vector3i64{-1, 0, 0}};
static const OctantOffsetArrayType k_IndexOffset1 = {Vector3i64{0, -1, -1}, Vector3i64{1, -1, -1}, Vector3i64{1, 0, -1}, Vector3i64{0, 0, -1},
                                                     Vector3i64{0, -1, 0},  Vector3i64{1, -1, 0},  Vector3i64{1, 0, 0},  Vector3i64{0, 0, 0}};
static const OctantOffsetArrayType k_IndexOffset2 = {Vector3i64{0, 0, -1}, Vector3i64{1, 0, -1}, Vector3i64{1, 1, -1}, Vector3i64{0, 1, -1},
                                                     Vector3i64{0, 0, 0},  Vector3i64{1, 0, 0},  Vector3i64{1, 1, 0},  Vector3i64{0, 1, 0}};
static const OctantOffsetArrayType k_IndexOffset3 = {Vector3i64{-1, 0, -1}, Vector3i64{0, 0, -1}, Vector3i64{0, 1, -1}, Vector3i64{-1, 1, -1},
                                                     Vector3i64{-1, 0, 0},  Vector3i64{0, 0, 0},  Vector3i64{0, 1, 0},  Vector3i64{-1, 1, 0}};
static const OctantOffsetArrayType k_IndexOffset4 = {Vector3i64{-1, -1, 0}, Vector3i64{0, -1, 0}, Vector3i64{0, 0, 0}, Vector3i64{-1, 0, 0},
                                                     Vector3i64{-1, -1, 1}, Vector3i64{0, -1, 1}, Vector3i64{0, 0, 1}, Vector3i64{-1, 0, 1}};
static const OctantOffsetArrayType k_IndexOffset5 = {Vector3i64{0, -1, 0}, Vector3i64{1, -1, 0}, Vector3i64{1, 0, 0}, Vector3i64{0, 0, 0},
                                                     Vector3i64{0, -1, 1}, Vector3i64{1, -1, 1}, Vector3i64{1, 0, 1}, Vector3i64{0, 0, 1}};
static const OctantOffsetArrayType k_IndexOffset6 = {Vector3i64{0, 0, 0}, Vector3i64{1, 0, 0}, Vector3i64{1, 1, 0}, Vector3i64{0, 1, 0},
                                                     Vector3i64{0, 0, 1}, Vector3i64{1, 0, 1}, Vector3i64{1, 1, 1}, Vector3i64{0, 1, 1}};
static const OctantOffsetArrayType k_IndexOffset7 = {Vector3i64{-1, 0, 0}, Vector3i64{0, 0, 0}, Vector3i64{0, 1, 0}, Vector3i64{-1, -1, 0},
                                                     Vector3i64{-1, 0, 1}, Vector3i64{0, 0, 1}, Vector3i64{0, 1, 1}, Vector3i64{-1, -1, 1}};
static const std::array<OctantOffsetArrayType, 8> k_AllOctantOffsets{k_IndexOffset0, k_IndexOffset1, k_IndexOffset2, k_IndexOffset3, k_IndexOffset4, k_IndexOffset5, k_IndexOffset6, k_IndexOffset7};

/**
 * @brief FindInterpolationValues
 * @param params
 * @param index
 * @param octant
 * @param oldIndicesU
 * @param oldCoords
 * @param sourceArray
 * @param pValues
 * @param uvw
 */
template <typename T>
inline void FindInterpolationValues(const RotateArgs& params, size_t octant, SizeVec3 oldIndicesU, Eigen::Array4f& oldCoords, const DataArray<T>& sourceArray, std::vector<T>& pValues,
                                    Eigen::Vector3f& uvw)
{
  const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];

  const Vector3i64 oldIndices(static_cast<int64_t>(oldIndicesU[0]), static_cast<int64_t>(oldIndicesU[1]), static_cast<int64_t>(oldIndicesU[2]));
  size_t numComps = sourceArray.getNumberOfComponents();

  Eigen::Vector3f p1Coord;

  for(size_t i = 0; i < 8; i++)
  {
    auto pIndices = oldIndices + indexOffset[i];
    for(size_t compIndex = 0; compIndex < numComps; compIndex++)
    {
      T value = GetSourceArrayValue<T>(params, pIndices, sourceArray, compIndex);
      pValues[i * numComps + compIndex] = value;
    }
    if(i == 0)
    {
      p1Coord = {static_cast<float32>(pIndices[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                 static_cast<float32>(pIndices[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                 static_cast<float32>(pIndices[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};
    }
  }
  uvw[0] = oldCoords[0] - p1Coord[0];
  uvw[1] = oldCoords[1] - p1Coord[1];
  uvw[2] = oldCoords[2] - p1Coord[2];
}

/**
 * @brief
 */
class FilterProgressCallback
{
public:
  FilterProgressCallback(const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel)
  : m_MessageHandler(mesgHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void sendThreadSafeProgressMessage(int64_t counter)
  {
    static std::mutex mutex;
    m_Progcounter += static_cast<int32>(counter);
    const std::lock_guard<std::mutex> lock(mutex);
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000)
    {
      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, fmt::format("Nodes Completed: {}", m_Progcounter), m_Progcounter});
      m_InitialTime = std::chrono::steady_clock::now();
    }
  }

  void sendThreadSafeProgressMessage(const std::string& progressMessage)
  {
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000)
    {
      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, progressMessage});
      m_InitialTime = std::chrono::steady_clock::now();
    }
  }

  const std::atomic_bool& getCancel()
  {
    return m_ShouldCancel;
  }

private:
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  int32 m_Progcounter = 0;
};

/**
 * @brief The RotateImageGeometryWithTrilinearInterpolation class
 */
template <typename T>
class RotateImageGeometryWithTrilinearInterpolation
{
public:
  RotateImageGeometryWithTrilinearInterpolation(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& rotateArgs, const Matrix4fR& transformationMatrix,
                                                FilterProgressCallback* filterCallback)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(rotateArgs)
  , m_TransformationMatrix(transformationMatrix)
  , m_FilterCallback(filterCallback)
  {
  }
  ~RotateImageGeometryWithTrilinearInterpolation() = default;

  RotateImageGeometryWithTrilinearInterpolation(const RotateImageGeometryWithTrilinearInterpolation&) = default;
  RotateImageGeometryWithTrilinearInterpolation(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = default;
  RotateImageGeometryWithTrilinearInterpolation& operator=(const RotateImageGeometryWithTrilinearInterpolation&) = delete;
  RotateImageGeometryWithTrilinearInterpolation& operator=(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = delete;
  /**
   * @brief calculateInterpolatedValue
   *
   * This comes from https://www.cs.purdue.edu/homes/cs530/slides/04.DataStructure.pdf, page 36.
   *
   * Note in the codes below the equations have been changed to do all of the additions first, then
   * the subtractions. This should hopefully alleviate issue with trying to subtract unsigned integers
   * and ending up with what should have been a negative number but since it is unsigned the value
   * that the compiler will compute would be vastly different.
   *
   * @param sourceArray
   * @param oldIndex
   * @param indices
   * @return
   */
  T calculateInterpolatedValue(std::vector<T>& pValues, Eigen::Vector3f& uvw, size_t numComps, size_t compIndex) const
  {
    constexpr size_t P1 = 0;
    constexpr size_t P2 = 1;
    constexpr size_t P3 = 2;
    constexpr size_t P4 = 3;
    constexpr size_t P5 = 4;
    constexpr size_t P6 = 5;
    constexpr size_t P7 = 6;
    constexpr size_t P8 = 7;

    const float u = uvw[0];
    const float v = uvw[1];
    const float w = uvw[2];

    T value = pValues[0];
    // clang-format off
    value += u * (pValues[P2 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += v * (pValues[P4 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += w * (pValues[P5 * numComps + compIndex] - pValues[P1 * numComps + compIndex]);
    value += u * v * (pValues[P1 * numComps + compIndex] + pValues[P3 * numComps + compIndex] - pValues[P2 * numComps + compIndex] - pValues[P4 * numComps + compIndex]);
    value += u * w * (pValues[P1 * numComps + compIndex] + pValues[P6 * numComps + compIndex] - pValues[P2 * numComps + compIndex] - pValues[P5 * numComps + compIndex]);
    value += v * w * (pValues[P1 * numComps + compIndex] + pValues[P8 * numComps + compIndex] - pValues[P4 * numComps + compIndex] - pValues[P5 * numComps + compIndex]);
    value += u * v * w *
             ( pValues[P4 * numComps + compIndex]
              + pValues[P2 * numComps + compIndex]
              + pValues[P8 * numComps + compIndex]
              + pValues[P6 * numComps + compIndex]
              - pValues[P1 * numComps + compIndex]
              - pValues[P3 * numComps + compIndex]
              - pValues[P5 * numComps + compIndex]
              - pValues[P7 * numComps + compIndex] );

    // clang-format on
    return value;
  }

  /**
   * @brief This is the main algorithm to perform the interpolation and get a final value that is placed into the transformed
   * voxel. This uses Trilinear interpolation which will devolve into Bilinear and Linear interpolation depending on the
   * values of U, V and W.
   */
  void operator()() const
  {
    using DataArrayType = DataArray<T>;

    const DataArrayType& sourceArray = dynamic_cast<const DataArrayType&>(*m_SourceArray);
    const size_t numComps = sourceArray.getNumberOfComponents();
    if(numComps == 0)
    {
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Number of Components was Zero for array. Exiting Transform.", sourceArray.getName()));
      return;
    }

    m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Transform Starting", sourceArray.getName()));

    auto& newDataStore = m_TargetArray->getIDataStoreRefAs<AbstractDataStore<T>>();

    DataStructure tempDataStructure;
    ImageGeom* origImageGeomPtr = ImageGeom::Create(tempDataStructure, "Temp");
    origImageGeomPtr->setDimensions(m_Params.OriginalDims);
    origImageGeomPtr->setSpacing(m_Params.OriginalSpacing);
    origImageGeomPtr->setOrigin(m_Params.OriginalOrigin);

    ImageGeom* destImageGeomPtr = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeomPtr->setDimensions(m_Params.TransformedDims);
    destImageGeomPtr->setSpacing(m_Params.TransformedSpacing);
    destImageGeomPtr->setOrigin(m_Params.TransformedOrigin);

    // Tri linearInterpolationData<T> interpolationValues;
    // SizeVec3 oldGeomIndices = {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};

    std::vector<T> pValues(8 * numComps);
    Eigen::Vector3f uvw;

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    for(int64_t k = 0; k < m_Params.zpNew; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.zpNew));
      int64_t ktot = (m_Params.xpNew * m_Params.ypNew) * k;

      for(int64_t j = 0; j < m_Params.ypNew; j++)
      {
        int64_t jtot = (m_Params.xpNew) * j;
        for(int64_t i = 0; i < m_Params.xpNew; i++)
        {
          int64_t newIndex = ktot + jtot + i;

          Point3Df point = destImageGeomPtr->getCoordsf(newIndex);
          // Last value is 1. See https://www.euclideanspace.com/maths/geometry/affine/matrix4x4/index.htm
          Eigen::Vector4f coordsNew(point.getX(), point.getY(), point.getZ(), 1.0f);
          // Transform back to the old coordinate
          Eigen::Array4f coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = origImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            size_t oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * m_Params.OriginalDims[1]) + oldGeomIndices[0];
            auto centerPoint = origImageGeomPtr->getCoordsf(oldIndex);

            int octant = FindOctant(m_Params, centerPoint, coordsOld);

            FindInterpolationValues(m_Params, octant, oldGeomIndices, coordsOld, sourceArray, pValues, uvw);
            for(size_t compIndex = 0; compIndex < numComps; compIndex++)
            {
              T value = calculateInterpolatedValue(pValues, uvw, numComps, compIndex);
              newDataStore.setComponent(newIndex, compIndex, value);
            }
          }
          else
          {
            newDataStore.fillTuple(newIndex, static_cast<T>(0));
          }
        }
      }
    }
    m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Transform Ending", sourceArray.getName()));
  }

private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  Matrix4fR m_TransformationMatrix;
  FilterProgressCallback* m_FilterCallback = nullptr;
};

//------------------------------------------------------------------------------
template <typename T>
class RotateImageGeometryWithNearestNeighbor
{
public:
  RotateImageGeometryWithNearestNeighbor(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& args, const Matrix4fR& transformationMatrix, bool sliceBySlice,
                                         FilterProgressCallback* filterCallback)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(args)
  , m_TransformationMatrix(transformationMatrix)
  , m_SliceBySlice(sliceBySlice)
  , m_FilterCallback(filterCallback)
  {
  }
  ~RotateImageGeometryWithNearestNeighbor() = default;

  RotateImageGeometryWithNearestNeighbor(const RotateImageGeometryWithNearestNeighbor&) = default;
  RotateImageGeometryWithNearestNeighbor(RotateImageGeometryWithNearestNeighbor&&) noexcept = default;
  RotateImageGeometryWithNearestNeighbor& operator=(const RotateImageGeometryWithNearestNeighbor&) = delete;
  RotateImageGeometryWithNearestNeighbor& operator=(RotateImageGeometryWithNearestNeighbor&&) noexcept = delete;

  void convert() const
  {

    DataStructure tempDataStructure;
    ImageGeom* srcImageGeomPtr = ImageGeom::Create(tempDataStructure, "source image geom");
    srcImageGeomPtr->setDimensions(m_Params.OriginalDims);
    srcImageGeomPtr->setSpacing(m_Params.OriginalSpacing);
    srcImageGeomPtr->setOrigin(m_Params.OriginalOrigin);

    ImageGeom* destImageGeomPtr = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeomPtr->setDimensions(m_Params.TransformedDims);
    destImageGeomPtr->setSpacing(m_Params.TransformedSpacing);
    destImageGeomPtr->setOrigin(m_Params.TransformedOrigin);

    const auto& oldDataStore = m_SourceArray->getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_TargetArray->getIDataStoreRefAs<AbstractDataStore<T>>();

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();
    for(int64 k = 0; k < m_Params.zpNew; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.zpNew));

      int64 const ktot = (m_Params.xpNew * m_Params.ypNew) * k;
      for(int64 j = 0; j < m_Params.ypNew; j++)
      {
        int64 jtot = (m_Params.xpNew) * j;
        for(int64 i = 0; i < m_Params.xpNew; i++)
        {
          int64 const newIndex = ktot + jtot + i;
          Point3Df point = destImageGeomPtr->getCoordsf(newIndex);
          // Last value is 1. See https://www.euclideanspace.com/maths/geometry/affine/matrix4x4/index.htm
          Eigen::Vector4f coordsNew(point.getX(), point.getY(), point.getZ(), 1.0f);
          // Transform back to the old coordinate
          Eigen::Array4f coordsOld = inverseTransform * coordsNew;

          // Now compute the old Cell Index from the old coordinate
          SizeVec3 oldGeomIndices;
          auto errorResult = srcImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

          // Now we know what voxel the new cell center maps back to in the original geometry.
          if(errorResult == ImageGeom::ErrorType::NoError)
          {
            if(m_SliceBySlice)
            {
              oldGeomIndices[2] = k;
            }
            size_t oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];

            if(newDataStore.copyFrom(newIndex, oldDataStore, oldIndex, 1).invalid())
            {
              std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_SourceArray->getName(), oldIndex,
                                       m_SourceArray->getName(), newIndex)
                        << std::endl;
              break;
            }
          }
          else
          {
            newDataStore.fillTuple(newIndex, 0);
          }
        }
      }
    }
    m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Transform Ending", m_SourceArray->getName()));
  }

  void operator()() const
  {
    convert();
  }

private:
  const IDataArray* m_SourceArray;
  IDataArray* m_TargetArray;
  ImageRotationUtilities::RotateArgs m_Params;
  const Matrix4fR& m_TransformationMatrix;
  bool m_SliceBySlice = false;
  FilterProgressCallback* m_FilterCallback = nullptr;
};

/**
 * @brief The ApplyTransformationToNodeGeometry class will apply a transformation to a node based geometry.
 */
class ApplyTransformationToNodeGeometry
{

public:
  ApplyTransformationToNodeGeometry(IGeometry::SharedVertexList& verticesPtr, const Matrix4fR& transformationMatrix, FilterProgressCallback* filterCallback)
  : m_Vertices(verticesPtr)
  , m_TransformationMatrix(transformationMatrix)
  , m_FilterCallback(filterCallback)
  {
  }

  void convert(size_t start, size_t end) const
  {
    int64_t progCounter = 0;
    const size_t totalElements = (end - start);
    const size_t progIncrement = static_cast<int64_t>(totalElements / 100);

    for(size_t i = start; i < end; i++)
    {
      if(m_FilterCallback->getCancel())
      {
        return;
      }
      const Eigen::Vector4f position(m_Vertices[3 * i + 0], m_Vertices[3 * i + 1], m_Vertices[3 * i + 2], 1);
      Eigen::Vector4f transformedPosition = m_TransformationMatrix * position;
      m_Vertices[3 * i + 0] = transformedPosition[0];
      m_Vertices[3 * i + 1] = transformedPosition[1];
      m_Vertices[3 * i + 2] = transformedPosition[2];

      if(progCounter > progIncrement)
      {
        m_FilterCallback->sendThreadSafeProgressMessage(progCounter);
        progCounter = 0;
      }
      progCounter++;
    }
  }

  /**
   * @brief operator () This is called from the TBB stye of code
   * @param range The range to compute the values
   */
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  const Matrix4fR& m_TransformationMatrix;
  IGeometry::SharedVertexList& m_Vertices;
  FilterProgressCallback* m_FilterCallback = nullptr;
};

} // namespace nx::core::ImageRotationUtilities
