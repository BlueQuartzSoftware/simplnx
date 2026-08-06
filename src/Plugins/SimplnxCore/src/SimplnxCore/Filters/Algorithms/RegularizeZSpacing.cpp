#include "RegularizeZSpacing.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <fstream>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
// Copies each destination tuple from its mapped source tuple. The source tuple
// index is computed from the per-plane mapping: within a plane the X/Y layout is
// unchanged, so only the Z plane is remapped.
template <typename T>
class RegularizeZSpacingArrayImpl
{
public:
  RegularizeZSpacingArrayImpl(RegularizeZSpacing* algorithm, const IDataArray& srcArray, IDataArray& destArray, const std::vector<usize>& newToOldZPlane, usize sliceSize,
                              const std::atomic_bool& shouldCancel)
  : m_AlgorithmPtr(algorithm)
  , m_SrcArray(srcArray)
  , m_DestArray(destArray)
  , m_NewToOldZPlane(newToOldZPlane)
  , m_SliceSize(sliceSize)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()() const
  {
    const auto& srcDataStoreRef = m_SrcArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destDataStoreRef = m_DestArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    // Within a Z plane the X/Y layout is unchanged, so each destination plane maps to a single
    // contiguous slab of m_SliceSize tuples in the source. Copy one whole plane per call rather
    // than one tuple at a time; this is far more efficient and out-of-core friendly.
    const usize numPlanes = m_NewToOldZPlane.size();
    const usize progressIncrement = numPlanes / 100 == 0 ? 1 : numPlanes / 100;
    for(usize destPlane = 0; destPlane < numPlanes; destPlane++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize destTupleStart = destPlane * m_SliceSize;
      const usize srcTupleStart = m_NewToOldZPlane[destPlane] * m_SliceSize;
      destDataStoreRef.copyFrom(destTupleStart, srcDataStoreRef, srcTupleStart, m_SliceSize);

      // Only build/emit a progress string roughly every 1% to avoid formatting a string per plane
      // that the throttled messenger would otherwise discard.
      if(destPlane % progressIncrement == 0 || destPlane == numPlanes - 1)
      {
        const float32 progress = static_cast<float32>(destPlane + 1) / static_cast<float32>(numPlanes) * 100.0f;
        m_AlgorithmPtr->sendThreadSafeProgressMessage(fmt::format("Copying Data Array '{}' {:.0f}% Complete", m_DestArray.getName(), progress));
      }
    }
  }

private:
  RegularizeZSpacing* m_AlgorithmPtr = nullptr;
  const IDataArray& m_SrcArray;
  IDataArray& m_DestArray;
  const std::vector<usize>& m_NewToOldZPlane;
  usize m_SliceSize;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

namespace nx::core
{
// -----------------------------------------------------------------------------
Result<std::vector<float32>> ReadZBoundsFile(const std::filesystem::path& inputFile, usize count)
{
  std::ifstream inStream(inputFile);
  if(!inStream.good())
  {
    return MakeErrorResult<std::vector<float32>>(-5556, fmt::format("Unable to open input file with name '{}'", inputFile.string()));
  }

  std::vector<float32> zBoundValues(count, 0.0f);
  for(usize i = 0; i < count; i++)
  {
    if(!(inStream >> zBoundValues[i]))
    {
      return MakeErrorResult<std::vector<float32>>(
          -5557,
          fmt::format(
              "Input file '{}' did not contain enough parseable values. Expected {} whitespace-delimited float values (ZPoints + 1) but reading stopped after {} (end of file or a non-numeric token).",
              inputFile.string(), count, i));
    }
  }

  return {std::move(zBoundValues)};
}

// -----------------------------------------------------------------------------
usize ComputeRegularizedZDim(float32 lastZBound, float32 newZRes)
{
  auto zP = static_cast<usize>(lastZBound / newZRes);
  if(zP == 0)
  {
    zP = 1;
  }
  return zP;
}

// -----------------------------------------------------------------------------
RegularizeZSpacing::RegularizeZSpacing(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, RegularizeZSpacingInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
, m_Throttle(msgHandler)
{
}

// -----------------------------------------------------------------------------
RegularizeZSpacing::~RegularizeZSpacing() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RegularizeZSpacing::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> RegularizeZSpacing::operator()()
{

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);
  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);

  const SizeVec3 srcDims = selectedImageGeom.getDimensions();
  const SizeVec3 destDims = destImageGeom.getDimensions();
  const usize origZDim = srcDims[2];
  const usize newZDim = destDims[2];
  const usize sliceSize = srcDims[0] * srcDims[1];

  // Read the Z boundary positions (ZPoints + 1 values).
  Result<std::vector<float32>> zBoundsResult = ReadZBoundsFile(m_InputValues->InputFile, origZDim + 1);
  if(zBoundsResult.invalid())
  {
    return ConvertResult(std::move(zBoundsResult));
  }
  const std::vector<float32> zBoundValues = std::move(zBoundsResult.value());

  // Build the mapping from each new (regularly spaced) Z plane to the original Z plane whose
  // boundary interval contains it. This mirrors the legacy RegularizeZSpacing behavior: for a new
  // plane 'i', the source plane is the largest 'iter' in [1, origZDim) with (i * newZRes) > zBoundValues[iter].
  std::vector<usize> newToOldZPlane(newZDim, 0);
  for(usize i = 0; i < newZDim; i++)
  {
    usize plane = 0;
    const float32 newPlanePos = static_cast<float32>(i) * m_InputValues->NewZRes;
    for(usize iter = 1; iter < origZDim; iter++)
    {
      if(newPlanePos > zBoundValues[iter])
      {
        plane = iter;
      }
    }
    newToOldZPlane[i] = plane;
  }

  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  usize arrayIndex = 0;
  const usize totalArrays = srcCellDataAM.getSize();

  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    arrayIndex++;
    // Preflight rejects non-DataArray members (error -5561); guard here as well so a direct
    // invocation of the algorithm cannot throw std::bad_cast.
    const auto* oldDataArrayPtr = dynamic_cast<const IDataArray*>(oldDataObject.get());
    if(oldDataArrayPtr == nullptr)
    {
      return MakeErrorResult(-5561, fmt::format("Cell Attribute Matrix member '{}' is not a DataArray and cannot be resampled by this filter.", oldDataObject->getName()));
    }
    const std::string srcName = oldDataArrayPtr->getName();
    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler.sendInfoMessage(fmt::format("Copying Data Array: '{}' ({}/{})", srcName, arrayIndex, totalArrays));

    ExecuteParallelFunction<RegularizeZSpacingArrayImpl>(oldDataArrayPtr->getDataType(), taskRunner, this, *oldDataArrayPtr, newDataArray, newToOldZPlane, sliceSize, m_ShouldCancel);
  }

  taskRunner.wait();

  return {};
}

// -----------------------------------------------------------------------------
void RegularizeZSpacing::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.trySendMessage(message);
}
} // namespace nx::core
