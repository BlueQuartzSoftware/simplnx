#include "CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

#include <cstring>

using namespace nx::core;

namespace
{
const std::string k_TempGeometryName = ".cropped_image_geometry";
// Number of source Z-slices to read per bulk I/O call. Trades memory footprint for
// I/O call count. At 32 slices with a 1139x1174 uint16 volume the source slab is
// roughly 86 MB; the paired destination slab is smaller. Empirically dropping the
// Z-slice-batched I/O call count by this factor removes essentially all of the
// HDF5 chunk-op overhead that dominated the previous row-at-a-time implementation.
constexpr uint64 k_ZSliceBatch = 32;

/**
 * @brief Copy a single cell-level data array from the source image geometry into the
 * cropped destination geometry using Z-slice-batched bulk I/O.
 *
 * The cropping operation is conceptually a 3D subarray copy: for every destination
 * voxel (x, y, z), the value is read from source voxel (x + xMin, y + yMin, z + zMin).
 *
 * A naive per-voxel implementation issues one DataStore access per component per
 * voxel, which thrashes HDF5 chunk caches on out-of-core arrays. This class instead
 * reads a batch of k_ZSliceBatch full source Z-slices into a contiguous RAM slab,
 * extracts the cropped (xMin..xMax, yMin..yMax) region via per-row std::memcpy,
 * and writes the corresponding destination slab back in a single bulk call.
 *
 * Peak working-set memory is bounded by the slab size
 * (k_ZSliceBatch * srcDimX * srcDimY + k_ZSliceBatch * cropX * cropY) * numComps * sizeof(T),
 * independent of the total dataset size.
 *
 * @tparam T The element type of the cell-level array (uint8, int32, float32, ...).
 */
template <typename T>
class CropImageGeomDataArray
{
public:
  CropImageGeomDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const ImageGeom& srcImageGeom, std::array<uint64, 6> bounds, const std::atomic_bool& shouldCancel)
  : m_OldCellStore(oldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_NewCellStore(newCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>())
  , m_SrcImageGeom(srcImageGeom)
  , m_Bounds(bounds)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~CropImageGeomDataArray() = default;

  CropImageGeomDataArray(const CropImageGeomDataArray&) = default;
  CropImageGeomDataArray(CropImageGeomDataArray&&) noexcept = default;
  CropImageGeomDataArray& operator=(const CropImageGeomDataArray&) = delete;
  CropImageGeomDataArray& operator=(CropImageGeomDataArray&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    const usize numComps = m_OldCellStore.getNumberOfComponents();

    m_NewCellStore.fill(static_cast<T>(-1));

    const auto srcDims = m_SrcImageGeom.getDimensions();
    const uint64 srcDimX = srcDims[0];
    const uint64 srcDimY = srcDims[1];
    const uint64 srcDimZ = srcDims[2];

    // Crop region (in tuples)
    const uint64 xMin = m_Bounds[0];
    const uint64 xMax = m_Bounds[1];
    const uint64 yMin = m_Bounds[2];
    const uint64 yMax = m_Bounds[3];
    const uint64 zMin = m_Bounds[4];
    const uint64 zMax = m_Bounds[5];
    const uint64 cropX = xMax - xMin;
    const uint64 cropY = yMax - yMin;
    const uint64 cropZ = zMax - zMin;

    // OOC optimization: batch K source Z-slices per bulk I/O call. For each batch,
    // read all K full source Z-slices in a single copyIntoBuffer, extract the crop
    // region into a destination slab via in-memory memcpy-per-row, then flush the
    // destination slab with a single copyFromBuffer. Replaces the previous
    // row-at-a-time implementation which issued one I/O pair per (z, y) pair — for
    // a 1489x1139 output that is ~1.7M I/O pairs per data array.
    const uint64 srcSliceTuples = srcDimX * srcDimY;
    const uint64 dstSliceTuples = cropX * cropY;
    const uint64 rowTuples = cropX;
    const uint64 rowElements = rowTuples * numComps;
    const usize rowBytes = rowElements * sizeof(T);

    // Cap the batch at the number of cropped Z-slices so we do not over-allocate
    // for small crops. K may be smaller than k_ZSliceBatch on the last iteration.
    const uint64 initialBatch = std::min<uint64>(k_ZSliceBatch, cropZ);
    auto srcSlab = std::make_unique<T[]>(initialBatch * srcSliceTuples * numComps);
    auto dstSlab = std::make_unique<T[]>(initialBatch * dstSliceTuples * numComps);
    uint64 allocatedBatch = initialBatch;

    for(uint64 zStart = zMin; zStart < zMax; zStart += k_ZSliceBatch)
    {
      if(m_ShouldCancel)
      {
        return;
      }
      const uint64 batch = std::min<uint64>(k_ZSliceBatch, zMax - zStart);

      // Grow the slab buffers only if the caller's bounds do not divide evenly;
      // shrinking below the current allocation is a no-op — the buffers are reused.
      if(batch > allocatedBatch)
      {
        srcSlab = std::make_unique<T[]>(batch * srcSliceTuples * numComps);
        dstSlab = std::make_unique<T[]>(batch * dstSliceTuples * numComps);
        allocatedBatch = batch;
      }

      const usize srcSlabElements = batch * srcSliceTuples * numComps;
      const usize dstSlabElements = batch * dstSliceTuples * numComps;

      // Bulk read K consecutive source Z-slices in a single I/O call.
      const uint64 srcStartTuple = zStart * srcSliceTuples;
      m_OldCellStore.copyIntoBuffer(srcStartTuple * numComps, nonstd::span<T>(srcSlab.get(), srcSlabElements));

      // Extract the crop rows from each in-memory source slice into the destination slab.
      for(uint64 dz = 0; dz < batch; dz++)
      {
        const T* const srcSliceBase = srcSlab.get() + dz * srcSliceTuples * numComps;
        T* const dstSliceBase = dstSlab.get() + dz * dstSliceTuples * numComps;
        for(uint64 yIdx = 0; yIdx < cropY; yIdx++)
        {
          const T* const srcRow = srcSliceBase + ((yMin + yIdx) * srcDimX + xMin) * numComps;
          T* const dstRow = dstSliceBase + (yIdx * cropX) * numComps;
          std::memcpy(dstRow, srcRow, rowBytes);
        }
      }

      // Bulk write the K destination Z-slices in a single I/O call.
      const uint64 dstStartTuple = (zStart - zMin) * dstSliceTuples;
      m_NewCellStore.copyFromBuffer(dstStartTuple * numComps, nonstd::span<const T>(dstSlab.get(), dstSlabElements));
    }

    // Avoid unused-variable warning when built without assertions.
    (void)srcDimZ;
  }

private:
  const AbstractDataStore<T>& m_OldCellStore;
  AbstractDataStore<T>& m_NewCellStore;
  const ImageGeom& m_SrcImageGeom;
  std::array<uint64, 6> m_Bounds;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
CropImageGeometry::CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CropImageGeometry::~CropImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CropImageGeometry::operator()()
{
  auto srcImagePath = m_InputValues->InputImageGeometryPath;
  auto destImagePath = m_InputValues->OutputImageGeometryPath;
  const auto featureIdsArrayPath = m_InputValues->FeatureIdsPath;
  auto shouldRenumberFeatures = m_InputValues->RenumberFeatures;
  auto cellFeatureAMPath = m_InputValues->CellFeatureAttributeMatrixPath;
  auto removeOriginalGeometry = m_InputValues->RemoveOriginalGeometry;

  uint64 xMin = m_InputValues->XMin;
  uint64 xMax = m_InputValues->XMax;
  uint64 yMax = m_InputValues->YMax;
  uint64 yMin = m_InputValues->YMin;
  uint64 zMax = m_InputValues->ZMax;
  uint64 zMin = m_InputValues->ZMin;

  auto& srcImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(srcImagePath);

  // No matter where the AM is (same DC or new DC), we have the correct DC and AM pointers...now it's time to crop
  SizeVec3 udims = srcImageGeom.getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Check to see if the dims have actually changed.
  if(dims[0] == (xMax - xMin) && dims[1] == (yMax - yMin) && dims[2] == (zMax - zMin))
  {
    return {};
  }

  if(removeOriginalGeometry)
  {
    auto tempPathVector = srcImagePath.getPathVector();
    std::string tempName = k_TempGeometryName;
    tempPathVector.back() = tempName;
    destImagePath = DataPath({tempPathVector});
  }

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(destImagePath);
  FloatVec3 oldOrigin = destImageGeom.getOrigin();

  // Check to make sure the new dimensions are not "out of bounds" and warn the user if they are
  if(dims[0] <= xMax)
  {
    std::string errMsg = fmt::format("The Max X value ({}) is greater than the Image Geometry X extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     xMax, dims[0]);
    return MakeErrorResult(-950, errMsg);
  }
  if(dims[1] <= yMax)
  {
    std::string errMsg = fmt::format("The Max Y value ({}) is greater than the Image Geometry Y extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     yMax, dims[1]);
    return MakeErrorResult(-951, errMsg);
  }
  if(dims[2] <= zMax)
  {
    std::string errMsg = fmt::format("The Max Z value ({}) is greater than the Image Geometry Z extent ({})."
                                     " This may lead to junk data being filled into the extra space.",
                                     zMax, dims[2]);
    return MakeErrorResult(-952, errMsg);
  }

  std::array<uint64, 6> bounds = {xMin, xMax + 1, yMin, yMax + 1, zMin, zMax + 1};

  // The actual cropping of the dataStructure arrays is done in parallel where parallel here
  // refers to the cropping of each DataArray being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  const auto& srcCellDataAM = srcImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();
  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();

    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));

    m_MessageHandler(fmt::format("Cropping Volume || Copying Data Array {}", srcName));
    ExecuteParallelFunction<CropImageGeomDataArray>(oldDataArray.getDataType(), taskRunner, oldDataArray, newDataArray, srcImageGeom, bounds, m_ShouldCancel);
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  if(m_ShouldCancel)
  {
    return {};
  }

  // Careful with this next section. We purposefully copy in the original dataStructure arrays
  // into the destination feature attribute matrix so that we have somewhere to start.
  // During the renumbering phase is when those copied arrays will get potentially resized
  // to their proper number of tuples.
  if(shouldRenumberFeatures)
  {
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(featureIdsArrayPath);
    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, cellFeatureAMPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }
    std::vector<DataPath> sourceFeatureDataPaths;
    auto childPathsResult = GetAllChildArrayDataPaths(m_DataStructure, cellFeatureAMPath);
    if(childPathsResult.has_value())
    {
      sourceFeatureDataPaths = childPathsResult.value();
    }

    std::vector<DataPath> destFeatureDataPaths = sourceFeatureDataPaths;
    DataPath destCellFeatureAMPath = destImagePath.createChildPath(cellFeatureAMPath.getTargetName());

    for(auto& dataPath : destFeatureDataPaths)
    {
      dataPath = destCellFeatureAMPath.createChildPath(dataPath.getTargetName());
    }

    // Loop over all the DataPaths and do a deep copy on each DataArray|StringArray
    // so that the updating of the Feature level data can happen. We do a bit of
    // under-the-covers where we actually remove the existing array that preflight
    // created, so we can use the convenience of the DataArray.deepCopy() function.
    for(usize index = 0; index < sourceFeatureDataPaths.size(); index++)
    {
      DataObject* dataObject = m_DataStructure.getData(sourceFeatureDataPaths[index]);
      if(dataObject->getDataObjectType() == DataObject::Type::DataArray)
      {
        auto result = DeepCopy<IDataArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
      else if(dataObject->getDataObjectType() == DataObject::Type::StringArray)
      {
        auto result = DeepCopy<StringArray>(m_DataStructure, sourceFeatureDataPaths[index], destFeatureDataPaths[index]);
        if(result.invalid())
        {
          return result;
        }
      }
    }

    // NOW DO THE ACTUAL RENUMBERING and updating.
    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  // The deferred actions will take care of removing the original and renaming the output if
  // the user decided to do the crop "in place"
  return {};
}
