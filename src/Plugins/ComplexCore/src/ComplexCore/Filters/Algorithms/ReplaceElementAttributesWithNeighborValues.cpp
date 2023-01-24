#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

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

  virtual bool compare(T left, T right) const = 0;
  virtual bool compare1(T left, T right) const = 0;
  virtual bool compare2(T left, T right) const = 0;
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

  bool compare(T left, T right) const override
  {
    return left < right;
  }
  bool compare1(T left, T right) const override
  {
    return left >= right;
  }
  bool compare2(T left, T right) const override
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

  bool compare(T left, T right) const override
  {
    return left > right;
  }
  bool compare1(T left, T right) const override
  {
    return left <= right;
  }
  bool compare2(T left, T right) const override
  {
    return left < right;
  }
};

struct ExecuteTemplate
{
  template <typename T>
  void CompareValues(std::shared_ptr<IComparisonFunctor<T>>& comparator, const DataArray<T>& inputArray, int64 neighbor, float thresholdValue, float32& best, std::vector<int64_t>& bestNeighbor,
                     size_t i) const
  {
    if(comparator->compare1(inputArray[neighbor], thresholdValue) && comparator->compare2(inputArray[neighbor], best))
    {
      best = inputArray[neighbor];
      bestNeighbor[i] = neighbor;
    }
  }

  template <typename T>
  void operator()(const ImageGeom& imageGeom, IDataArray& inputIDataArray, int32 comparisonAlgorithm, float thresholdValue, bool loopUntilDone, const std::atomic_bool& shouldCancel,
                  const IFilter::MessageHandler& messageHandler)
  {
    using DataArrayType = DataArray<T>;

    const auto& inputArray = dynamic_cast<const DataArrayType&>(inputIDataArray);

    const size_t totalPoints = inputArray.getNumberOfTuples();

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

    std::array<int64, 6> neighborPoints = {static_cast<int64_t>(-dims[0] * dims[1]), static_cast<int64_t>(-dims[0]), static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int64_t>(dims[0]),
                                           static_cast<int64_t>(dims[0] * dims[1])};

    std::vector<int64_t> bestNeighbor(totalPoints, -1);

    size_t count = 0;
    bool keepGoing = true;

    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThanIndex)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      int64 progIncrement = static_cast<int64_t>(totalPoints / 50);
      int64 prog = 1;
      int64 progressInt = 0;
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(comparator->compare(inputArray[i], thresholdValue))
        {
          column = i % dims[0];
          row = (i / dims[0]) % dims[1];
          plane = i / (dims[0] * dims[1]);
          count++;
          float32 best = inputArray[i];

          neighbor = static_cast<int64>(i) + neighborPoints[0];
          if(plane != 0)
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
          neighbor = static_cast<int64>(i) + neighborPoints[1];
          if(row != 0)
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
          neighbor = static_cast<int64>(i) + neighborPoints[2];
          if(column != 0)
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
          neighbor = static_cast<int64>(i) + neighborPoints[3];
          if(column != (dims[0] - 1))
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
          neighbor = static_cast<int64>(i) + neighborPoints[4];
          if(row != (dims[1] - 1))
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
          neighbor = static_cast<int64>(i) + neighborPoints[5];
          if(plane != (dims[2] - 1))
          {
            CompareValues<T>(comparator, inputArray, neighbor, thresholdValue, best, bestNeighbor, i);
          }
        }
        if(int64_t(i) > prog)
        {
          progressInt = static_cast<int64_t>(((float)i / totalPoints) * 100.0f);
          const std::string progressMessage = fmt::format("Processing Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
          prog += progIncrement;
        }
      }

      if(shouldCancel)
      {
        break;
      }

      progIncrement = static_cast<int64_t>(totalPoints / 50);
      prog = 1;
      progressInt = 0;
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(int64_t(i) > prog)
        {
          progressInt = static_cast<int64_t>(((float)i / totalPoints) * 100.0f);
          const std::string progressMessage = fmt::format("Transferring Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
          prog = prog + progIncrement;
        }

        neighbor = bestNeighbor[i];
        if(neighbor != -1)
        {
          for(const auto& [dataId, dataObject] : *attrMatrix)
          {
            auto& dataArray = dynamic_cast<IDataArray&>(*dataObject);
            dataArray.copyTuple(neighbor, i);
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

  auto& srcIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray.getDataType(), imageGeom, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop, m_ShouldCancel,
                      m_MessageHandler);

  return {};
}
