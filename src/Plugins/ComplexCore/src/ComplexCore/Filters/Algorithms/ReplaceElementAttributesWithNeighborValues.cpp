#include "ReplaceElementAttributesWithNeighborValues.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

namespace
{
const int32 k_GreaterThan = 1;

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

  virtual bool compare(T a, T b) const = 0;
  virtual bool compare1(T a, T b) const = 0;
  virtual bool compare2(T a, T b) const = 0;
};

template <typename T>
class LessThanComparison : public IComparisonFunctor<T>
{
public:
  LessThanComparison() = default;
  virtual ~LessThanComparison() = default;

  LessThanComparison(const LessThanComparison&) = delete;            // Copy Constructor Not Implemented
  LessThanComparison(LessThanComparison&&) = delete;                 // Move Constructor Not Implemented
  LessThanComparison& operator=(const LessThanComparison&) = delete; // Copy Assignment Not Implemented
  LessThanComparison& operator=(LessThanComparison&&) = delete;      // Move Assignment Not Implemented

  bool compare(T a, T b) const override
  {
    return a < b;
  }
  bool compare1(T a, T b) const override
  {
    return a >= b;
  }
  bool compare2(T a, T b) const override
  {
    return a > b;
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

  bool compare(T a, T b) const override
  {
    return a > b;
  }
  bool compare1(T a, T b) const override
  {
    return a <= b;
  }
  bool compare2(T a, T b) const override
  {
    return a < b;
  }
};

struct ExecuteTemplate
{

  template <typename T>
  void operator()(const DataStructure& dataStructure, const ImageGeom& imageGeom, IDataArray& inputIDataArray, int32 comparisonAlgorithm, float thresholdValue, bool loopUntilDone,
                  const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  {
    using DataArrayType = DataArray<T>;

    const DataArrayType& inputArray = dynamic_cast<const DataArrayType&>(inputIDataArray);

    size_t totalPoints = inputArray.getNumberOfTuples();

    Vec3 udims = imageGeom.getDimensions();
    int64_t dims[3] = {
        static_cast<int64_t>(udims[0]),
        static_cast<int64_t>(udims[1]),
        static_cast<int64_t>(udims[2]),
    };

    bool good = true;
    int64_t neighbor = 0;
    int64_t column = 0, row = 0, plane = 0;

    int64_t neighborPoints[6] = {0, 0, 0, 0, 0, 0};
    neighborPoints[0] = static_cast<int64_t>(-dims[0] * dims[1]);
    neighborPoints[1] = static_cast<int64_t>(-dims[0]);
    neighborPoints[2] = static_cast<int64_t>(-1);
    neighborPoints[3] = static_cast<int64_t>(1);
    neighborPoints[4] = static_cast<int64_t>(dims[0]);
    neighborPoints[5] = static_cast<int64_t>(dims[0] * dims[1]);

    std::vector<int64_t> bestNeighbor(totalPoints, -1);

    size_t count = 0;
    float best = 0.0f;
    bool keepGoing = true;

    std::shared_ptr<IComparisonFunctor<T>> comparator = std::make_shared<LessThanComparison<T>>();
    if(comparisonAlgorithm == k_GreaterThan)
    {
      comparator = std::make_shared<GreaterThanComparison<T>>();
    }

    const AttributeMatrix* attrMatrix = imageGeom.getCellData();

    for(const auto& [dataId, dataObject] : *attrMatrix)
    {
    }
    while(keepGoing)
    {
      keepGoing = false;
      count = 0;
      if(shouldCancel)
      {
        break;
      }

      int64_t progIncrement = static_cast<int64_t>(totalPoints / 50);
      int64_t prog = 1;
      int64_t progressInt = 0;
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(comparator->compare(inputArray[i], thresholdValue))
        {
          column = i % dims[0];
          row = (i / dims[0]) % dims[1];
          plane = i / (dims[0] * dims[1]);
          count++;
          best = inputArray[i];
          for(int64_t j = 0; j < 6; j++)
          {
            good = true;
            neighbor = int64_t(i) + neighborPoints[j];
            if(j == 0 && plane == 0)
            {
              good = false;
            }
            if(j == 5 && plane == (dims[2] - 1))
            {
              good = false;
            }
            if(j == 1 && row == 0)
            {
              good = false;
            }
            if(j == 4 && row == (dims[1] - 1))
            {
              good = false;
            }
            if(j == 2 && column == 0)
            {
              good = false;
            }
            if(j == 3 && column == (dims[0] - 1))
            {
              good = false;
            }
            if(good)
            {
              if(comparator->compare1(inputArray[neighbor], thresholdValue) && comparator->compare2(inputArray[neighbor], best))
              {
                best = inputArray[neighbor];
                bestNeighbor[i] = neighbor;
              }
            }
          }
        }
        if(int64_t(i) > prog)
        {
          progressInt = static_cast<int64_t>(((float)i / totalPoints) * 100.0f);
          std::string progressMessage = fmt::format("Processing Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
          prog = prog + progIncrement;
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
          std::string progressMessage = fmt::format("Transferring Loop({}) Progress: {}% Complete", count, progressInt);
          messageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
          prog = prog + progIncrement;
        }

        neighbor = bestNeighbor[i];
        if(neighbor != -1)
        {
          for(const auto& [dataId, dataObject] : *attrMatrix)
          {
            auto& p = dynamic_cast<IDataArray&>(*dataObject);
            p.copyTuple(neighbor, i);
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

  ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray.getDataType(), m_DataStructure, imageGeom, srcIDataArray, m_InputValues->SelectedComparison, m_InputValues->MinConfidence, m_InputValues->Loop,
                      m_ShouldCancel, m_MessageHandler);

  return {};
}
