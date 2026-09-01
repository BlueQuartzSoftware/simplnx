/**
 * @file ReplaceElementAttributesWithNeighborValues.cpp
 * @brief Implements slice-buffered threshold replacement.
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
constexpr int32 k_GreaterThanIndex = 1;

/**
 * @class IComparisonFunctor
 * @brief Defines threshold and candidate comparisons.
 * @tparam T Specifies the comparison-array value type.
 */
template <typename T>
class IComparisonFunctor
{
public:
  IComparisonFunctor() = default;
  /**
   * @brief Destroys a comparison strategy through the base interface.
   */
  virtual ~IComparisonFunctor() = default;

  IComparisonFunctor(const IComparisonFunctor&) = delete;
  IComparisonFunctor(IComparisonFunctor&&) = delete;
  IComparisonFunctor& operator=(const IComparisonFunctor&) = delete;
  IComparisonFunctor& operator=(IComparisonFunctor&&) = delete;

  /**
   * @brief Tests whether a value fails a threshold.
   * @param left Value to test.
   * @param right Threshold value.
   * @return True if left requires replacement.
   */
  [[nodiscard]] virtual bool compare(T left, T right) const = 0;
  /**
   * @brief Tests whether a neighbor passes a threshold.
   * @param left Neighbor value.
   * @param right Threshold value.
   * @return True if left can supply a replacement.
   */
  [[nodiscard]] virtual bool compare1(T left, T right) const = 0;
  /**
   * @brief Compares a candidate with the current best value.
   * @param left Candidate value.
   * @param right Current best value.
   * @return True if left must replace right.
   */
  [[nodiscard]] virtual bool compare2(T left, T right) const = 0;
};

/**
 * @class LessThanComparison
 * @brief Targets voxels below the threshold; prefers neighbors with higher values.
 * @tparam T Specifies the comparison-array value type.
 */
template <typename T>
class LessThanComparison : public IComparisonFunctor<T>
{
public:
  LessThanComparison() = default;
  ~LessThanComparison() override = default;

  LessThanComparison(const LessThanComparison&) = delete;
  LessThanComparison(LessThanComparison&&) = delete;
  LessThanComparison& operator=(const LessThanComparison&) = delete;
  LessThanComparison& operator=(LessThanComparison&&) = delete;

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
 * @tparam T Specifies the comparison-array value type.
 */
template <typename T>
class GreaterThanComparison : public IComparisonFunctor<T>
{
public:
  GreaterThanComparison() = default;
  ~GreaterThanComparison() override = default;
  GreaterThanComparison(const GreaterThanComparison&) = delete;
  GreaterThanComparison(GreaterThanComparison&&) = delete;
  GreaterThanComparison& operator=(const GreaterThanComparison&) = delete;
  GreaterThanComparison& operator=(GreaterThanComparison&&) = delete;

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
 * @brief Implements typed replacement with a comparison-slice window.
 */
struct ExecuteTemplate
{
  /**
   * @brief Retains a passing neighbor when it is the better candidate.
   * @tparam T Specifies the comparison-array value type.
   * @param comparator Defines failed, passing, and better comparisons.
   * @param neighborValue Candidate value.
   * @param ThresholdValue Threshold converted to T by the comparator call.
   * @param best Receives the best candidate value.
   * @param bestNeighbor Receives the selected global source index.
   * @param i Identifies the destination in the current slice.
   * @param neighborPoint Identifies the candidate in the complete volume.
   */
  template <typename T>
  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, T neighborValue, float32 ThresholdValue, T& best, std::vector<int64>& bestNeighbor, usize i, int64 neighborPoint) const
  {
    if(comparator->compare1(neighborValue, ThresholdValue) && comparator->compare2(neighborValue, best))
    {
      best = neighborValue;
      bestNeighbor[i] = neighborPoint;
    }
  }

  /**
   * @brief Replaces failed tuples through one or more Z-ordered passes.
   * @tparam T Specifies the comparison-array value type.
   * @param imageGeom Supplies dimensions and the cell AttributeMatrix.
   * @param inputIDataArray Supplies scalar values that select replacements.
   * @param comparisonAlgorithm Zero selects less-than. One selects greater-than.
   * @param ThresholdValue Threshold converted to T for comparisons.
   * @param loopUntilDone True to repeat while any value fails.
   * @param shouldCancel Signals cancellation between complete passes.
   * @param messageHandler Receives progress messages.
   *
   * Each sibling array commits one destination slice at a time. Bulk-I/O results
   * are discarded. Loop mode has no pass limit and can fail to terminate when a
   * failed value has no passing face neighbor.
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

    // Precompute face-neighbor offsets and their deterministic tie order.
    constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
    const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

    usize count = 0;
    bool keepGoing = true;

    // Select the comparison strategy.
    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThanIndex)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    // All cell IDataArray siblings receive the selected source tuple.
    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
    const usize dimZ = static_cast<usize>(dims[2]);

    // Slice-sized marks avoid a volume-sized source map.
    std::vector<int64> sliceBestNeighbor(sliceSize, -1);

    // Slots zero, one, and two contain Z-1, Z, and Z+1 comparison values.
    // Raw arrays also provide contiguous storage for bool.
    std::array<std::unique_ptr<T[]>, 3> inputSlices;
    for(auto& is : inputSlices)
    {
      is = std::make_unique<T[]>(sliceSize);
    }

    auto readInputSlice = [&](int64 z, usize slot) {
      const usize zOffset = static_cast<usize>(z) * sliceSize;
      inputStore.copyIntoBuffer(zOffset, nonstd::span<T>(inputSlices[slot].get(), sliceSize));
    };

    // Map each face direction to its comparison-buffer slot.
    constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      // A new pass reads the comparison array after prior tuple transfers.
      readInputSlice(0, 1);
      if(dims[2] > 1)
      {
        readInputSlice(1, 2);
      }

      auto progIncrement = static_cast<int64>(totalPoints / 50);
      int64 prog = 1;
      int64 progressInt = 0;

      for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
      {
        // Advance the comparison window without copying a slice.
        if(zIdx > 0)
        {
          std::swap(inputSlices[0], inputSlices[1]);
          std::swap(inputSlices[1], inputSlices[2]);
          if(zIdx + 1 < dims[2])
          {
            readInputSlice(zIdx + 1, 2);
          }
        }

        for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
        {
          for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
          {
            const int64 voxelIndex = zIdx * static_cast<int64>(sliceSize) + yIdx * dims[0] + xIdx;
            const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);

            if(comparator->compare(inputSlices[1][inSlice], ThresholdValue))
            {
              count++;
              T best = inputSlices[1][inSlice];

              const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

              // Map each face direction to its in-slice position.
              const std::array<usize, 6> neighborInSlice = {
                  inSlice,                                         // -Z
                  static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                  static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                  static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                  static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                  inSlice                                          // +Z
              };

              // Equal candidates keep the first face in the fixed direction order.
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

        // Immediate transfer limits mark memory to one destination slice. A
        // source in Z-1 can already contain data copied earlier in this pass.
        for(const auto& [dataId, dataObject] : *attrMatrix)
        {
          auto* dataArrayPtr = dynamic_cast<IDataArray*>(dataObject.get());
          if(dataArrayPtr == nullptr)
          {
            continue;
          }
          SliceBufferedTransferOneZ(*dataArrayPtr, sliceBestNeighbor, sliceSize, static_cast<usize>(zIdx), dimZ);
        }

        // Reuse the mark buffer for the next destination slice.
        std::fill(sliceBestNeighbor.begin(), sliceBestNeighbor.end(), -1);
      }

      if(shouldCancel)
      {
        break;
      }

      // count records failed values, including values without a passing source.
      if(loopUntilDone && count > 0)
      {
        keepGoing = true;
      }
    }
  }
};

} // namespace

ReplaceElementAttributesWithNeighborValues::ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                       ReplaceElementAttributesWithNeighborValuesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ReplaceElementAttributesWithNeighborValues::~ReplaceElementAttributesWithNeighborValues() noexcept = default;

const std::atomic_bool& ReplaceElementAttributesWithNeighborValues::getCancel() const
{
  return m_ShouldCancel;
}

Result<> ReplaceElementAttributesWithNeighborValues::operator()()
{
  auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray->getDataType(), imageGeom, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop, m_ShouldCancel,
                      m_MessageHandler);

  return {};
}
