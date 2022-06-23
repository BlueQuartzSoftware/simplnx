#include "AlignSections.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <chrono>

#ifdef COMPLEX_ENABLE_MULTICORE
#define RUN_TASK taskGroup->run
#else
#define RUN_TASK(ARG) (ARG)();
#endif

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
class AlignSectionsTransferDataImpl
{
public:
  AlignSectionsTransferDataImpl() = delete;
  AlignSectionsTransferDataImpl(const AlignSectionsTransferDataImpl&) = default; // Copy Constructor Default Implemented
  AlignSectionsTransferDataImpl(AlignSectionsTransferDataImpl&&) = default;      // Move Constructor Default Implemented

  AlignSectionsTransferDataImpl(AlignSections* filter, SizeVec3 dims, const std::vector<int64_t>& xShifts, const std::vector<int64_t>& yShifts, complex::DataArray<T>& dataArray)
  : m_Filter(filter)
  , m_Dims(dims)
  , m_Xshifts(xShifts)
  , m_Yshifts(yShifts)
  , m_DataArray(dataArray)
  {
  }

  ~AlignSectionsTransferDataImpl() = default;

  AlignSectionsTransferDataImpl& operator=(const AlignSectionsTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  AlignSectionsTransferDataImpl& operator=(AlignSectionsTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  void operator()() const
  {
    size_t slice = 0;
    T var = static_cast<T>(0);

    auto start = std::chrono::steady_clock::now();

    for(size_t i = 1; i < m_Dims[2]; i++)
    {
      auto now = std::chrono::steady_clock::now();
      // Only send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        std::string message = fmt::format("Processing {}: {}% completed", m_DataArray.getName(), static_cast<int32>(100 * (static_cast<float>(i) / static_cast<float>(m_Dims[2]))));
        m_Filter->updateProgress(message);
        start = std::chrono::steady_clock::now();
      }
      if(m_Filter->getCancel())
      {
        return;
      }
      slice = (m_Dims[2] - 1) - i;
      for(size_t l = 0; l < m_Dims[1]; l++)
      {
        for(size_t n = 0; n < m_Dims[0]; n++)
        {
          int64_t xspot = 0;
          int64_t yspot = 0;
          int64_t newPosition = 0;
          int64_t currentPosition = 0;
          if(m_Yshifts[i] >= 0)
          {
            yspot = static_cast<int64_t>(l);
          }
          else if(m_Yshifts[i] < 0)
          {
            yspot = static_cast<int64_t>(m_Dims[1]) - 1 - static_cast<int64_t>(l);
          }
          if(m_Xshifts[i] >= 0)
          {
            xspot = static_cast<int64_t>(n);
          }
          else if(m_Xshifts[i] < 0)
          {
            xspot = static_cast<int64_t>(m_Dims[0]) - 1 - static_cast<int64_t>(n);
          }
          newPosition = (slice * m_Dims[0] * m_Dims[1]) + (yspot * m_Dims[0]) + xspot;
          currentPosition = (slice * m_Dims[0] * m_Dims[1]) + ((yspot + m_Yshifts[i]) * m_Dims[0]) + (xspot + m_Xshifts[i]);
          if((yspot + m_Yshifts[i]) >= 0 && (yspot + m_Yshifts[i]) <= static_cast<int64_t>(m_Dims[1]) - 1 && (xspot + m_Xshifts[i]) >= 0 &&
             (xspot + m_Xshifts[i]) <= static_cast<int64_t>(m_Dims[0]) - 1)
          {
            m_DataArray.copyTuple(static_cast<size_t>(currentPosition), static_cast<size_t>(newPosition));
          }
          if((yspot + m_Yshifts[i]) < 0 || (yspot + m_Yshifts[i]) > static_cast<int64_t>(m_Dims[1] - 1) || (xspot + m_Xshifts[i]) < 0 || (xspot + m_Xshifts[i]) > static_cast<int64_t>(m_Dims[0]) - 1)
          {
            m_DataArray.initializeTuple(newPosition, var);
          }
        }
      }
    }
  }

private:
  AlignSections* m_Filter = nullptr;
  SizeVec3 m_Dims;
  std::vector<int64_t> m_Xshifts;
  std::vector<int64_t> m_Yshifts;
  complex::DataArray<T>& m_DataArray;
};

} // namespace

// -----------------------------------------------------------------------------
AlignSections::AlignSections(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(data)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSections::~AlignSections() = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AlignSections::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void AlignSections::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> AlignSections::execute(complex::AbstractGeometryGrid* gridGeom)
{
  SizeVec3 udims = gridGeom->getDimensions();
  std::array<int64, 3> dims = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};
  std::vector<int64_t> xshifts(dims[2], 0);
  std::vector<int64_t> yshifts(dims[2], 0);

  // Find the voxel shifts that need to happen
  find_shifts(xshifts, yshifts);

  // Now Adjust the actual DataArrays
  std::vector<DataPath> selectedCellArrays = getSelectedDataPaths();

#ifdef COMPLEX_ENABLE_MULTICORE
  std::shared_ptr<tbb::task_group> taskGroup(new tbb::task_group);
  // C++11 RIGHT HERE....
  auto nThreads = static_cast<int32_t>(std::thread::hardware_concurrency()); // Returns ZERO if not defined on this platform
  int32_t threadCount = 0;
#endif

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    m_MessageHandler(fmt::format("Updating DataArray '{}'", cellArrayPath.toString()));
    const auto& oldCellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);

    DataType type = oldCellArray.getDataType();

    switch(type)
    {
    case DataType::boolean: {
      RUN_TASK(AlignSectionsTransferDataImpl<bool>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<bool>>(cellArrayPath)));
      break;
    }
    case DataType::int8: {
      RUN_TASK(AlignSectionsTransferDataImpl<int8>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<int8>>(cellArrayPath)));
      break;
    }
    case DataType::int16: {
      RUN_TASK(AlignSectionsTransferDataImpl<int16>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<int16>>(cellArrayPath)));
      break;
    }
    case DataType::int32: {
      RUN_TASK(AlignSectionsTransferDataImpl<int32>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<int32>>(cellArrayPath)));
      break;
    }
    case DataType::int64: {
      RUN_TASK(AlignSectionsTransferDataImpl<int64>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<int64>>(cellArrayPath)));
      break;
    }
    case DataType::uint8: {
      RUN_TASK(AlignSectionsTransferDataImpl<uint8>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<uint8>>(cellArrayPath)));
      break;
    }
    case DataType::uint16: {
      RUN_TASK(AlignSectionsTransferDataImpl<uint16>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<uint16>>(cellArrayPath)));
      break;
    }
    case DataType::uint32: {
      RUN_TASK(AlignSectionsTransferDataImpl<uint32>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<uint32>>(cellArrayPath)));
      break;
    }
    case DataType::uint64: {
      RUN_TASK(AlignSectionsTransferDataImpl<uint64>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<uint64>>(cellArrayPath)));
      break;
    }
    case DataType::float32: {
      RUN_TASK(AlignSectionsTransferDataImpl<float32>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<float32>>(cellArrayPath)));
      break;
    }
    case DataType::float64: {
      RUN_TASK(AlignSectionsTransferDataImpl<float64>(this, udims, xshifts, yshifts, m_DataStructure.getDataRefAs<DataArray<float64>>(cellArrayPath)));
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
#ifdef COMPLEX_ENABLE_MULTICORE
    threadCount++;
    if(threadCount == nThreads)
    {
      taskGroup->wait();
      threadCount = 0;
    }
#endif
  }

#ifdef COMPLEX_ENABLE_MULTICORE
  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  taskGroup->wait();
#endif

  return {};
}
