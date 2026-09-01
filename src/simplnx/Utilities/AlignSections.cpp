#include "AlignSections.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <chrono>

using namespace nx::core;

namespace
{
/**
 * @class AlignSectionsTransferDataImpl
 * @brief Applies one in-place slice shift to one in-memory cell array.
 * @tparam T Specifies the array value type.
 *
 * Direction-dependent iteration prevents a destination write from overwriting
 * a source tuple that the same shift still needs. Concurrent instances must refer
 * to different arrays.
 */
template <typename T>
class AlignSectionsTransferDataImpl
{
public:
  AlignSectionsTransferDataImpl() = delete;
  AlignSectionsTransferDataImpl(const AlignSectionsTransferDataImpl&) = default;
  AlignSectionsTransferDataImpl(AlignSectionsTransferDataImpl&&) noexcept = default;

  AlignSectionsTransferDataImpl(AlignSections* filter, SizeVec3 dims, std::vector<int64_t> xShifts, std::vector<int64_t> yShifts, IDataArray& dataArray)
  : m_Filter(filter)
  , m_Dims(std::move(dims))
  , m_Xshifts(std::move(xShifts))
  , m_Yshifts(std::move(yShifts))
  , m_DataArray(static_cast<DataArray<T>&>(dataArray))
  {
  }

  ~AlignSectionsTransferDataImpl() = default;

  AlignSectionsTransferDataImpl& operator=(const AlignSectionsTransferDataImpl&) = delete;
  AlignSectionsTransferDataImpl& operator=(AlignSectionsTransferDataImpl&&) = delete;

  /**
   * @brief Applies all calculated shifts to this in-memory array.
   *
   * Cancellation stops the operation and can leave later slices unchanged.
   */
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

/**
 * @class AlignSectionsTransferDataOocImpl
 * @brief Applies slice shifts with bounded bulk I/O.
 * @tparam T Specifies the array value type.
 *
 * The operation reads one Z slice, shifts it in memory, and writes it back.
 * Store access stays sequential and avoids per-tuple OOC cache traffic. Two
 * full-slice buffers bound working memory. The output buffer also supports bool
 * without std::vector<bool> proxy storage.
 */
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

  /**
   * @brief Applies all calculated shifts with sequential slice transfers.
   * @return Valid result or the first bulk-I/O error.
   *
   * Cancellation returns a valid result and leaves later slices unchanged.
   */
  Result<> operator()() const
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

    auto sliceBuffer = std::make_unique<T[]>(sliceElements);
    auto outBuffer = std::make_unique<T[]>(sliceElements);

    for(usize i = 1; i < m_Dims[2]; i++)
    {
      progressMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_Dims[2])); });
      if(m_Filter->getCancel())
      {
        return {};
      }

      usize slice = (m_Dims[2] - 1) - i;
      usize sliceOffset = slice * sliceElements;

      // Read the complete source slice before any value in that slice changes.
      auto readResult = dataStore.copyIntoBuffer(sliceOffset, nonstd::span<T>(sliceBuffer.get(), sliceElements));
      if(readResult.invalid())
      {
        return readResult;
      }

      // Zero-fill cells whose shifted source coordinate is outside the image.
      int64_t xShift = m_Xshifts[i];
      int64_t yShift = m_Yshifts[i];

      std::fill(outBuffer.get(), outBuffer.get() + sliceElements, static_cast<T>(0));
      for(usize yIndex = 0; yIndex < dimY; yIndex++)
      {
        for(usize xIndex = 0; xIndex < dimX; xIndex++)
        {
          int64_t srcX = static_cast<int64_t>(xIndex) + xShift;
          int64_t srcY = static_cast<int64_t>(yIndex) + yShift;

          if(srcX >= 0 && srcX < static_cast<int64_t>(dimX) && srcY >= 0 && srcY < static_cast<int64_t>(dimY))
          {
            usize srcBufBase = (static_cast<usize>(srcY) * dimX + static_cast<usize>(srcX)) * numComp;
            usize dstBufBase = (yIndex * dimX + xIndex) * numComp;
            for(usize c = 0; c < numComp; c++)
            {
              outBuffer[dstBufBase + c] = sliceBuffer[srcBufBase + c];
            }
          }
        }
      }
      auto writeResult = dataStore.copyFromBuffer(sliceOffset, nonstd::span<const T>(outBuffer.get(), sliceElements));
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    return {};
  }

private:
  AlignSections* m_Filter = nullptr;
  SizeVec3 m_Dims;
  std::vector<int64_t> m_Xshifts;
  std::vector<int64_t> m_Yshifts;
  nx::core::DataArray<T>& m_DataArray;
};

/**
 * @struct AlignSectionsTransferDataOocFunctor
 * @brief Dispatches the bounded transfer for one runtime array type.
 */
struct AlignSectionsTransferDataOocFunctor
{
  /**
   * @brief Runs the bounded transfer for one array value type.
   * @tparam T Specifies the array value type.
   * @param dataArray Supplies the array to modify.
   * @param filter Supplies cancellation and progress state.
   * @param dims Specifies image dimensions.
   * @param xShifts Supplies one X shift per slice.
   * @param yShifts Supplies one Y shift per slice.
   * @return Valid result or the first bulk-I/O error.
   */
  template <typename T>
  Result<> operator()(IDataArray& dataArray, AlignSections* filter, SizeVec3 dims, std::vector<int64_t> xShifts, std::vector<int64_t> yShifts) const
  {
    return AlignSectionsTransferDataOocImpl<T>(filter, std::move(dims), std::move(xShifts), std::move(yShifts), dataArray)();
  }
};
} // namespace

AlignSections::AlignSections(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_MessageHelper(mesgHandler)
{
}

AlignSections::~AlignSections() noexcept = default;

const std::atomic_bool& AlignSections::getCancel()
{
  return m_ShouldCancel;
}

MessageHelper& AlignSections::getMessageHelper()
{
  return m_MessageHelper;
}

Result<> AlignSections::execute(const SizeVec3& udims, const DataPath& imageGeometryPath)
{
  std::array<int64, 3> dims = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};
  std::vector<int64_t> xShifts(dims[2], 0);
  std::vector<int64_t> yShifts(dims[2], 0);

  // Calculate all shifts before cell data changes.
  Result<> foundShiftsResults = findShifts(xShifts, yShifts);
  if(foundShiftsResults.invalid())
  {
    return foundShiftsResults;
  }

  if(getCancel())
  {
    return {};
  }

  const std::vector<DataPath> selectedCellArrays = getSelectedDataPaths(imageGeometryPath);

  // One OOC target selects the bounded path for every array. This keeps generic
  // store access sequential and avoids concurrent HDF5-backed transfers.
  bool usesOutOfCoreStore = false;
  for(const auto& cellArrayPath : selectedCellArrays)
  {
    const auto* cellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);
    if(cellArray != nullptr && IsOutOfCore(*cellArray))
    {
      usesOutOfCoreStore = true;
      break;
    }
  }
  const bool useOoc = !ForceInCoreAlgorithm() && (ForceOocAlgorithm() || usesOutOfCoreStore);

  if(!selectedCellArrays.empty())
  {
    RecordAlgorithmPathExecution(useOoc ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);
  }

  ParallelTaskAlgorithm taskRunner;

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHelper.sendMessage(fmt::format("Updating DataArray '{}'", cellArrayPath.toString()));
    auto* cellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);
    if(cellArray == nullptr)
    {
      continue;
    }

    if(useOoc)
    {
      auto transferResult = ExecuteDataFunction(AlignSectionsTransferDataOocFunctor{}, cellArray->getDataType(), *cellArray, this, udims, xShifts, yShifts);
      if(transferResult.invalid())
      {
        return transferResult;
      }
    }
    else
    {
      ExecuteParallelFunction<AlignSectionsTransferDataImpl>(cellArray->getDataType(), taskRunner, this, udims, xShifts, yShifts, *cellArray);
    }
  }

  // Wait for every scheduled in-memory array transfer.
  taskRunner.wait();

  return {};
}

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
