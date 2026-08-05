#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_GreaterThanIndex = 1;

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
  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, const AbstractDataStore<T>& inputArray, int64 neighbor, float32 thresholdValue, T& best, std::vector<int64>& bestNeighbor,
                     usize i) const
  {
    if(comparator->compare1(inputArray[neighbor], thresholdValue) && comparator->compare2(inputArray[neighbor], best))
    {
      best = inputArray[neighbor];
      bestNeighbor[i] = neighbor;
    }
  }

  template <typename T>
  void operator()(const ImageGeom& imageGeom, const std::vector<IDataArray*>& neighborAMArrays, IDataArray* inputIDataArray, int32 comparisonAlgorithm, float32 thresholdValue, bool loopUntilDone,
                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  {
    const auto& inputStore = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize totalPoints = inputStore.getNumberOfTuples();

    Vec3 udims = imageGeom.getDimensions();
    std::array<int64, 3> dims = {
        static_cast<int64>(udims[0]),
        static_cast<int64>(udims[1]),
        static_cast<int64>(udims[2]),
    };

    // bool good = true;
    int64 neighbor = 0;
    int64 column = 0;
    int64 row = 0;
    int64 plane = 0;

    constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
    const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    std::vector<int64> bestNeighbor(totalPoints, -1);

    usize count = 0;
    bool keepGoing = true;

    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThanIndex)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      auto progIncrement = static_cast<int64>(totalPoints / 50);
      int64 prog = 1;
      int64 progressInt = 0;
      for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        if(comparator->compare(inputStore[voxelIndex], thresholdValue))
        {
          column = voxelIndex % dims[0];
          row = (voxelIndex / dims[0]) % dims[1];
          plane = voxelIndex / (dims[0] * dims[1]);
          count++;
          T best = inputStore[voxelIndex];

          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[0];
          if(plane != 0)
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[1];
          if(row != 0)
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[2];
          if(column != 0)
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[3];
          if(column != (dims[0] - 1))
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[4];
          if(row != (dims[1] - 1))
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
          neighbor = static_cast<int64>(voxelIndex) + neighborVoxelIndexOffsets[5];
          if(plane != (dims[2] - 1))
          {
            CompareValues<T>(comparator, inputStore, neighbor, thresholdValue, best, bestNeighbor, voxelIndex);
          }
        }
        if(voxelIndex > prog)
        {
          progressInt = static_cast<int64>((static_cast<float32>(voxelIndex) / totalPoints) * 100.0f);
          const std::string progressMessage = fmt::format("Processing Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler.sendProgressMessage(progressMessage, static_cast<int32>(progressInt));
          prog += progIncrement;
        }
      }

      if(shouldCancel)
      {
        break;
      }

      progIncrement = static_cast<int64>(totalPoints / 50);
      prog = 1;
      progressInt = 0;
      for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        if(voxelIndex > prog)
        {
          progressInt = static_cast<int64>((static_cast<float32>(voxelIndex) / totalPoints) * 100.0f);
          const std::string progressMessage = fmt::format("Transferring Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler.sendProgressMessage(progressMessage, static_cast<int32>(progressInt));
          prog = prog + progIncrement;
        }

        neighbor = bestNeighbor[voxelIndex];
        if(neighbor != -1)
        {
          for(auto* iDataArray : neighborAMArrays)
          {
            iDataArray->copyTuple(neighbor, voxelIndex);
          }
        }
      }
      if(loopUntilDone && count > 0)
      {
        keepGoing = true;
        // Reset the vector to save unnecessary copies on next iteration
        std::fill(bestNeighbor.begin(), bestNeighbor.end(), -1);
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

  std::vector<IDataArray*> neighborAMArrays = {};

  {
    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    // Collect IDataArray objects from AM (NeighborList and StringArray excluded)
    std::optional<std::vector<DataPath>> optDataPaths = GetAllChildDataPathsOfType<IDataArray>(m_DataStructure, attrMatrix->getDataPaths().front());

    if(!optDataPaths.has_value())
    {
      // No IDataArrays == No work to do
      return {};
    }

    for(const auto& dataPath : optDataPaths.value())
    {
      neighborAMArrays.push_back(m_DataStructure.getDataAs<IDataArray>(dataPath));
    }
  }

  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray->getDataType(), imageGeom, neighborAMArrays, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop,
                      m_ShouldCancel, m_MessageHandler);

  return {};
}
