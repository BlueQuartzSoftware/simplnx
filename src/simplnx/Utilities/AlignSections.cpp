#include "AlignSections.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <chrono>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
class AlignSectionsTransferDataImpl
{
public:
  AlignSectionsTransferDataImpl() = delete;
  AlignSectionsTransferDataImpl(const AlignSectionsTransferDataImpl&) = default;     // Copy Constructor Default Implemented
  AlignSectionsTransferDataImpl(AlignSectionsTransferDataImpl&&) noexcept = default; // Move Constructor Default Implemented

  AlignSectionsTransferDataImpl(AlignSections* filter, SizeVec3 dims, std::vector<int64_t> xShifts, std::vector<int64_t> yShifts, IDataArray& dataArray)
  : m_Filter(filter)
  , m_Dims(std::move(dims))
  , m_Xshifts(std::move(xShifts))
  , m_Yshifts(std::move(yShifts))
  , m_DataArray(static_cast<DataArray<T>&>(dataArray))
  {
  }

  ~AlignSectionsTransferDataImpl() = default;

  AlignSectionsTransferDataImpl& operator=(const AlignSectionsTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  AlignSectionsTransferDataImpl& operator=(AlignSectionsTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  void operator()() const
  {
    T var = static_cast<T>(0);

    for(size_t i = 1; i < m_Dims[2]; i++)
    {
      m_Filter->sendThreadSafeProgressMessage(1);
      if(m_Filter->getCancel())
      {
        return;
      }
      size_t slice = (m_Dims[2] - 1) - i;
      for(size_t yIndex = 0; yIndex < m_Dims[1]; yIndex++)
      {
        for(size_t xIndex = 0; xIndex < m_Dims[0]; xIndex++)
        {
          int64_t xspot = 0;
          int64_t yspot = 0;
          if(m_Yshifts[i] >= 0)
          {
            yspot = static_cast<int64_t>(yIndex);
          }
          else if(m_Yshifts[i] < 0)
          {
            yspot = static_cast<int64_t>(m_Dims[1]) - 1 - static_cast<int64_t>(yIndex);
          }
          if(m_Xshifts[i] >= 0)
          {
            xspot = static_cast<int64_t>(xIndex);
          }
          else if(m_Xshifts[i] < 0)
          {
            xspot = static_cast<int64_t>(m_Dims[0]) - 1 - static_cast<int64_t>(xIndex);
          }
          int64_t newPosition = (slice * m_Dims[0] * m_Dims[1]) + (yspot * m_Dims[0]) + xspot;
          int64_t currentPosition = (slice * m_Dims[0] * m_Dims[1]) + ((yspot + m_Yshifts[i]) * m_Dims[0]) + (xspot + m_Xshifts[i]);
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
  nx::core::DataArray<T>& m_DataArray;
};
} // namespace

// -----------------------------------------------------------------------------
AlignSections::AlignSections(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSections::~AlignSections() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AlignSections::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
const IFilter::MessageHandler& AlignSections::getMessageHandler() const
{
  return m_MessageHandler;
}

// -----------------------------------------------------------------------------
void AlignSections::sendThreadSafeProgressMessage(usize counter)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.incrementPercent(counter);
}

// -----------------------------------------------------------------------------
Result<> AlignSections::execute(const SizeVec3& udims, const DataPath& imageGeometryPath)
{
  std::array<int64, 3> dims = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};
  std::vector<int64_t> xShifts(dims[2], 0);
  std::vector<int64_t> yShifts(dims[2], 0);

  // Find the voxel shifts that need to happen
  Result<> foundShiftsResults = findShifts(xShifts, yShifts);
  if(foundShiftsResults.invalid())
  {
    return foundShiftsResults;
  }

  if(getCancel())
  {
    return {};
  }

  // Now Adjust the actual DataArrays
  const std::vector<DataPath> selectedCellArrays = getSelectedDataPaths(imageGeometryPath);
  m_Throttle.reset(selectedCellArrays.size() * udims[2], "Transferring cell data");

  ParallelTaskAlgorithm taskRunner;

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler.sendInfoMessage(fmt::format("Updating DataArray '{}'", cellArrayPath.toString()));
    auto& cellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);
    ExecuteParallelFunction<AlignSectionsTransferDataImpl>(cellArray.getDataType(), taskRunner, this, udims, xShifts, yShifts, cellArray);
  }

  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  taskRunner.wait();

  return {};
}

// -----------------------------------------------------------------------------
std::vector<DataPath> AlignSections::getSelectedDataPaths(const DataPath& imageGeometryPath) const
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  auto cellDataGroupPath = imageGeometryPath.createChildPath(imageGeom.getCellData()->getName());
  auto optionalResult = GetAllChildDataPaths(m_DataStructure, cellDataGroupPath);
  if(optionalResult.has_value())
  {
    return optionalResult.value();
  }

  return {};
}
