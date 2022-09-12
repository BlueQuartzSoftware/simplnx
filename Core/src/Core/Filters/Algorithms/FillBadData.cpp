#include "FillBadData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

#ifdef COMPLEX_ENABLE_MULTICORE
#define RUN_TASK g->run
#else
#define RUN_TASK
#endif

using namespace complex;

namespace
{
template <typename T>
class FillBadDataUpdateTuples
{
public:
  FillBadDataUpdateTuples(FillBadData* filter, const Int32Array& featureIds, DataArray<T>& outputArray, const std::vector<int32>& neighbors)
  : m_Filter(filter)
  , m_FeatureIds(featureIds)
  , m_OuputArray(outputArray)
  , m_Neighbors(neighbors)
  {
  }
  ~FillBadDataUpdateTuples() = default;

  void convert(size_t start, size_t stop) const
  {
    for(size_t tupleIndex = start; tupleIndex < stop; tupleIndex++)
    {
      int32 featureName = m_FeatureIds[tupleIndex];
      int32 neighbor = m_Neighbors[tupleIndex];
      if(featureName < 0 && neighbor != -1 && m_FeatureIds[static_cast<size_t>(neighbor)] > 0)
      {
        m_OuputArray.copyTuple(neighbor, tupleIndex);
      }
    }
  }

  void operator()() const
  {
    convert(0, m_OuputArray.getNumberOfTuples());
  }

private:
  FillBadData* m_Filter = nullptr;
  const Int32Array& m_FeatureIds;
  DataArray<T>& m_OuputArray;
  const std::vector<int32>& m_Neighbors;
};
} // namespace

// -----------------------------------------------------------------------------
FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FillBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FillBadData::operator()()
{
  Int32Array& m_FeatureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->featureIdsArrayPath);
  size_t totalPoints = m_FeatureIds.getNumberOfTuples();

  Int32Array* m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);

  std::vector<int32> m_Neighbors(totalPoints, -1);

  std::vector<bool> m_AlreadyChecked(totalPoints, false);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  size_t count = 1;
  int32_t good = 1;
  int64_t neighbor = 0;
  int64_t index = 0;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  int64_t column = 0;
  int64_t row = 0;
  int64_t plane = 0;
  int64_t neighpoint = 0;
  int32_t featurename = 0;
  int32_t feature = 0;
  size_t numfeatures = 0;
  size_t maxPhase = 0;

  // Find the max value of featureIds;
  for(size_t i = 0; i < totalPoints; i++)
  {
    featurename = m_FeatureIds[i];
    if(featurename > numfeatures)
    {
      numfeatures = featurename;
    }
  }

  if(m_InputValues->storeAsNewPhase)
  {
    for(size_t i = 0; i < totalPoints; i++)
    {
      if((*m_CellPhases)[i] > maxPhase)
      {
        maxPhase = (*m_CellPhases)[i];
      }
    }
  }

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];
  std::vector<int64_t> currentvlist;

  for(size_t iter = 0; iter < totalPoints; iter++)
  {
    m_AlreadyChecked[iter] = false;
    if(m_FeatureIds[iter] != 0)
    {
      m_AlreadyChecked[iter] = true;
    }
  }

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    if(!m_AlreadyChecked[i] && m_FeatureIds[i] == 0)
    {
      currentvlist.push_back(static_cast<int64_t>(i));
      count = 0;
      while(count < currentvlist.size())
      {
        index = currentvlist[count];
        column = index % dims[0];
        row = (index / dims[0]) % dims[1];
        plane = index / (dims[0] * dims[1]);
        for(int32_t j = 0; j < 6; j++)
        {
          good = 1;
          neighbor = index + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = 0;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            good = 0;
          }
          if(j == 1 && row == 0)
          {
            good = 0;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            good = 0;
          }
          if(j == 2 && column == 0)
          {
            good = 0;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            good = 0;
          }
          if(good == 1 && m_FeatureIds[neighbor] == 0 && !m_AlreadyChecked[neighbor])
          {
            currentvlist.push_back(neighbor);
            m_AlreadyChecked[neighbor] = true;
          }
        }
        count++;
      }
      if((int32_t)currentvlist.size() >= m_InputValues->minAllowedDefectSizeValue)
      {
        for(const int64& currentVListValue : currentvlist)
        {
          m_FeatureIds[currentVListValue] = 0;
          if(m_InputValues->storeAsNewPhase)
          {
            (*m_CellPhases)[currentVListValue] = static_cast<int32>(maxPhase + 1);
          }
        }
      }
      if(static_cast<int32>(currentvlist.size()) < m_InputValues->minAllowedDefectSizeValue)
      {
        for(const int64& currentVListValue : currentvlist)
        {
          m_FeatureIds[currentVListValue] = -1;
        }
      }
      currentvlist.clear();
    }
  }

  int32_t current = 0;
  int32_t most = 0;
  std::vector<int32_t> featureCount(numfeatures + 1, 0);

  std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, m_InputValues->cellDataGroupPath, DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
  std::vector<DataPath> selectedCellArrays;
  if(allChildArrays.has_value())
  {
    selectedCellArrays = allChildArrays.value();
  }

#ifdef COMPLEX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> g(new tbb::task_group);
  // C++11 RIGHT HERE....
  int32_t nthreads = static_cast<int32_t>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  int32_t threadCount = 0;
#endif

  while(count != 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    count = 0;
    for(size_t i = 0; i < totalPoints; i++)
    {
      featurename = m_FeatureIds[i];
      if(featurename < 0)
      {
        count++;
        current = 0;
        most = 0;
        x = static_cast<float>(i % dims[0]);
        y = static_cast<float>((i / dims[0]) % dims[1]);
        z = static_cast<float>(i / (dims[0] * dims[1]));
        for(int32_t j = 0; j < 6; j++)
        {
          good = 1;
          neighpoint = static_cast<int64_t>(i + neighpoints[j]);
          if(j == 0 && z == 0)
          {
            good = 0;
          }
          if(j == 5 && z == (dims[2] - 1))
          {
            good = 0;
          }
          if(j == 1 && y == 0)
          {
            good = 0;
          }
          if(j == 4 && y == (dims[1] - 1))
          {
            good = 0;
          }
          if(j == 2 && x == 0)
          {
            good = 0;
          }
          if(j == 3 && x == (dims[0] - 1))
          {
            good = 0;
          }
          if(good == 1)
          {
            feature = m_FeatureIds[neighpoint];
            if(feature > 0)
            {
              featureCount[feature]++;
              current = featureCount[feature];
              if(current > most)
              {
                most = current;
                m_Neighbors[i] = neighpoint;
              }
            }
          }
        }
        for(int32_t l = 0; l < 6; l++)
        {
          good = 1;
          neighpoint = i + neighpoints[l];
          if(l == 0 && z == 0)
          {
            good = 0;
          }
          if(l == 5 && z == (dims[2] - 1))
          {
            good = 0;
          }
          if(l == 1 && y == 0)
          {
            good = 0;
          }
          if(l == 4 && y == (dims[1] - 1))
          {
            good = 0;
          }
          if(l == 2 && x == 0)
          {
            good = 0;
          }
          if(l == 3 && x == (dims[0] - 1))
          {
            good = 0;
          }
          if(good == 1)
          {
            feature = m_FeatureIds[neighpoint];
            if(feature > 0)
            {
              featureCount[feature] = 0;
            }
          }
        }
      }
    }

    for(const auto& cellArrayPath : selectedCellArrays)
    {
      auto& oldCellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      DataType type = oldCellArray.getDataType();

      switch(type)
      {
      case DataType::boolean: {
        RUN_TASK(FillBadDataUpdateTuples<bool>(this, m_FeatureIds, dynamic_cast<BoolArray&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::int8: {
        RUN_TASK(FillBadDataUpdateTuples<int8>(this, m_FeatureIds, dynamic_cast<Int8Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::int16: {
        RUN_TASK(FillBadDataUpdateTuples<int16>(this, m_FeatureIds, dynamic_cast<Int16Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::int32: {
        RUN_TASK(FillBadDataUpdateTuples<int32>(this, m_FeatureIds, dynamic_cast<Int32Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::int64: {
        RUN_TASK(FillBadDataUpdateTuples<int64>(this, m_FeatureIds, dynamic_cast<Int64Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::uint8: {
        RUN_TASK(FillBadDataUpdateTuples<uint8>(this, m_FeatureIds, dynamic_cast<UInt8Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::uint16: {
        RUN_TASK(FillBadDataUpdateTuples<uint16>(this, m_FeatureIds, dynamic_cast<UInt16Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::uint32: {
        RUN_TASK(FillBadDataUpdateTuples<uint32>(this, m_FeatureIds, dynamic_cast<UInt32Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::uint64: {
        RUN_TASK(FillBadDataUpdateTuples<uint64>(this, m_FeatureIds, dynamic_cast<UInt64Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::float32: {
        RUN_TASK(FillBadDataUpdateTuples<float32>(this, m_FeatureIds, dynamic_cast<Float32Array&>(oldCellArray), m_Neighbors));
        break;
      }
      case DataType::float64: {
        RUN_TASK(FillBadDataUpdateTuples<float64>(this, m_FeatureIds, dynamic_cast<Float64Array&>(oldCellArray), m_Neighbors));
        break;
      }
      default: {
        throw std::runtime_error("Invalid DataType");
      }
      }
#ifdef COMPLEX_ENABLE_MULTICORE
      threadCount++;
      if(threadCount == nthreads)
      {
        g->wait();
        threadCount = 0;
      }
#endif
    }

#ifdef COMPLEX_ENABLE_MULTICORE
    // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
    g->wait();
#endif
  }

  return {};
}
