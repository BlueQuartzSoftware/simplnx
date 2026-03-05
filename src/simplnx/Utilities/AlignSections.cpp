#include "AlignSections.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
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
// In-core transfer: original per-tuple copyTuple/initializeTuple with
// direction-dependent iteration to avoid overwriting source data.
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
    MessageHelper& messageHelper = m_Filter->getMessageHelper();

    ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();

    T var = static_cast<T>(0);

    std::string arrayName = m_DataArray.getName();

    for(size_t i = 1; i < m_Dims[2]; i++)
    {
      progressMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_Dims[2])); });
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

// -----------------------------------------------------------------------------
// OOC transfer: slice-buffered approach. Reads each Z-slice into a local buffer,
// applies the 2D X/Y shift in memory, then writes the shifted data back.
// All DataStore access is sequential, eliminating per-tuple chunk thrashing.
// -----------------------------------------------------------------------------
template <typename T>
class AlignSectionsTransferDataOocImpl
{
public:
  AlignSectionsTransferDataOocImpl() = delete;
  AlignSectionsTransferDataOocImpl(const AlignSectionsTransferDataOocImpl&) = default;
  AlignSectionsTransferDataOocImpl(AlignSectionsTransferDataOocImpl&&) noexcept = default;

  AlignSectionsTransferDataOocImpl(AlignSections* filter, SizeVec3 dims, std::vector<int64_t> xShifts, std::vector<int64_t> yShifts, IDataArray& dataArray)
  : m_Filter(filter)
  , m_Dims(std::move(dims))
  , m_Xshifts(std::move(xShifts))
  , m_Yshifts(std::move(yShifts))
  , m_DataArray(static_cast<DataArray<T>&>(dataArray))
  {
  }

  ~AlignSectionsTransferDataOocImpl() = default;

  AlignSectionsTransferDataOocImpl& operator=(const AlignSectionsTransferDataOocImpl&) = delete;
  AlignSectionsTransferDataOocImpl& operator=(AlignSectionsTransferDataOocImpl&&) = delete;

  void operator()() const
  {
    MessageHelper& messageHelper = m_Filter->getMessageHelper();
    ThrottledMessenger progressMessenger = messageHelper.createThrottledMessenger();

    auto& dataStore = m_DataArray.getDataStoreRef();
    const usize numComp = m_DataArray.getNumberOfComponents();
    const usize dimX = m_Dims[0];
    const usize dimY = m_Dims[1];
    const usize sliceVoxels = dimX * dimY;
    const usize sliceElements = sliceVoxels * numComp;

    std::string arrayName = m_DataArray.getName();

    // Buffer for one Z-slice of data (snapshot before shifting)
    std::vector<T> sliceBuffer(sliceElements);

    for(usize i = 1; i < m_Dims[2]; i++)
    {
      progressMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_Dims[2])); });
      if(m_Filter->getCancel())
      {
        return;
      }

      usize slice = (m_Dims[2] - 1) - i;
      usize sliceOffset = slice * sliceElements;

      // Phase 1: Read entire Z-slice into local buffer (sequential read)
      for(usize idx = 0; idx < sliceElements; idx++)
      {
        sliceBuffer[idx] = dataStore[sliceOffset + idx];
      }

      // Phase 2: Write shifted data back (sequential write)
      // For each destination voxel, find the shifted source in the buffer
      int64_t xShift = m_Xshifts[i];
      int64_t yShift = m_Yshifts[i];

      for(usize yIndex = 0; yIndex < dimY; yIndex++)
      {
        for(usize xIndex = 0; xIndex < dimX; xIndex++)
        {
          int64_t srcX = static_cast<int64_t>(xIndex) + xShift;
          int64_t srcY = static_cast<int64_t>(yIndex) + yShift;

          usize dstBase = sliceOffset + (yIndex * dimX + xIndex) * numComp;

          if(srcX >= 0 && srcX < static_cast<int64_t>(dimX) && srcY >= 0 && srcY < static_cast<int64_t>(dimY))
          {
            usize srcBufBase = (static_cast<usize>(srcY) * dimX + static_cast<usize>(srcX)) * numComp;
            for(usize c = 0; c < numComp; c++)
            {
              dataStore[dstBase + c] = sliceBuffer[srcBufBase + c];
            }
          }
          else
          {
            for(usize c = 0; c < numComp; c++)
            {
              dataStore[dstBase + c] = static_cast<T>(0);
            }
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
, m_MessageHelper(mesgHandler)
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
MessageHelper& AlignSections::getMessageHelper()
{
  return m_MessageHelper;
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

  // Determine whether to use OOC-optimized transfer
  bool useOoc = ForceOocAlgorithm();
  if(!useOoc)
  {
    for(const auto& cellArrayPath : selectedCellArrays)
    {
      const auto& cellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      if(IsOutOfCore(cellArray))
      {
        useOoc = true;
        break;
      }
    }
  }

  ParallelTaskAlgorithm taskRunner;

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHelper.sendMessage(fmt::format("Updating DataArray '{}'", cellArrayPath.toString()));
    auto& cellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);

    if(useOoc)
    {
      ExecuteParallelFunction<AlignSectionsTransferDataOocImpl>(cellArray.getDataType(), taskRunner, this, udims, xShifts, yShifts, cellArray);
    }
    else
    {
      ExecuteParallelFunction<AlignSectionsTransferDataImpl>(cellArray.getDataType(), taskRunner, this, udims, xShifts, yShifts, cellArray);
    }
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
