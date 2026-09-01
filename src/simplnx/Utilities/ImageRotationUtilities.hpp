#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Range.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/simplnx_export.hpp"

#include <Eigen/Dense>

#include <chrono>
#include <concepts>
#include <cstring>
#include <fstream>
#include <mutex>
#include <new>

/**
 * @namespace nx::core::ImageRotationUtilities
 * @brief Provides image and node geometry transformation utilities.
 */
namespace nx::core::ImageRotationUtilities
{
const Eigen::Vector3f k_XAxis = Eigen::Vector3f::UnitX();
const Eigen::Vector3f k_YAxis = Eigen::Vector3f::UnitY();
const Eigen::Vector3f k_ZAxis = Eigen::Vector3f::UnitZ();

/**
 * @typedef Matrix3fR
 * @brief Defines a row-major 3 by 3 float matrix.
 */
using Matrix3fR = Eigen::Matrix<float32, 3, 3, Eigen::RowMajor>;
/**
 * @typedef Matrix4fR
 * @brief Defines a row-major 4 by 4 float matrix.
 */
using Matrix4fR = Eigen::Matrix<float32, 4, 4, Eigen::RowMajor>;

/**
 * @typedef Vector3i64
 * @brief Defines a three-value Int64 index vector.
 */
using Vector3i64 = Eigen::Array<int64, 1, 3>;

// Worker result aggregation uses this code for a failed nearest-neighbor tuple copy.
constexpr int32 k_NearestNeighborCopyFailed_Error = -6852;

/**
 * @struct RotateArgs
 * @brief Stores source and transformed image dimensions, spacing, and origins.
 */
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
 * @brief Formats a four-by-four transformation matrix.
 * @param transform Specifies the matrix.
 * @return Four signed fixed-point rows.
 */
SIMPLNX_EXPORT std::string GenerateTransformationMatrixDescription(const ImageRotationUtilities::Matrix4fR& transform);

/**
 * @brief Copies 16 stored values to a row-major transformation matrix.
 * @param precomputed Provides at least 16 float values.
 * @return Copied matrix.
 */
SIMPLNX_EXPORT Matrix4fR CopyPrecomputedToTransformationMatrix(const AbstractDataStore<float32>& precomputed);

/**
 * @brief Copies a four-by-four dynamic table to a transformation matrix.
 * @param tableData Provides four rows with four values each.
 * @return Copied matrix.
 */
SIMPLNX_EXPORT Matrix4fR GenerateManualTransformationMatrix(const DynamicTableParameter::ValueType& tableData);

/**
 * @brief Creates an axis-angle rotation matrix.
 * @param pRotationValue Provides XYZ axis values and angle in degrees.
 * @return Homogeneous rotation matrix.
 */
SIMPLNX_EXPORT Matrix4fR GenerateRotationTransformationMatrix(const VectorFloat32Parameter::ValueType& pRotationValue);

/**
 * @brief Creates a translation matrix.
 * @param pTranslationValue Provides XYZ translation.
 * @return Homogeneous translation matrix.
 */
SIMPLNX_EXPORT Matrix4fR GenerateTranslationTransformationMatrix(const VectorFloat32Parameter::ValueType& pTranslationValue);

/**
 * @brief Creates an axis-aligned scale matrix.
 * @param pScaleValue Provides XYZ scale factors.
 * @return Homogeneous scale matrix.
 */
SIMPLNX_EXPORT Matrix4fR GenerateScaleTransformationMatrix(const VectorFloat32Parameter::ValueType& pScaleValue);

/**
 * @brief Transforms a bounding box and calculates axis bounds.
 * @param imageGeomBoundingBox Provides source bounds.
 * @param transformationMatrix Specifies the transformation.
 * @return X, Y, and Z minimum and maximum coordinates.
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const BoundingBox3Df& imageGeomBoundingBox, const Matrix4fR& transformationMatrix);

/**
 * @brief Transforms ImageGeom bounds and calculates axis bounds.
 * @param imageGeometry Provides source bounds.
 * @param transformationMatrix Specifies the transformation.
 * @return X, Y, and Z minimum and maximum coordinates.
 */
SIMPLNX_EXPORT FloatVec6 DetermineMinMaxCoords(const ImageGeom& imageGeometry, const Matrix4fR& transformationMatrix);

/**
 * @brief Calculates the cosine between two vectors.
 * @tparam T Specifies a floating-point scalar type.
 * @param vectorA Provides the first vector.
 * @param vectorB Provides the second vector.
 * @return Cosine value, or 1 when either vector has zero length.
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
 * @brief Selects source spacing nearest to a transformed axis.
 * @param spacing Provides source axis spacing.
 * @param axisNew Provides the transformed axis direction.
 * @return Spacing of the source axis with the largest absolute cosine.
 */
SIMPLNX_EXPORT float32 DetermineSpacing(const FloatVec3& spacing, const Eigen::Vector3f& axisNew);

/**
 * @brief Calculates transformed image dimensions and spatial metadata.
 * @param imageGeom Provides source dimensions and spatial metadata.
 * @param transformationMatrix Specifies rotation, scale, and translation.
 * @return Rotation arguments for source and output geometry.
 */
SIMPLNX_EXPORT ImageRotationUtilities::RotateArgs CreateRotationArgs(const ImageGeom& imageGeom, const Matrix4fR& transformationMatrix);

/**
 * @brief Reads one source component after clamping XYZ indexes.
 * @tparam T Specifies the array scalar type.
 * @param params Provides source dimensions.
 * @param xyzIndex Specifies a possibly exterior source index.
 * @param sourceArray Provides source tuples.
 * @param compIndex Specifies the component.
 * @return Clamped source value.
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

  const usize index = (xyzIndex[2] * params.xp * params.yp) + (xyzIndex[1] * params.xp) + xyzIndex[0];
  return sourceArray[index * sourceArray.getNumberOfComponents() + compIndex];
}

/**
 * @brief Finds the source voxel octant nearest to a coordinate.
 * @param params Provides source spacing.
 * @param centerPoint Specifies the source voxel center.
 * @param coord Specifies the inverse-transformed coordinate.
 * @return Octant index from 0 through 7.
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

/**
 * @typedef AccumulationValueType
 * @brief Uses Float64 accumulation for floating-point input and Int64 otherwise.
 * @tparam T Specifies the source scalar type.
 */
template <class T>
using AccumulationValueType = std::conditional_t<std::is_floating_point_v<T>, float64, int64>;

/**
 * @brief Collects trilinear corner values and normalized interpolation weights.
 * @tparam T Specifies the source scalar type.
 * @param params Provides source dimensions and spatial metadata.
 * @param octant Selects eight corner offsets.
 * @param oldIndicesU Specifies the source voxel index.
 * @param oldCoords Specifies the inverse-transformed physical coordinate.
 * @param sourceArray Provides source tuples.
 * @param pValues Receives eight corner values for each component.
 * @param uvw Receives normalized XYZ interpolation weights.
 * @param hitVoxelCenterPoint Is retained but not used.
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

  // Calculate weights from the inverse-transformed coordinate and corner bounds.
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
 * @class FilterProgressCallback
 * @brief Provides throttled progress, cancellation access, and worker-result aggregation.
 *
 * The object borrows filter callbacks until all tasks join. Result aggregation
 * uses one object mutex. Progress overloads use separate static mutexes.
 */
class FilterProgressCallback
{
public:
  /**
   * @brief Creates a callback from borrowed filter state.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Provides cancellation state.
   */
  FilterProgressCallback(const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel)
  : m_MessageHandler(mesgHandler)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Adds completed nodes and emits throttled aggregate progress.
   * @param counter Specifies newly completed nodes.
   * @warning The counter update occurs outside the throttle mutex.
   */
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

  /**
   * @brief Emits one caller-formatted throttled progress message.
   * @param progressMessage Specifies message text.
   */
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

  /**
   * @brief Merges one worker Result under the result mutex.
   * @param result Provides worker warnings and errors.
   */
  void mergeResult(Result<>&& result)
  {
    const std::lock_guard<std::mutex> lock(m_ResultMutex);
    m_Result = MergeResults(std::move(m_Result), std::move(result));
  }

  /**
   * @brief Moves out the accumulated worker Result.
   * @return Accumulated warnings and errors.
   * @pre All worker tasks have joined.
   */
  Result<> takeResult()
  {
    const std::lock_guard<std::mutex> lock(m_ResultMutex);
    return std::move(m_Result);
  }

private:
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  int32 m_Progcounter = 0;
  std::mutex m_ResultMutex;
  Result<> m_Result;
};

/**
 * @class BoundedDataStorePageCache
 * @brief Fixed-memory LRU cache for arbitrary source-array element reads.
 * @tparam T Specifies the source scalar type.
 *
 * Rotations that map one output slice across most of the source Z range cannot
 * use a Z-slab without allocating nearly the entire source array. This cache
 * retains at most eight 1 MiB flat pages and fills them through bulk
 * DataStore reads. It is intentionally used serially by the OOC transform path.
 */
template <typename T>
class BoundedDataStorePageCache
{
public:
  /**
   * @brief Creates a bounded cache for one borrowed source store.
   * @param store Provides source values for the cache lifetime.
   */
  explicit BoundedDataStorePageCache(const AbstractDataStore<T>& store)
  : m_Store(store)
  , m_PageElements(std::max<usize>(1, (1024 * 1024) / sizeof(T)))
  {
    m_Pages.reserve(k_MaxPages);
  }

  /**
   * @brief Copies one flat element range through cached pages.
   * @param elementOffset Specifies the first source value.
   * @param destination Receives source values.
   * @return Range, allocation, or source-read error, or success.
   *
   * The method joins any required pages while retaining no more than eight.
   */
  Result<> copyElements(usize elementOffset, nonstd::span<T> destination)
  {
    if(elementOffset > m_Store.getSize() || destination.size() > m_Store.getSize() - elementOffset)
    {
      return MakeErrorResult(k_NearestNeighborCopyFailed_Error, "Image transform source-page request exceeds the source array.");
    }

    usize copied = 0;
    while(copied < destination.size())
    {
      const usize currentOffset = elementOffset + copied;
      const usize pageIndex = currentOffset / m_PageElements;
      auto pageResult = page(pageIndex);
      if(pageResult.invalid())
      {
        return ConvertResult(std::move(pageResult));
      }
      Page* pagePtr = pageResult.value();
      const usize inPageOffset = currentOffset - pageIndex * m_PageElements;
      const usize copyCount = std::min(destination.size() - copied, pagePtr->size - inPageOffset);
      std::copy_n(pagePtr->values.get() + inPageOffset, copyCount, destination.data() + copied);
      copied += copyCount;
    }
    return {};
  }

private:
  /**
   * @struct Page
   * @brief Stores one flat source page and its LRU state.
   */
  struct Page
  {
    usize index = std::numeric_limits<usize>::max();
    uint64 useSequence = 0;
    std::unique_ptr<T[]> values;
    usize size = 0;
  };

  /**
   * @brief Returns or loads one source page.
   * @param pageIndex Specifies the flat page index.
   * @return Page pointer or allocation/source-read error.
   */
  Result<Page*> page(usize pageIndex)
  {
    for(auto& page : m_Pages)
    {
      if(page.index == pageIndex)
      {
        page.useSequence = m_NextUseSequence++;
        return {&page};
      }
    }

    Page* destination = nullptr;
    if(m_Pages.size() < k_MaxPages)
    {
      try
      {
        m_Pages.emplace_back();
      } catch(const std::bad_alloc&)
      {
        return MakeErrorResult<Page*>(k_NearestNeighborCopyFailed_Error, "Image transform could not allocate a bounded source page.");
      }
      destination = &m_Pages.back();
    }
    else
    {
      destination = &*std::min_element(m_Pages.begin(), m_Pages.end(), [](const Page& left, const Page& right) { return left.useSequence < right.useSequence; });
    }

    const usize elementOffset = pageIndex * m_PageElements;
    const usize elementCount = std::min(m_PageElements, m_Store.getSize() - elementOffset);
    try
    {
      if(destination->size < elementCount)
      {
        destination->values = std::make_unique<T[]>(elementCount);
        destination->size = elementCount;
      }
    } catch(const std::bad_alloc&)
    {
      return MakeErrorResult<Page*>(k_NearestNeighborCopyFailed_Error, "Image transform could not allocate a bounded source page.");
    }
    auto readResult = m_Store.copyIntoBuffer(elementOffset, nonstd::span<T>(destination->values.get(), elementCount));
    if(readResult.invalid())
    {
      return ConvertResultTo<Page*>(std::move(readResult), nullptr);
    }
    destination->index = pageIndex;
    destination->size = elementCount;
    destination->useSequence = m_NextUseSequence++;
    return {destination};
  }

  static constexpr usize k_MaxPages = 8;
  const AbstractDataStore<T>& m_Store;
  const usize m_PageElements;
  std::vector<Page> m_Pages;
  uint64 m_NextUseSequence = 1;
};

/**
 * @brief Updates a contiguous Z-slice slab cache.
 * @tparam T Specifies the source scalar type.
 * @param srcStore Provides source values.
 * @param slabBuf Provides and receives slab storage.
 * @param slabBufSize Provides and receives allocated element capacity.
 * @param cachedZMin Provides and receives the first cached Z index.
 * @param cachedZMax Provides and receives the last cached Z index.
 * @param newZMin Specifies the first required Z index.
 * @param newZMax Specifies the last required Z index.
 * @param sliceTuples Specifies tuples per Z slice.
 * @param numComps Specifies components per tuple.
 * @return Source bulk-read error, or success.
 * @pre The new range is ordered and inside the source Z range.
 *
 * An overlapping request retains common slices and reads only its new edges.
 * Growth or no overlap causes a complete range read. Capacity does not shrink.
 */
template <typename T>
inline Result<> updateSlabCache(const AbstractDataStore<T>& srcStore, std::unique_ptr<T[]>& slabBuf, usize& slabBufSize, int64& cachedZMin, int64& cachedZMax, int64 newZMin, int64 newZMax,
                                usize sliceTuples, usize numComps)
{
  const usize sliceElems = sliceTuples * numComps;
  const usize needElems = static_cast<usize>(newZMax - newZMin + 1) * sliceElems;

  bool validCache = (cachedZMin >= 0 && cachedZMax >= cachedZMin);

  // Growth discards cached contents and requires a complete range read.
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
    // memmove preserves overlapping cached slices in either shift direction.
    const usize srcOff = static_cast<usize>(overlapMin - cachedZMin) * sliceElems;
    const usize dstOff = static_cast<usize>(overlapMin - newZMin) * sliceElems;
    const usize moveCount = static_cast<usize>(overlapMax - overlapMin + 1) * sliceElems;
    if(srcOff != dstOff)
    {
      std::memmove(slabBuf.get() + dstOff, slabBuf.get() + srcOff, moveCount * sizeof(T));
    }
    // Read a new lower edge before the retained overlap.
    if(newZMin < overlapMin)
    {
      const usize readElems = static_cast<usize>(overlapMin - newZMin) * sliceElems;
      if(auto readResult = srcStore.copyIntoBuffer(static_cast<usize>(newZMin) * sliceElems, nonstd::span<T>(slabBuf.get(), readElems)); readResult.invalid())
      {
        return readResult;
      }
    }
    // Read a new upper edge after the retained overlap.
    if(newZMax > overlapMax)
    {
      const usize readElems = static_cast<usize>(newZMax - overlapMax) * sliceElems;
      const usize readStartZ = static_cast<usize>(overlapMax + 1);
      const usize dstReadOff = static_cast<usize>(overlapMax + 1 - newZMin) * sliceElems;
      if(auto readResult = srcStore.copyIntoBuffer(readStartZ * sliceElems, nonstd::span<T>(slabBuf.get() + dstReadOff, readElems)); readResult.invalid())
      {
        return readResult;
      }
    }
  }
  else
  {
    if(auto readResult = srcStore.copyIntoBuffer(static_cast<usize>(newZMin) * sliceElems, nonstd::span<T>(slabBuf.get(), needElems)); readResult.invalid())
    {
      return readResult;
    }
  }

  cachedZMin = newZMin;
  cachedZMax = newZMax;
  return {};
}

/**
 * @class RotateImageGeometryWithTrilinearInterpolation
 * @brief Resamples one image array through trilinear interpolation.
 * @tparam T Specifies the array scalar type.
 *
 * Resident execution uses a sliding source Z slab. OOC execution uses eight
 * fixed pages so a wide source-Z mapping cannot materialize the complete source.
 */
template <typename T>
class RotateImageGeometryWithTrilinearInterpolation
{
public:
  /**
   * @brief Creates one borrowed trilinear transformation task.
   * @param sourceArray Provides source tuples.
   * @param targetArray Receives transformed tuples.
   * @param rotateArgs Provides source and output spatial metadata.
   * @param transformationMatrix Maps source coordinates to output coordinates.
   * @param filterCallback Receives progress, cancellation, and worker errors.
   */
  RotateImageGeometryWithTrilinearInterpolation(const IDataArray* sourceArray, IDataArray* targetArray, const RotateArgs& rotateArgs, const Matrix4fR& transformationMatrix,
                                                FilterProgressCallback* filterCallback)
  : m_SourceArray(sourceArray)
  , m_TargetArray(targetArray)
  , m_Params(rotateArgs)
  , m_TransformationMatrix(transformationMatrix)
  , m_FilterCallback(filterCallback)
  {
  }

  /**
   * @brief Destroys the borrowed transformation task.
   */
  ~RotateImageGeometryWithTrilinearInterpolation() = default;

  RotateImageGeometryWithTrilinearInterpolation(const RotateImageGeometryWithTrilinearInterpolation&) = default;

  RotateImageGeometryWithTrilinearInterpolation(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = default;

  RotateImageGeometryWithTrilinearInterpolation& operator=(const RotateImageGeometryWithTrilinearInterpolation&) = delete;

  RotateImageGeometryWithTrilinearInterpolation& operator=(RotateImageGeometryWithTrilinearInterpolation&&) noexcept = delete;

  /**
   * @brief Interpolates one component from eight corner values.
   * @param pValues Provides eight corner values for each component.
   * @param uvw Provides normalized XYZ weights.
   * @param numComps Specifies components per tuple.
   * @param compIndex Specifies the output component.
   * @return Trilinear value converted to T.
   * @see https://en.wikipedia.org/wiki/Trilinear_interpolation
   *
   * Integer input accumulates in signed Int64. This prevents unsigned underflow
   * during weighted intermediate calculations.
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
   * @brief Transforms all output slices through trilinear interpolation.
   *
   * Each output slice uses one local buffer and one checked write. Cancellation
   * stops before a later slice. Errors are merged into filterCallback.
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
    const bool usesOutOfCoreStore = IsOutOfCore(*m_SourceArray) || IsOutOfCore(*m_TargetArray);
    const bool useBoundedPageCache = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    // Keep one output Z slice in local memory.
    auto outSliceBuf = std::make_unique<T[]>(outSliceSize * numComps);
    std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

    // Direct uses a contiguous source Z-slab; OOC uses the bounded page cache.
    std::unique_ptr<T[]> srcSlabBuf;
    usize srcSlabBufSize = 0;
    int64 cachedSrcZMin = -1;
    int64 cachedSrcZMax = -2; // invalid range initially
    std::unique_ptr<BoundedDataStorePageCache<T>> sourcePageCache;
    if(useBoundedPageCache)
    {
      sourcePageCache = std::make_unique<BoundedDataStorePageCache<T>>(oldDataStore);
    }

    for(int64 k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.outputDims[2]));

      // Source Z is linear across one output slice. Its four XY corners bound
      // the required source range before trilinear padding.
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
      // Two-slice padding covers corner neighbors and floor or ceiling ambiguity.
      neededZMin = std::max(static_cast<int64>(0), neededZMin - 2);
      neededZMax = std::min(srcDimZ - 1, neededZMax + 2);

      if(neededZMin > neededZMax || neededZMin >= srcDimZ || neededZMax < 0)
      {
        // An exterior slice remains zero-filled.
        std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));
        if(auto writeResult = newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps)); writeResult.invalid())
        {
          m_FilterCallback->mergeResult(
              MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Trilinear destination slice write failed for '{}' at destination Z {}", m_SourceArray->getName(), k)));
          return;
        }
        continue;
      }

      // A resident slab retains overlapping slices and reads only new range edges.
      if(!useBoundedPageCache)
      {
        if(auto readResult = updateSlabCache<T>(oldDataStore, srcSlabBuf, srcSlabBufSize, cachedSrcZMin, cachedSrcZMax, neededZMin, neededZMax, srcSliceSize, numComps); readResult.invalid())
        {
          m_FilterCallback->mergeResult(MakeErrorResult(k_NearestNeighborCopyFailed_Error,
                                                        fmt::format("Trilinear source slab read failed for '{}' at source Z range [{}, {}]", m_SourceArray->getName(), neededZMin, neededZMax)));
          return;
        }
      }

      // Zero-fill destinations that map outside the source grid.
      std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

      // Direct executes rows in parallel over the read-only slab. OOC executes
      // serially because its bounded LRU cache mutates on page misses.
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
      dataAlg.setParallelizationEnabled(!useBoundedPageCache);
      dataAlg.setRange(0, static_cast<usize>(outDimY));
      Result<> boundedReadResult;
      dataAlg.execute([&](const Range& range) {
        // Each worker reuses scratch for eight corner tuples.
        std::vector<AccumulationValueType<T>> pValues(8 * numComps);
        std::vector<T> sourceTuple(numComps);

        for(int64 j = static_cast<int64>(range.min()); j < static_cast<int64>(range.max()); j++)
        {
          for(int64 i = 0; i < outDimX; i++)
          {
            if(boundedReadResult.invalid())
            {
              return;
            }
            const int64 destIndex = destSliceBaseIdx + outDimX * j + i;
            const usize outBufIdx = static_cast<usize>(j * outDimX + i);
            Point3Df destPoint = destImageGeomPtr->getCoordsf(destIndex);
            Eigen::Vector4f coordsNew(destPoint.getX(), destPoint.getY(), destPoint.getZ(), 1.0f);
            Eigen::Array4f coordsOld = inverseTransform * coordsNew;

            SizeVec3 oldGeomIndices;
            auto errorResult = origImageGeomPtr->computeCellIndex(coordsOld.data(), oldGeomIndices);

            if(errorResult != ImageGeom::ErrorType::NoError)
            {
              continue;
            }

            usize oldIndex = (m_Params.OriginalDims[0] * m_Params.OriginalDims[1] * oldGeomIndices[2]) + (m_Params.OriginalDims[0] * oldGeomIndices[1]) + oldGeomIndices[0];
            auto oldVoxelCenterPoint = origImageGeomPtr->getCoordsf(oldIndex);
            int octant = FindOctant(m_Params, oldVoxelCenterPoint, coordsOld);

            // Read eight corner tuples from the active slab or bounded page cache.
            const std::array<Vector3i64, 8>& indexOffset = k_AllOctantOffsets[octant];
            const Vector3i64 oldIndicesV(static_cast<int64>(oldGeomIndices[0]), static_cast<int64>(oldGeomIndices[1]), static_cast<int64>(oldGeomIndices[2]));
            Eigen::Vector3f p1Coord;
            for(usize ci = 0; ci < 8; ci++)
            {
              auto pIndices = oldIndicesV + indexOffset[ci];
              if(useBoundedPageCache)
              {
                const int64 xClamped = std::min(std::max<int64>(0, pIndices[0]), srcDimX - 1);
                const int64 yClamped = std::min(std::max<int64>(0, pIndices[1]), srcDimY - 1);
                const int64 zClamped = std::min(std::max<int64>(0, pIndices[2]), srcDimZ - 1);
                const usize sourceTupleIndex = (static_cast<usize>(zClamped) * srcSliceSize + static_cast<usize>(yClamped) * static_cast<usize>(srcDimX) + static_cast<usize>(xClamped));
                auto readResult = sourcePageCache->copyElements(sourceTupleIndex * numComps, nonstd::span<T>(sourceTuple.data(), numComps));
                if(readResult.invalid())
                {
                  boundedReadResult = std::move(readResult);
                  return;
                }
                for(usize compIndex = 0; compIndex < numComps; compIndex++)
                {
                  pValues[ci * numComps + compIndex] = sourceTuple[compIndex];
                }
              }
              else
              {
                for(usize compIndex = 0; compIndex < numComps; compIndex++)
                {
                  pValues[ci * numComps + compIndex] = readFromSlab(pIndices[0], pIndices[1], pIndices[2], compIndex);
                }
              }
              if(ci == 0)
              {
                p1Coord = {static_cast<float32>(pIndices[0]) * m_Params.xRes + (0.5F * m_Params.xRes) + m_Params.OriginalOrigin[0],
                           static_cast<float32>(pIndices[1]) * m_Params.yRes + (0.5F * m_Params.yRes) + m_Params.OriginalOrigin[1],
                           static_cast<float32>(pIndices[2]) * m_Params.zRes + (0.5F * m_Params.zRes) + m_Params.OriginalOrigin[2]};
              }
            }
            // Normalize the inverse coordinate relative to the first corner.
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

      if(boundedReadResult.invalid())
      {
        m_FilterCallback->mergeResult(std::move(boundedReadResult));
        return;
      }

      // Write one completed output slice.
      if(auto writeResult = newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps)); writeResult.invalid())
      {
        m_FilterCallback->mergeResult(
            MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Trilinear destination slice write failed for '{}' at destination Z {}", m_SourceArray->getName(), k)));
        return;
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

/**
 * @class RotateImageGeometryWithNearestNeighbor
 * @brief Resamples one image array through nearest-neighbor selection.
 * @tparam T Specifies the array scalar type.
 *
 * Resident execution uses a sliding source Z slab. OOC execution uses eight
 * fixed pages. Slice-by-slice mode locks source Z to destination Z.
 */
template <typename T>
class RotateImageGeometryWithNearestNeighbor
{
public:
  /**
   * @brief Creates one borrowed nearest-neighbor transformation task.
   * @param sourceArray Provides source tuples.
   * @param targetArray Receives transformed tuples.
   * @param args Provides source and output spatial metadata.
   * @param transformationMatrix Maps source coordinates to output coordinates.
   * @param sliceBySlice Preserves the destination Z index when true.
   * @param filterCallback Receives progress, cancellation, and worker errors.
   */
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

  /**
   * @brief Destroys the borrowed transformation task.
   */
  ~RotateImageGeometryWithNearestNeighbor() = default;

  RotateImageGeometryWithNearestNeighbor(const RotateImageGeometryWithNearestNeighbor&) = default;

  RotateImageGeometryWithNearestNeighbor(RotateImageGeometryWithNearestNeighbor&&) noexcept = default;

  RotateImageGeometryWithNearestNeighbor& operator=(const RotateImageGeometryWithNearestNeighbor&) = delete;

  RotateImageGeometryWithNearestNeighbor& operator=(RotateImageGeometryWithNearestNeighbor&&) noexcept = delete;

  /**
   * @brief Transforms all output slices through nearest-neighbor selection.
   *
   * Each output slice uses one local buffer and one checked write. Cancellation
   * stops before a later slice. Errors are merged into filterCallback.
   */
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
    const bool usesOutOfCoreStore = IsOutOfCore(*m_SourceArray) || IsOutOfCore(*m_TargetArray);
    const bool useBoundedPageCache = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());

    Matrix4fR inverseTransform = m_TransformationMatrix.inverse();

    // Keep one output Z slice in local memory.
    auto outSliceBuf = std::make_unique<T[]>(outSliceSize * numComps);
    std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

    // Direct uses a contiguous source Z-slab; OOC uses the bounded page cache.
    std::unique_ptr<T[]> srcSlabBuf;
    usize srcSlabBufSize = 0;
    int64 cachedSrcZMin = -1;
    int64 cachedSrcZMax = -2; // invalid range initially
    std::unique_ptr<BoundedDataStorePageCache<T>> sourcePageCache;
    if(useBoundedPageCache)
    {
      sourcePageCache = std::make_unique<BoundedDataStorePageCache<T>>(oldDataStore);
    }

    for(int64 k = 0; k < m_Params.outputDims[2]; k++)
    {
      if(m_FilterCallback->getCancel())
      {
        break;
      }
      m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Interpolating values for slice '{}/{}'", m_SourceArray->getName(), k, m_Params.outputDims[2]));

      // Source Z is linear across one output slice. Its four XY corners bound
      // the required source range.
      int64 neededZMin = srcDimZ;
      int64 neededZMax = -1;

      if(m_SliceBySlice)
      {
        neededZMin = k;
        neededZMax = k;
      }
      else
      {
        // Evaluate all four corners before clamping the source range.
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
            // Convert physical Z to its containing source cell.
            float srcPhysZ = cornerOld[2];
            float srcOriginZ = m_Params.OriginalOrigin[2];
            float srcSpacingZ = m_Params.OriginalSpacing[2];
            int64 srcZIdx = static_cast<int64>(std::floor((srcPhysZ - srcOriginZ) / srcSpacingZ));
            neededZMin = std::min(neededZMin, srcZIdx);
            neededZMax = std::max(neededZMax, srcZIdx);
          }
        }
        // One-slice padding covers mapped cell boundaries.
        neededZMin = std::max(static_cast<int64>(0), neededZMin - 1);
        neededZMax = std::min(srcDimZ - 1, neededZMax + 1);
      }

      if(neededZMin > neededZMax || neededZMin >= srcDimZ || neededZMax < 0)
      {
        // An exterior slice remains zero-filled.
        std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));
        auto writeResult = newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
        if(writeResult.invalid())
        {
          m_FilterCallback->mergeResult(
              MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Nearest-neighbor destination slice write failed for '{}' at destination Z {}", m_SourceArray->getName(), k)));
          return;
        }
        continue;
      }
      neededZMin = std::max(neededZMin, static_cast<int64>(0));
      neededZMax = std::min(neededZMax, srcDimZ - 1);

      // A resident slab retains overlapping slices and reads only new range edges.
      if(!useBoundedPageCache)
      {
        if(auto readResult = updateSlabCache<T>(oldDataStore, srcSlabBuf, srcSlabBufSize, cachedSrcZMin, cachedSrcZMax, neededZMin, neededZMax, srcSliceSize, numComps); readResult.invalid())
        {
          m_FilterCallback->mergeResult(MakeErrorResult(k_NearestNeighborCopyFailed_Error,
                                                        fmt::format("Nearest-neighbor source slab read failed for '{}' at source Z range [{}, {}]", m_SourceArray->getName(), neededZMin, neededZMax)));
          return;
        }
      }

      // Zero-fill destinations that map outside the source grid.
      std::fill(outSliceBuf.get(), outSliceBuf.get() + outSliceSize * numComps, static_cast<T>(0));

      // Direct executes rows in parallel over the read-only slab. OOC executes
      // serially because its bounded LRU cache mutates on page misses.
      T* outSliceBufPtr = outSliceBuf.get();
      const T* srcSlabBufPtr = srcSlabBuf.get();
      const int64 outDimX = static_cast<int64>(m_Params.outputDims[0]);
      const int64 outDimY = static_cast<int64>(m_Params.outputDims[1]);
      const int64 destSliceBaseIdx = outDimX * outDimY * k;
      const bool sliceBySlice = m_SliceBySlice;

      ParallelDataAlgorithm dataAlg;
      dataAlg.setParallelizationEnabled(!useBoundedPageCache);
      dataAlg.setRange(0, static_cast<usize>(outDimY));
      Result<> boundedReadResult;
      dataAlg.execute([&](const Range& range) {
        for(int64 j = static_cast<int64>(range.min()); j < static_cast<int64>(range.max()); j++)
        {
          for(int64 i = 0; i < outDimX; i++)
          {
            if(boundedReadResult.invalid())
            {
              return;
            }
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
              if(useBoundedPageCache)
              {
                const usize sourceTupleIndex = static_cast<usize>(srcZ) * srcSliceSize + oldGeomIndices[1] * static_cast<usize>(srcDimX) + oldGeomIndices[0];
                auto readResult = sourcePageCache->copyElements(sourceTupleIndex * numComps, nonstd::span<T>(outSliceBufPtr + outBufIdx * numComps, numComps));
                if(readResult.invalid())
                {
                  boundedReadResult = std::move(readResult);
                  return;
                }
              }
              else if(srcZ >= cachedSrcZMin && srcZ <= cachedSrcZMax)
              {
                const usize slabLocalIdx = (static_cast<usize>(srcZ - cachedSrcZMin) * srcSliceSize + oldGeomIndices[1] * static_cast<usize>(srcDimX) + oldGeomIndices[0]) * numComps;
                for(usize c = 0; c < numComps; c++)
                {
                  outSliceBufPtr[outBufIdx * numComps + c] = srcSlabBufPtr[slabLocalIdx + c];
                }
              }
            }
          }
        }
      });

      if(boundedReadResult.invalid())
      {
        m_FilterCallback->mergeResult(std::move(boundedReadResult));
        return;
      }

      auto writeResult = newDataStore.copyFromBuffer(static_cast<usize>(k) * outSliceSize * numComps, nonstd::span<const T>(outSliceBuf.get(), outSliceSize * numComps));
      if(writeResult.invalid())
      {
        m_FilterCallback->mergeResult(
            MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Nearest-neighbor destination slice write failed for '{}' at destination Z {}", m_SourceArray->getName(), k)));
        return;
      }
    }
    m_FilterCallback->sendThreadSafeProgressMessage(fmt::format("{}: Transform Ending", m_SourceArray->getName()));
  }

  /**
   * @brief Runs nearest-neighbor conversion for a task wrapper.
   */
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
 * @class ApplyTransformationToNodeGeometry
 * @brief Applies a transformation matrix to node-geometry vertices.
 *
 * Each 16,384-vertex chunk uses checked bulk reads and writes. Errors merge
 * into the shared callback because parallel tasks cannot return Result values.
 */
class ApplyTransformationToNodeGeometry
{
public:
  /**
   * @brief Creates one borrowed node-transformation worker.
   * @param verticesPtr Provides and receives vertex coordinates.
   * @param transformationMatrix Specifies the homogeneous transformation.
   * @param filterCallback Receives progress, cancellation, and worker errors.
   */
  ApplyTransformationToNodeGeometry(IGeometry::SharedVertexList& verticesPtr, const Matrix4fR& transformationMatrix, FilterProgressCallback* filterCallback)
  : m_TransformationMatrix(transformationMatrix)
  , m_Vertices(verticesPtr)
  , m_FilterCallback(filterCallback)
  {
  }

  /**
   * @brief Transforms one vertex range through bounded buffers.
   * @param start Specifies the first vertex.
   * @param end Specifies the exclusive last vertex.
   */
  void convert(usize start, usize end) const
  {
    // Bulk transfer avoids per-component access to disk-backed vertex stores.
    constexpr usize k_ChunkVertices = 16384;
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

      if(auto readResult = vertexStore.copyIntoBuffer(elementOffset, nonstd::span<float32>(chunkBuf.get(), elementCount)); readResult.invalid())
      {
        m_FilterCallback->mergeResult(
            MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Node-geometry vertex read failed at vertex range [{}, {}]", chunkStart, chunkStart + chunkCount - 1)));
        return;
      }

      for(usize i = 0; i < chunkCount; i++)
      {
        const Eigen::Vector4f position(chunkBuf[3 * i + 0], chunkBuf[3 * i + 1], chunkBuf[3 * i + 2], 1.0f);
        const Eigen::Vector4f transformedPosition = m_TransformationMatrix * position;
        chunkBuf[3 * i + 0] = transformedPosition[0];
        chunkBuf[3 * i + 1] = transformedPosition[1];
        chunkBuf[3 * i + 2] = transformedPosition[2];
      }

      if(auto writeResult = vertexStore.copyFromBuffer(elementOffset, nonstd::span<const float32>(chunkBuf.get(), elementCount)); writeResult.invalid())
      {
        m_FilterCallback->mergeResult(
            MakeErrorResult(k_NearestNeighborCopyFailed_Error, fmt::format("Node-geometry vertex write failed at vertex range [{}, {}]", chunkStart, chunkStart + chunkCount - 1)));
        return;
      }

      progCounter += chunkCount;
      if(progCounter > static_cast<int64>(progIncrement))
      {
        m_FilterCallback->sendThreadSafeProgressMessage(progCounter);
        progCounter = 0;
      }
    }
  }

  /**
   * @brief Transforms one scheduler range.
   * @param range Specifies the vertex range.
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
