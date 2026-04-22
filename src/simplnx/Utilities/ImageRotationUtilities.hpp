#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/simplnx_export.hpp"

#include <Eigen/Dense>

#include <chrono>
#include <concepts>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

namespace nx::core::ImageRotationUtilities
{
const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

using Matrix3fR = Eigen::Matrix<float32, 3, 3, Eigen::RowMajor>;
using Matrix4fR = Eigen::Matrix<float32, 4, 4, Eigen::RowMajor>;

using Vector3i64 = Eigen::Array<int64, 1, 3>;

struct RotateArgs
{
  USizeVec3 OriginalDims;
  FloatVec3 OriginalSpacing;
  FloatVec3 OriginalOrigin;
  int64 xp = 0;
  int64 yp = 0;
  int64 zp = 0;
  float32 xRes = 0.0f;
  float32 yRes = 0.0f;
  float32 zRes = 0.0f;

  USizeVec3 TransformedDims;
  FloatVec3 TransformedSpacing;
  FloatVec3 TransformedOrigin;

  USizeVec3 outputDims;
  FloatVec3 outputSpacing;

  float32 outputXMin = 0.0f;
  float32 outputYMin = 0.0f;
  float32 outputZMin = 0.0f;
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
SIMPLNX_EXPORT Matrix4fR CopyPrecomputedToTransformationMatrix(const AbstractDataStore<float32>& precomputed);

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
 * @brief Function to determine the min and max coordinates of the transformed Image Geometry using the bounding box.
 * @param imageGeomBoundingBox
 * @param transformationMatrix
 * @return
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const BoundingBox3Df& imageGeomBoundingBox, const Matrix4fR& transformationMatrix);

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
SIMPLNX_EXPORT float32 DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew);

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
T inline GetSourceArrayValue(const RotateArgs& params, Vector3i64 xyzIndex, const DataArray<T>& sourceArray, usize compIndex)
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
 * @brief
 * @param params
 * @param centerPoint
 * @param coord
 * @return
 */
SIMPLNX_EXPORT usize FindOctant(const RotateArgs& params, const Point3Df& centerPoint, const Eigen::Array4f& coord);

using OctantOffsetArrayType = std::array<Vector3i64, 8>;

/* clang-format off */

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

/* clang-format on */

template <class T>
using AccumulationValueType = std::conditional_t<std::is_floating_point_v<T>, float64, int64>;

/**
 * @brief FindInterpolationValues
 * @tparam T
 * @param params
 * @param octant
 * @param oldIndicesU
 * @param oldCoords
 * @param sourceArray
 * @param pValues
 * @param uvw
 * @param hitVoxelCenterPoint
 */
template <typename T>
inline void FindInterpolationValues(const RotateArgs& params, usize octant, SizeVec3 oldIndicesU, Eigen::Array4f& oldCoords, const DataArray<T>& sourceArray,
                                    std::vector<AccumulationValueType<T>>& pValues, Eigen::Vector3f& uvw, Point3Df& hitVoxelCenterPoint)
{
  const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];

  const Vector3i64 oldIndices(static_cast<int64>(oldIndicesU[0]), static_cast<int64>(oldIndicesU[1]), static_cast<int64>(oldIndicesU[2]));
  usize numComps = sourceArray.getNumberOfComponents();

  Eigen::Vector3f p1Coord;

  for(usize i = 0; i < 8; i++)
  {
    auto pIndices = oldIndices + indexOffset[i];
    for(usize compIndex = 0; compIndex < numComps; compIndex++)
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

  // NEED TO CALCULATE NEW UVW VALUES BASED ON coordsOld (which is the actual xyz point coord that we need to interpolate).
  auto c000_Index = oldIndices + indexOffset[0];
  auto c111_Index = oldIndices + indexOffset[6];
  Eigen::Vector3f c000_Coord = {static_cast<float32>(c000_Index[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                                static_cast<float32>(c000_Index[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                                static_cast<float32>(c000_Index[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};
  Eigen::Vector3f c111_Coord = {static_cast<float32>(c111_Index[0]) * params.xRes + (0.5F * params.xRes) + params.OriginalOrigin[0],
                                static_cast<float32>(c111_Index[1]) * params.yRes + (0.5F * params.yRes) + params.OriginalOrigin[1],
                                static_cast<float32>(c111_Index[2]) * params.zRes + (0.5F * params.zRes) + params.OriginalOrigin[2]};

  for(usize i = 0; i < 3; i++)
  {
    uvw[i] = (oldCoords[i] - c000_Coord[i]) / (c111_Coord[i] - c000_Coord[i]);
    uvw[i] = uvw[i] < 0.0 ? 0.0 : uvw[i];
    uvw[i] = uvw[i] > 1.0 ? 1.0 : uvw[i];
  }
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

  void sendThreadSafeProgressMessage(int64 counter)
  {
    static std::mutex mutex;
    m_Progcounter += static_cast<int32>(counter);
    const std::lock_guard<std::mutex> lock(mutex);
    auto now = std::chrono::steady_clock::now();
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000)
    {
      m_MessageHandler(IFilter::Message{IFilter::Message::Type::Info, fmt::format("Nodes Completed: {}", m_Progcounter)});
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

  const std::atomic_bool& getCancel() const
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
 * @brief Update a Z-slice slab cache to cover [newZMin, newZMax].
 *
 * The slab cache holds a contiguous range of source Z-slices in a pre-allocated buffer.
 * When the caller asks for a new range that overlaps the cached range, this helper shifts
 * the surviving slices to their new position via memmove and issues bulk reads only for
 * the delta slices (below or above the overlap). When there is no overlap (or the buffer
 * had to grow), the entire new range is re-read.
 *
 * The caller owns \a slabBuf and \a slabBufSize; this function may grow the buffer but
 * will not shrink it. \a cachedZMin and \a cachedZMax are updated in place.
 *
 * Preconditions: \a newZMin <= \a newZMax, both within the source dataset's Z range.
 */
template <typename T>
inline void updateSlabCache(const AbstractDataStore<T>& srcStore, std::unique_ptr<T[]>& slabBuf, usize& slabBufSize, int64& cachedZMin, int64& cachedZMax, int64 newZMin, int64 newZMax,
                            usize sliceTuples, usize numComps)
{
  const usize sliceElems = sliceTuples * numComps;
  const usize needElems = static_cast<usize>(newZMax - newZMin + 1) * sliceElems;

  bool validCache = (cachedZMin >= 0 && cachedZMax >= cachedZMin);

  // Grow buffer if needed. Growth discards the old contents, so the cache must be re-read in full.
  if(needElems > slabBufSize)
  {
    slabBuf = std::make_unique<T[]>(needElems);
    slabBufSize = needElems;
    validCache = false;
  }

  const int64 overlapMin = std::max(newZMin, cachedZMin);
  const int64 overlapMax = std::min(newZMax, cachedZMax);
  const bool hasOverlap = validCache && overlapMin <= overlapMax;

  if(hasOverlap)
  {
    // Shift the surviving slices to their new position (memmove handles overlap in either direction).
    const usize srcOff = static_cast<usize>(overlapMin - cachedZMin) * sliceElems;
    const usize dstOff = static_cast<usize>(overlapMin - newZMin) * sliceElems;
    const usize moveCount = static_cast<usize>(overlapMax - overlapMin + 1) * sliceElems;
    if(srcOff != dstOff)
    {
      std::memmove(slabBuf.get() + dstOff, slabBuf.get() + srcOff, moveCount * sizeof(T));
    }
    // Read slices below the overlap (the new range extends further back).
    if(newZMin < overlapMin)
    {
      const usize readElems = static_cast<usize>(overlapMin - newZMin) * sliceElems;
      srcStore.copyIntoBuffer(static_cast<usize>(newZMin) * sliceElems, nonstd::span<T>(slabBuf.get(), readElems));
    }
    // Read slices above the overlap (the new range extends further forward — typical case).
    if(newZMax > overlapMax)
    {
      const usize readElems = static_cast<usize>(newZMax - overlapMax) * sliceElems;
      const usize readStartZ = static_cast<usize>(overlapMax + 1);
      const usize dstReadOff = static_cast<usize>(overlapMax + 1 - newZMin) * sliceElems;
      srcStore.copyIntoBuffer(readStartZ * sliceElems, nonstd::span<T>(slabBuf.get() + dstReadOff, readElems));
    }
  }
  else
  {
    srcStore.copyIntoBuffer(static_cast<usize>(newZMin) * sliceElems, nonstd::span<T>(slabBuf.get(), needElems));
  }

  cachedZMin = newZMin;
  cachedZMax = newZMax;
}

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
   * This comes from https://en.wikipedia.org/wiki/Trilinear_interpolation
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
  T calculateInterpolatedValue(const std::vector<AccumulationValueType<T>>& pValues, const Eigen::Vector3f& uvw, usize numComps, usize compIndex) const
  {
    constexpr usize P1 = 0;
    constexpr usize P2 = 1;
    constexpr usize P3 = 2;
    constexpr usize P4 = 3;
    constexpr usize P5 = 4;
    constexpr usize P6 = 5;
    constexpr usize P7 = 6;
    constexpr usize P8 = 7;

    /* clang-format on */
    const AccumulationValueType<T> c000 = pValues[P1 * numComps + compIndex];
    const AccumulationValueType<T> c100 = pValues[P2 * numComps + compIndex];
    const AccumulationValueType<T> c110 = pValues[P3 * numComps + compIndex];
    const AccumulationValueType<T> c010 = pValues[P4 * numComps + compIndex];
    const AccumulationValueType<T> c001 = pValues[P5 * numComps + compIndex];
    const AccumulationValueType<T> c101 = pValues[P6 * numComps + compIndex];
    const AccumulationValueType<T> c111 = pValues[P7 * numComps + compIndex];
    const AccumulationValueType<T> c011 = pValues[P8 * numComps + compIndex];

    const float32 Xd = uvw[0];
    const float32 Yd = uvw[1];
    const float32 Zd = uvw[2];

    const AccumulationValueType<T> c00 = c000 * (1 - Xd) + c100 * Xd;
    const AccumulationValueType<T> c01 = c001 * (1 - Xd) + c101 * Xd;
    const AccumulationValueType<T> c10 = c010 * (1 - Xd) + c110 * Xd;
    const AccumulationValueType<T> c11 = c011 * (1 - Xd) + c111 * Xd;

    const AccumulationValueType<T> c0 = c00 * (1 - Yd) + c10 * Yd;
    const AccumulationValueType<T> c1 = c01 * (1 - Yd) + c11 * Yd;

    const AccumulationValueType<T> c = c0 * (1 - Zd) + c1 * Zd;

    return c;
  }

  /**
   * @brief This is the main algorithm to perform the interpolation and get a final value that is placed into the transformed
   * voxel. This uses Trilinear interpolation which will devolve into Bilinear and Linear interpolation depending on the
   * values of U, V and W.
   *
   * OOC optimization: mirrors the RotateImageGeometryWithNearestNeighbor slab-cache pattern but
   * with a +/- 1 Z margin added to the needed source Z range so that all 8 trilinear corner
   * neighbors around every output voxel are guaranteed to live inside the cached slab. Each
   * output Z-slice is accumulated into a local buffer and flushed with a single
   * copyFromBuffer(), eliminating per-voxel virtual dispatch and HDF5 chunk thrashing.
   */
  void operator()() const
  {
    using DataArrayType = DataArray<T>;

    const auto& sourceArray = dynamic_cast<const DataArrayType&>(*m_SourceArray);
    const auto& oldDataStore = sourceArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComps = sourceArray.getNumberOfComponents();
    if(numComps == 0)
    {
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Number of Components was Zero for array. Exiting Transform.", sourceArray.getName()));
      return;
    }

    m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Transform Starting", sourceArray.getName()));

    auto& newDataStore = m_TargetArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    DataStructure tempDataStructure;
    ImageGeom* origImageGeomPtr = ImageGeom::Create(tempDataStructure, "Temp");
    origImageGeomPtr->setDimensions(m_Params.OriginalDims);
    origImageGeomPtr->setSpacing(m_Params.OriginalSpacing);
    origImageGeomPtr->setOrigin(m_Params.OriginalOrigin);

    ImageGeom* destImageGeomPtr = ImageGeom::Create(tempDataStructure, "dest image geom");
    destImageGeomPtr->setDimensions(m_Params.TransformedDims);
    destImageGeomPtr->setSpacing(m_Params.TransformedSpacing);
    destImageGeomPtr->setOrigin(m_Params.TransformedOrigin);

    const int64 srcDimX = static_cast<int64>(m_Params.OriginalDims[0]);
    const int64 srcDimY = static_cast<int64>(m_Params.OriginalDims[1]);
    const int64 srcDimZ = static_cast<int64>(m_Params.OriginalDims[2]);
    const usize srcSliceSize = static_cast<usize>(srcDimX * srcDimY);
    const usize outSliceSize = static_cast<usize>(m_Params.outputDims[0] * m_Params.outputDims[1]);

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    // Output slice buffer (one Z-slice of the output geometry)
    auto outSliceBuf = std::make_unique<T[]>(outSliceSize * numComps);
    std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

    // Source slab cache: holds a contiguous range of source Z-slices
    std::unique_ptr<T[]> srcSlabBuf;
    usize srcSlabBufSize = 0;
    int64 cachedSrcZMin = -1;
    int64 cachedSrcZMax = -2; // invalid range initially

    for(int64 k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.outputDims[2]));

      // Determine source Z range needed for this output slice analytically using the 4 corners
      // of the output slice's XY bounding box (same idea as RotateImageGeometryWithNearestNeighbor)
      // and then pad by +/- 1 on each side to cover the 8-corner trilinear neighbors.
      int64 neededZMin = srcDimZ;
      int64 neededZMax = -1;
      for(int cj = 0; cj <= 1; cj++)
      {
        for(int ci = 0; ci <= 1; ci++)
        {
          int64 cx = ci == 0 ? 0 : static_cast<int64>(m_Params.outputDims[0] - 1);
          int64 cy = cj == 0 ? 0 : static_cast<int64>(m_Params.outputDims[1] - 1);
          int64 cornerFlatIdx = cx + cy * static_cast<int64>(m_Params.outputDims[0]) + k * static_cast<int64>(m_Params.outputDims[0] * m_Params.outputDims[1]);
          Point3Df cornerPt = destImageGeomPtr->getCoordsf(cornerFlatIdx);
          Eigen::Vector4f cornerNew(cornerPt.getX(), cornerPt.getY(), cornerPt.getZ(), 1.0f);
          Eigen::Array4f cornerOld = inverseTransform * cornerNew;
          float32 srcPhysZ = cornerOld[2];
          float32 srcOriginZ = m_Params.OriginalOrigin[2];
          float32 srcSpacingZ = m_Params.OriginalSpacing[2];
          int64 srcZIdx = static_cast<int64>(std::floor((srcPhysZ - srcOriginZ) / srcSpacingZ));
          neededZMin = std::min(neededZMin, srcZIdx);
          neededZMax = std::max(neededZMax, srcZIdx);
        }
      }
      // +/- 1 margin for trilinear corner neighbors, plus +1 extra slop for floor/ceil ambiguity
      neededZMin = std::max(static_cast<int64>(0), neededZMin - 2);
      neededZMax = std::min(srcDimZ - 1, neededZMax + 2);

      if(neededZMin > neededZMax || neededZMin >= srcDimZ || neededZMax < 0)
      {
        // No valid source mapping for this slice — fill with zeros
        std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));
        newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
        continue;
      }

      // Slide the slab cache to cover [neededZMin, neededZMax]. When the new range overlaps the
      // cached range (typical case, where consecutive output slices shift the source window by a
      // small amount), only the delta slices are read from disk; the surviving slices are moved
      // to their new position in the buffer via memmove.
      updateSlabCache<T>(oldDataStore, srcSlabBuf, srcSlabBufSize, cachedSrcZMin, cachedSrcZMax, neededZMin, neededZMax, srcSliceSize, numComps);

      // Process output slice into local buffer. Zero-fill first so that destination voxels whose
      // inverse-transformed coordinate falls outside the source grid remain zero.
      std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

      // Capture state used inside the parallel worker. `destImageGeomPtr`, `origImageGeomPtr`,
      // `srcSlabBuf`, `outSliceBuf`, and `m_Params` / `inverseTransform` are read-only for the
      // duration of the parallel region; threads write to disjoint Y-rows of outSliceBuf.
      T* outSliceBufPtr = outSliceBuf.get();
      const T* srcSlabBufPtr = srcSlabBuf.get();
      const int64 outDimX = static_cast<int64>(m_Params.outputDims[0]);
      const int64 outDimY = static_cast<int64>(m_Params.outputDims[1]);
      const int64 destSliceBaseIdx = outDimX * outDimY * k;

      auto readFromSlab = [&](int64 xIdx, int64 yIdx, int64 zIdx, usize compIndex) -> T {
        int64 xClamped = std::min(std::max<int64>(0, xIdx), srcDimX - 1);
        int64 yClamped = std::min(std::max<int64>(0, yIdx), srcDimY - 1);
        int64 zClamped = std::min(std::max<int64>(0, zIdx), srcDimZ - 1);
        const usize slabLocalIdx = (static_cast<usize>(zClamped - cachedSrcZMin) * srcSliceSize + static_cast<usize>(yClamped) * static_cast<usize>(srcDimX) + static_cast<usize>(xClamped)) * numComps;
        return srcSlabBufPtr[slabLocalIdx + compIndex];
      };

      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, static_cast<usize>(outDimY));
      dataAlg.execute([&](const Range& range) {
        // Per-thread scratch. pValues holds the 8 corner voxel values for one destination voxel.
        std::vector<AccumulationValueType<T>> pValues(8 * numComps);

        for(int64 j = static_cast<int64>(range.min()); j < static_cast<int64>(range.max()); j++)
        {
          for(int64 i = 0; i < outDimX; i++)
          {
            const int64 destIndex = destSliceBaseIdx + outDimX * j + i;
            const usize outBufIdx = static_cast<usize>(j * outDimX + i);
            Point3Df destPoint = destImageGeomPtr->getCoordsf(destIndex);
            Eigen::Vector4f coordsNew(destPoint.getX(), destPoint.getY(), destPoint.getZ(), 1.0f);
            Eigen::Array4f coordsOld = inverseTransform * coordsNew;

            SizeVec3 oldGeomIndices;
            auto errorResult = origImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

            if(errorResult != ImageGeom::ErrorType::NoError)
            {
              // Already zero-filled above; leave as zero.
              continue;
            }

            usize oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];
            auto oldVoxelCenterPoint = origImageGeomPtr->getCoordsf(oldIndex);
            int octant = FindOctant(m_Params, oldVoxelCenterPoint, coordsOld);

            // Inlined slab-aware version of FindInterpolationValues: read 8 corner voxels from the
            // cached slab instead of issuing per-element virtual dispatches against sourceArray.
            const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];
            const Vector3i64 oldIndicesV(static_cast<int64>(oldGeomIndices[0]), static_cast<int64>(oldGeomIndices[1]), static_cast<int64>(oldGeomIndices[2]));
            Eigen::Vector3f p1Coord;
            for(usize ci = 0; ci < 8; ci++)
            {
              auto pIndices = oldIndicesV + indexOffset[ci];
              for(usize compIndex = 0; compIndex < numComps; compIndex++)
              {
                pValues[ci * numComps + compIndex] = readFromSlab(pIndices[0], pIndices[1], pIndices[2], compIndex);
              }
              if(ci == 0)
              {
                p1Coord = {static_cast<float32>(pIndices[0]) * m_Params.xRes + (0.5F * m_Params.xRes) + m_Params.OriginalOrigin[0],
                           static_cast<float32>(pIndices[1]) * m_Params.yRes + (0.5F * m_Params.yRes) + m_Params.OriginalOrigin[1],
                           static_cast<float32>(pIndices[2]) * m_Params.zRes + (0.5F * m_Params.zRes) + m_Params.OriginalOrigin[2]};
              }
            }
            // Compute uvw (normalized interpolation weights) from the coordinate of the coordsOld
            // relative to the P1 corner. Matches the computation in FindInterpolationValues().
            Eigen::Vector3f uvw;
            for(usize axis = 0; axis < 3; axis++)
            {
              float32 cellSize = (axis == 0) ? m_Params.xRes : (axis == 1) ? m_Params.yRes : m_Params.zRes;
              uvw[axis] = (static_cast<float32>(coordsOld[axis]) - p1Coord[axis]) / cellSize;
              if(uvw[axis] < 0.0f)
              {
                uvw[axis] = 0.0f;
              }
              if(uvw[axis] > 1.0f)
              {
                uvw[axis] = 1.0f;
              }
            }

            for(usize compIndex = 0; compIndex < numComps; compIndex++)
            {
              T value = calculateInterpolatedValue(pValues, uvw, numComps, compIndex);
              outSliceBufPtr[outBufIdx * numComps + compIndex] = value;
            }
          }
        }
      });

      // Flush output slice with a single bulk write
      newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
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

    const auto& oldDataStore = m_SourceArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_TargetArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize numComps = oldDataStore.getNumberOfComponents();
    const int64 srcDimX = static_cast<int64>(m_Params.OriginalDims[0]);
    const int64 srcDimY = static_cast<int64>(m_Params.OriginalDims[1]);
    const int64 srcDimZ = static_cast<int64>(m_Params.OriginalDims[2]);
    const usize srcSliceSize = static_cast<usize>(srcDimX * srcDimY);
    const usize outSliceSize = static_cast<usize>(m_Params.outputDims[0] * m_Params.outputDims[1]);

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    // Allocate output slice buffer (bounded: one Z-slice of the output geometry)
    auto outSliceBuf = std::make_unique<T[]>(outSliceSize * numComps);
    std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

    // Source slab cache: holds a contiguous range of source Z-slices
    std::unique_ptr<T[]> srcSlabBuf;
    usize srcSlabBufSize = 0;
    int64 cachedSrcZMin = -1;
    int64 cachedSrcZMax = -2; // invalid range initially

    for(int64 k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.outputDims[2]));

      // Determine source Z range needed for this output slice analytically.
      // The inverse transform maps output physical coords to source physical coords.
      // Source Z is a linear function of output (X, Y) for a fixed output Z, so
      // extrema occur at the corners of the output slice's XY bounding box.
      int64 neededZMin = srcDimZ;
      int64 neededZMax = -1;

      if(m_SliceBySlice)
      {
        neededZMin = k;
        neededZMax = k;
      }
      else
      {
        // Probe all 4 corners — compute source Z regardless of whether the point is in-bounds
        for(int cj = 0; cj <= 1; cj++)
        {
          for(int ci = 0; ci <= 1; ci++)
          {
            int64 cx = ci == 0 ? 0 : static_cast<int64>(m_Params.outputDims[0] - 1);
            int64 cy = cj == 0 ? 0 : static_cast<int64>(m_Params.outputDims[1] - 1);
            int64 cornerFlatIdx = cx + cy * static_cast<int64>(m_Params.outputDims[0]) + k * static_cast<int64>(m_Params.outputDims[0] * m_Params.outputDims[1]);
            Point3Df cornerPt = destImageGeomPtr->getCoordsf(cornerFlatIdx);
            Eigen::Vector4f cornerNew(cornerPt.getX(), cornerPt.getY(), cornerPt.getZ(), 1.0f);
            Eigen::Array4f cornerOld = inverseTransform * cornerNew;
            // Convert source physical Z to cell index (floor division)
            float srcPhysZ = cornerOld[2];
            float srcOriginZ = m_Params.OriginalOrigin[2];
            float srcSpacingZ = m_Params.OriginalSpacing[2];
            int64 srcZIdx = static_cast<int64>(std::floor((srcPhysZ - srcOriginZ) / srcSpacingZ));
            neededZMin = std::min(neededZMin, srcZIdx);
            neededZMax = std::max(neededZMax, srcZIdx);
          }
        }
        // Clamp to valid source range with margin
        neededZMin = std::max(static_cast<int64>(0), neededZMin - 1);
        neededZMax = std::min(srcDimZ - 1, neededZMax + 1);
      }

      if(neededZMin > neededZMax || neededZMin >= srcDimZ || neededZMax < 0)
      {
        // No valid source mapping for this slice — fill with zeros
        std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));
        newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
        continue;
      }
      neededZMin = std::max(neededZMin, static_cast<int64>(0));
      neededZMax = std::min(neededZMax, srcDimZ - 1);

      // Slide the slab cache to cover [neededZMin, neededZMax]. Only the delta slices are
      // re-read when the new range overlaps the cached range.
      updateSlabCache<T>(oldDataStore, srcSlabBuf, srcSlabBufSize, cachedSrcZMin, cachedSrcZMax, neededZMin, neededZMax, srcSliceSize, numComps);

      // Process output slice. Zero-fill first so destination voxels with no valid source mapping
      // remain zero.
      std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

      // Capture state used inside the parallel worker. srcSlabBuf/outSliceBuf are shared: threads
      // read disjoint slab locations and write disjoint Y-rows of outSliceBuf.
      T* outSliceBufPtr = outSliceBuf.get();
      const T* srcSlabBufPtr = srcSlabBuf.get();
      const int64 outDimX = static_cast<int64>(m_Params.outputDims[0]);
      const int64 outDimY = static_cast<int64>(m_Params.outputDims[1]);
      const int64 destSliceBaseIdx = outDimX * outDimY * k;
      const bool sliceBySlice = m_SliceBySlice;

      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, static_cast<usize>(outDimY));
      dataAlg.execute([&](const Range& range) {
        for(int64 j = static_cast<int64>(range.min()); j < static_cast<int64>(range.max()); j++)
        {
          for(int64 i = 0; i < outDimX; i++)
          {
            const int64 destIndex = destSliceBaseIdx + outDimX * j + i;
            const usize outBufIdx = static_cast<usize>(j * outDimX + i);
            Point3Df destPoint = destImageGeomPtr->getCoordsf(destIndex);
            Eigen::Vector4f coordsNew(destPoint.getX(), destPoint.getY(), destPoint.getZ(), 1.0f);
            Eigen::Array4f coordsOld = inverseTransform * coordsNew;

            SizeVec3 oldGeomIndices;
            auto errorResult = srcImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

            if(errorResult == ImageGeom::ErrorType::NoError)
            {
              if(sliceBySlice)
              {
                oldGeomIndices[2] = k;
              }
              int64 srcZ = static_cast<int64>(oldGeomIndices[2]);
              if(srcZ >= cachedSrcZMin && srcZ <= cachedSrcZMax)
              {
                usize slabLocalIdx = (static_cast<usize>(srcZ - cachedSrcZMin) * srcSliceSize + oldGeomIndices[1] * static_cast<usize>(srcDimX) + oldGeomIndices[0]) * numComps;
                for(usize c = 0; c < numComps; c++)
                {
                  outSliceBufPtr[outBufIdx * numComps + c] = srcSlabBufPtr[slabLocalIdx + c];
                }
              }
            }
          }
        }
      });

      newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
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
  : m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_FilterCallback(filterCallback)
  {
  }

  void convert(usize start, usize end) const
  {
    // OOC optimization: process vertices in fixed-size chunks using bulk I/O. Each chunk reads
    // a contiguous range of vertex components into a local buffer, performs the transform in
    // memory, then writes the whole chunk back with a single copyFromBuffer. This replaces
    // per-element at()/setValue() virtual dispatches that force chunk load/evict thrashing in
    // OOC-backed SharedVertexList stores.
    constexpr usize k_ChunkVertices = 16384; // 16K vertices * 3 components * 4 bytes = 192 KB per chunk
    auto& vertexStore = m_Vertices.getDataStoreRef();
    auto chunkBuf = std::make_unique<float32[]>(k_ChunkVertices * 3);

    int64 progCounter = 0;
    const usize totalElements = (end - start);
    const usize progIncrement = std::max(totalElements / 100, static_cast<usize>(1));

    for(usize chunkStart = start; chunkStart < end; chunkStart += k_ChunkVertices)
    {
      if(m_FilterCallback->getCancel())
      {
        return;
      }
      const usize chunkCount = std::min(k_ChunkVertices, end - chunkStart);
      const usize elementOffset = chunkStart * 3;
      const usize elementCount = chunkCount * 3;

      vertexStore.copyIntoBuffer(elementOffset, nonstd::span<float32>(chunkBuf.get(), elementCount));

      for(usize i = 0; i < chunkCount; i++)
      {
        const Eigen::Vector4f position(chunkBuf[3 * i + 0], chunkBuf[3 * i + 1], chunkBuf[3 * i + 2], 1.0f);
        const Eigen::Vector4f transformedPosition = m_TransformationMatrix * position;
        chunkBuf[3 * i + 0] = transformedPosition[0];
        chunkBuf[3 * i + 1] = transformedPosition[1];
        chunkBuf[3 * i + 2] = transformedPosition[2];
      }

      vertexStore.copyFromBuffer(elementOffset, nonstd::span<const float32>(chunkBuf.get(), elementCount));

      progCounter += chunkCount;
      if(progCounter > static_cast<int64>(progIncrement))
      {
        m_FilterCallback->sendThreadSafeProgressMessage(progCounter);
        progCounter = 0;
      }
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
