#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
const int32 k_GreaterThanIndex = 1;

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

  [[nodiscard]] virtual bool compare(T left, T right) const = 0;
  [[nodiscard]] virtual bool compare1(T left, T right) const = 0;
  [[nodiscard]] virtual bool compare2(T left, T right) const = 0;
};

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

struct ExecuteTemplate
{
  template <typename T>
  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, T neighborValue, float thresholdValue, float32& best, std::vector<int64_t>& bestNeighbor, size_t i,
                     int64 neighborPoint) const
  {
    if(comparator->compare1(neighborValue, thresholdValue) && comparator->compare2(neighborValue, best))
    {
      best = neighborValue;
      bestNeighbor[i] = neighborPoint;
    }
  }

  template <typename T>
  void operator()(const ImageGeom& imageGeom, IDataArray* inputIDataArray, int32 comparisonAlgorithm, float thresholdValue, bool loopUntilDone, const std::atomic_bool& shouldCancel,
                  const IFilter::MessageHandler& messageHandler)
  {
    const auto& inputStore = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const size_t totalPoints = inputStore.getNumberOfTuples();

    Vec3 udims = imageGeom.getDimensions();
    std::array<int64, 3> dims = {
        static_cast<int64>(udims[0]),
        static_cast<int64>(udims[1]),
        static_cast<int64>(udims[2]),
    };

    std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();
    std::vector<int64_t> bestNeighbor(totalPoints, -1);

    size_t count = 0;
    bool keepGoing = true;

    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThanIndex)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    // Z-slice buffering: maintain rolling window of 3 adjacent Z-slices for input array
    // to avoid random OOC chunk access during neighbor lookups.
    const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

    // Rolling window: slot 0 = z-1, slot 1 = z (current), slot 2 = z+1
    std::array<std::vector<T>, 3> inputSlices;
    for(auto& is : inputSlices)
    {
      is.resize(sliceSize);
    }

    auto readInputSlice = [&](int64 z, usize slot) {
      const usize zOffset = static_cast<usize>(z) * sliceSize;
      for(usize i = 0; i < sliceSize; i++)
      {
        inputSlices[slot][i] = inputStore[zOffset + i];
      }
    };

    // Face neighbor ordering: 0=-Z, 1=-Y, 2=-X, 3=+X, 4=+Y, 5=+Z
    constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      // Initialize rolling window: load z=0 into slot 1, z=1 into slot 2
      readInputSlice(0, 1);
      if(dims[2] > 1)
      {
        readInputSlice(1, 2);
      }

      auto progIncrement = static_cast<int64_t>(totalPoints / 50);
      int64 prog = 1;
      int64 progressInt = 0;

      for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
      {
        // Advance rolling window for z > 0
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

            if(comparator->compare(inputSlices[1][inSlice], thresholdValue))
            {
              count++;
              float32 best = inputSlices[1][inSlice];

              std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

              const std::array<usize, 6> neighborInSlice = {
                  inSlice,                                          // -Z
                  static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
                  static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
                  static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
                  static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
                  inSlice                                           // +Z
              };

              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }

                const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
                const T neighborValue = inputSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
                CompareValues<T>(comparator, neighborValue, thresholdValue, best, bestNeighbor, voxelIndex, neighborPoint);
              }
            }
            if(voxelIndex > prog)
            {
              progressInt = static_cast<int64_t>(((float)voxelIndex / totalPoints) * 100.0f);
              const std::string progressMessage = fmt::format("Processing Loop({}) Progress: {}% Complete", count, progressInt);
              messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
              prog += progIncrement;
            }
          }
        }
      }

      if(shouldCancel)
      {
        break;
      }

      progIncrement = static_cast<int64_t>(totalPoints / 50);
      prog = 1;
      progressInt = 0;
      for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        if(voxelIndex > prog)
        {
          progressInt = static_cast<int64_t>(((float)voxelIndex / totalPoints) * 100.0f);
          const std::string progressMessage = fmt::format("Transferring Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
          prog = prog + progIncrement;
        }

        const int64 neighbor = bestNeighbor[voxelIndex];
        if(neighbor != -1)
        {
          for(const auto& [dataId, dataObject] : *attrMatrix)
          {
            auto& dataArray = dynamic_cast<IDataArray&>(*dataObject);
            dataArray.copyTuple(neighbor, voxelIndex);
          }
        }
      }
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
const std::atomic_bool& ReplaceElementAttributesWithNeighborValues::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReplaceElementAttributesWithNeighborValues::operator()()
{

  auto* srcIDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray->getDataType(), imageGeom, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop, m_ShouldCancel,
                      m_MessageHandler);

  return {};
}
