/**
 * @file ReplaceElementAttributesWithNeighborValues.cpp
 * @brief Threshold-based neighbor replacement algorithm, optimized for
 *        out-of-core (OOC) data stores via Z-slice buffered I/O.
 *
 * ## High-Level Flow (per pass)
 *
 * 1. **Initialize rolling window** -- Load input array Z-slices 0 and 1
 *    into slots 1 (current) and 2 (next) of the three-element window.
 *
 * 2. **Scan every voxel** (Z-major, then Y, then X):
 *    - If the voxel's value fails the threshold comparison (e.g., confidence
 *      index < 0.1), examine its 6 face neighbors via the rolling window.
 *    - Among neighbors that pass the threshold, track the one with the
 *      best (most favorable) value.
 *    - Record that neighbor's global index in the per-slice mark array.
 *
 * 3. **Immediate per-slice transfer** -- After each Z-slice's XY scan
 *    completes, commit the marks via SliceBufferedTransferOneZ for every
 *    array in the Attribute Matrix. This is safe because the marks for
 *    this algorithm always point to face neighbors (within +/- 1 Z-slice),
 *    and the best-neighbor mark only writes to the current voxel (not
 *    across slices like dilation does).
 *
 * 4. **Repeat** -- If Loop is true and any voxels were modified, start a
 *    new pass. Re-read the rolling window because transfers changed values.
 *
 * ## Comparison Functors
 *
 * The algorithm supports two comparison modes via a polymorphic functor:
 * - **LessThanComparison**: Targets voxels whose value < threshold. Prefers
 *   neighbors with the highest value (closest to passing).
 * - **GreaterThanComparison**: Targets voxels whose value > threshold. Prefers
 *   neighbors with the lowest value.
 *
 * Each functor provides three comparison methods:
 * - `compare(value, threshold)`: Does this voxel fail the threshold?
 * - `compare1(neighborValue, threshold)`: Does this neighbor pass?
 * - `compare2(neighborValue, bestSoFar)`: Is this neighbor better than the
 *   current best?
 */

#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

using namespace nx::core;

namespace
{
const int32 k_GreaterThanIndex = 1;

/**
 * @class IComparisonFunctor
 * @brief Abstract base for threshold comparison strategies.
 * @tparam T The element type of the input array being compared.
 *
 * Provides three virtual comparison methods that together define:
 * - Whether a voxel fails the threshold (compare)
 * - Whether a neighbor passes the threshold (compare1)
 * - Whether a neighbor is a better replacement than the current best (compare2)
 */
template <typename T>
class IComparisonFunctor
{
public:
  IComparisonFunctor() = default;
  virtual ~IComparisonFunctor() = default;

  IComparisonFunctor(const IComparisonFunctor&) = delete;            // Copy Constructor Not Implemented
  IComparisonFunctor(IComparisonFunctor&&) = delete;                 // Move Constructor Not Implemented
  IComparisonFunctor& operator=(const IComparisonFunctor&) = delete; // Copy Assignment Not Implemented
  IComparisonFunctor& operator=(IComparisonFunctor&&) = delete;      // Move Assignment Not Implemented

  /** @brief Returns true if `left` fails the threshold relative to `right`. */
  [[nodiscard]] virtual bool compare(T left, T right) const = 0;
  /** @brief Returns true if `left` (a neighbor value) passes the threshold `right`. */
  [[nodiscard]] virtual bool compare1(T left, T right) const = 0;
  /** @brief Returns true if `left` is a better replacement candidate than `right`. */
  [[nodiscard]] virtual bool compare2(T left, T right) const = 0;
};

/**
 * @class LessThanComparison
 * @brief Targets voxels below the threshold; prefers neighbors with higher values.
 * @tparam T The element type of the input array.
 *
 * - compare: value < threshold (voxel is below cutoff)
 * - compare1: neighbor >= threshold (neighbor passes)
 * - compare2: neighbor > best (neighbor is closer to ideal)
 */
template <typename T>
class LessThanComparison : public IComparisonFunctor<T>
{
public:
  LessThanComparison() = default;
  ~LessThanComparison() override = default;

  LessThanComparison(const LessThanComparison&) = delete;            // Copy Constructor Not Implemented
  LessThanComparison(LessThanComparison&&) = delete;                 // Move Constructor Not Implemented
  LessThanComparison& operator=(const LessThanComparison&) = delete; // Copy Assignment Not Implemented
  LessThanComparison& operator=(LessThanComparison&&) = delete;      // Move Assignment Not Implemented

  [[nodiscard]] bool compare(T left, T right) const override
  {
    return left < right;
  }
  [[nodiscard]] bool compare1(T left, T right) const override
  {
    return left >= right;
  }
  [[nodiscard]] bool compare2(T left, T right) const override
  {
    return left > right;
  }
};

/**
 * @class GreaterThanComparison
 * @brief Targets voxels above the threshold; prefers neighbors with lower values.
 * @tparam T The element type of the input array.
 *
 * - compare: value > threshold (voxel is above cutoff)
 * - compare1: neighbor <= threshold (neighbor passes)
 * - compare2: neighbor < best (neighbor is closer to ideal)
 */
template <typename T>
class GreaterThanComparison : public IComparisonFunctor<T>
{
public:
  GreaterThanComparison() = default;
  ~GreaterThanComparison() override = default;
  GreaterThanComparison(const GreaterThanComparison&) = delete;            // Copy Constructor Not Implemented
  GreaterThanComparison(GreaterThanComparison&&) = delete;                 // Move Constructor Not Implemented
  GreaterThanComparison& operator=(const GreaterThanComparison&) = delete; // Copy Assignment Not Implemented
  GreaterThanComparison& operator=(GreaterThanComparison&&) = delete;      // Move Assignment Not Implemented

  [[nodiscard]] bool compare(T left, T right) const override
  {
    return left > right;
  }
  [[nodiscard]] bool compare1(T left, T right) const override
  {
    return left <= right;
  }
  [[nodiscard]] bool compare2(T left, T right) const override
  {
    return left < right;
  }
};

/**
 * @struct ExecuteTemplate
 * @brief Type-dispatched functor that implements the core threshold-replacement
 *        algorithm with OOC-optimized Z-slice buffering.
 *
 * This struct is invoked via ExecuteDataFunction, which instantiates operator()
 * with the correct element type T matching the input array's DataType.
 */
struct ExecuteTemplate
{
  /**
   * @brief Checks whether a neighbor value qualifies as a replacement and
   *        updates the best-neighbor tracking state if so.
   * @tparam T Element type of the input array
   * @param comparator The active comparison functor
   * @param neighborValue The neighbor's value from the rolling-window buffer
   * @param ThresholdValue The user's threshold cutoff
   * @param best [in/out] The best candidate value found so far
   * @param bestNeighbor [in/out] Per-slice mark array tracking the best neighbor
   * @param i In-slice index of the current voxel
   * @param neighborPoint Global flat index of the neighbor voxel
   */
  template <typename T>
  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, T neighborValue, float32 ThresholdValue, float32& best, std::vector<int64>& bestNeighbor, usize i, int64 neighborPoint) const
  {
    if(comparator->compare1(neighborValue, ThresholdValue) && comparator->compare2(neighborValue, best))
    {
      best = neighborValue;
      bestNeighbor[i] = neighborPoint;
    }
  }

  /**
   * @brief Core algorithm: iteratively replaces voxels that fail a threshold
   *        comparison with the best-scoring face neighbor's data.
   * @tparam T Element type of the input array
   * @param imageGeom The ImageGeom defining the voxel grid dimensions
   * @param inputIDataArray Pointer to the input data array (used for comparison)
   * @param comparisonAlgorithm 0 = LessThan, 1 = GreaterThan
   * @param ThresholdValue The user's threshold cutoff value
   * @param loopUntilDone If true, repeat passes until no failing voxels remain
   * @param shouldCancel Atomic cancellation flag
   * @param messageHandler Handler for progress messages
   */
  template <typename T>
  void operator()(const ImageGeom& imageGeom, IDataArray* inputIDataArray, int32 comparisonAlgorithm, float32 ThresholdValue, bool loopUntilDone, const std::atomic_bool& shouldCancel,
                  const IFilter::MessageHandler& messageHandler)
  {
    const auto& inputStore = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize totalPoints = inputStore.getNumberOfTuples();

    Vec3 udims = imageGeom.getDimensions();
    std::array<int64, 3> dims = {
        static_cast<int64>(udims[0]),
        static_cast<int64>(udims[1]),
        static_cast<int64>(udims[2]),
    };

    // Precompute face-neighbor index offsets and iteration order
    constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
    const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

    usize count = 0;
    bool keepGoing = true;

    // Select the comparison strategy based on user choice
    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThanIndex)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    // The Attribute Matrix holds all sibling arrays that should be updated
    // together when a voxel is replaced (e.g., orientations, phases, etc.)
    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    // ---- Z-slice buffering setup ----
    const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
    const usize dimZ = static_cast<usize>(dims[2]);

    // Per-slice best-neighbor marks: O(sliceSize) instead of O(totalPoints).
    // Each entry is -1 (no replacement) or the global flat index of the best
    // neighbor to copy from.
    std::vector<int64> sliceBestNeighbor(sliceSize, -1);

    // Rolling window: 3 Z-slices of the input comparison array.
    // Slot 0 = z-1, slot 1 = z (current), slot 2 = z+1.
    // Uses unique_ptr<T[]> instead of std::vector<T> to avoid the
    // std::vector<bool> bit-packing problem for boolean arrays.
    std::array<std::unique_ptr<T[]>, 3> inputSlices;
    for(auto& is : inputSlices)
    {
      is = std::make_unique<T[]>(sliceSize);
    }

    auto readInputSlice = [&](int64 z, usize slot) {
      const usize zOffset = static_cast<usize>(z) * sliceSize;
      inputStore.copyIntoBuffer(zOffset, nonstd::span<T>(inputSlices[slot].get(), sliceSize));
    };

    // Maps face-neighbor index to rolling-window slot:
    // -Z -> slot 0, -Y/-X/+X/+Y -> slot 1 (same Z), +Z -> slot 2
    constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

    // ---- Main pass loop ----
    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      // Re-initialize rolling window from the (potentially modified) store
      readInputSlice(0, 1);
      if(dims[2] > 1)
      {
        readInputSlice(1, 2);
      }

      auto progIncrement = static_cast<int64>(totalPoints / 50);
      int64 prog = 1;
      int64 progressInt = 0;

      // ---- Z-slice scan loop ----
      for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
      {
        // Advance the rolling window forward by one Z-slice
        if(zIdx > 0)
        {
          std::swap(inputSlices[0], inputSlices[1]);
          std::swap(inputSlices[1], inputSlices[2]);
          if(zIdx + 1 < dims[2])
          {
            readInputSlice(zIdx + 1, 2);
          }
        }

        // ---- Inner XY scan ----
        for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
        {
          for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
          {
            const int64 voxelIndex = zIdx * static_cast<int64>(sliceSize) + yIdx * dims[0] + xIdx;
            const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);

            // Check if this voxel fails the threshold comparison
            if(comparator->compare(inputSlices[1][inSlice], ThresholdValue))
            {
              count++;
              float32 best = inputSlices[1][inSlice];

              const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

              // Map each face neighbor to its in-slice offset
              const std::array<usize, 6> neighborInSlice = {
                  inSlice,                                         // -Z
                  static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                  static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                  static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                  static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                  inSlice                                          // +Z
              };

              // Find the best qualifying neighbor among the 6 face neighbors
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }

                const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
                const T neighborValue = inputSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
                CompareValues<T>(comparator, neighborValue, ThresholdValue, best, sliceBestNeighbor, inSlice, neighborPoint);
              }
            }
            if(voxelIndex > prog)
            {
              progressInt = static_cast<int64>((static_cast<float32>(voxelIndex) / totalPoints) * 100.0f);
              const std::string progressMessage = fmt::format("Processing Loop({}) Progress: {}% Complete", count, progressInt);
              messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32>(progressInt)});
              prog += progIncrement;
            }
          }
        }

        // ---- Immediate per-slice transfer ----
        // Unlike ErodeDilateBadData where dilation marks cross Z-slices,
        // this algorithm only marks the current voxel (the failing one) to
        // receive data from one of its face neighbors. So the marks for
        // each Z-slice are complete as soon as that slice's XY scan finishes.
        // Transfer all sibling arrays in the Attribute Matrix.
        for(const auto& [dataId, dataObject] : *attrMatrix)
        {
          auto* dataArrayPtr = dynamic_cast<IDataArray*>(dataObject.get());
          if(dataArrayPtr == nullptr)
          {
            continue;
          }
          SliceBufferedTransferOneZ(*dataArrayPtr, sliceBestNeighbor, sliceSize, static_cast<usize>(zIdx), dimZ);
        }

        // Clear per-slice marks for the next Z-slice
        std::fill(sliceBestNeighbor.begin(), sliceBestNeighbor.end(), -1);
      }

      if(shouldCancel)
      {
        break;
      }

      // If looping is enabled and modifications were made, schedule another pass
      if(loopUntilDone && count > 0)
      {
        keepGoing = true;
      }
    }
  }
};

} // namespace

// -----------------------------------------------------------------------------
ReplaceElementAttributesWithNeighborValues::ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                       ReplaceElementAttributesWithNeighborValuesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReplaceElementAttributesWithNeighborValues::~ReplaceElementAttributesWithNeighborValues() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReplaceElementAttributesWithNeighborValues::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReplaceElementAttributesWithNeighborValues::operator()()
{

  auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  // Dispatch to the type-specific inner loop. ExecuteDataFunction instantiates
  // ExecuteTemplate::operator()<T> with the correct element type T matching
  // the input array's DataType (int32, float32, uint8, etc.).
  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray->getDataType(), imageGeom, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop, m_ShouldCancel,
                      m_MessageHandler);

  return {};
}
