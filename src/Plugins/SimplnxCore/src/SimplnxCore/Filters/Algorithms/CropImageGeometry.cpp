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
// A larger source slab reduces HDF5 calls but increases scratch for each array task.
constexpr uint64 k_ZSliceBatch = 32;

/**
 * @class CropImageGeomDataArray
 * @brief Copies one cell array through Z-slice slab transfers.
 * @tparam T Cell-array value type.
 *
 * Per-task scratch is at most 32 source slices plus 32 cropped destination
 * slices. ParallelTaskAlgorithm can run several array tasks at the same time,
 * so total scratch is the sum of active tasks.
 *
 * Bulk-transfer Result values are ignored because the worker interface returns
 * void. Cancellation leaves the destination fill value and completed slabs.
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

    // Read the half-open copy bounds prepared by the outer executor.
    const uint64 xMin = m_Bounds[0];
    const uint64 xMax = m_Bounds[1];
    const uint64 yMin = m_Bounds[2];
    const uint64 yMax = m_Bounds[3];
    const uint64 zMin = m_Bounds[4];
    const uint64 zMax = m_Bounds[5];
    const uint64 cropX = xMax - xMin;
    const uint64 cropY = yMax - yMin;
    const uint64 cropZ = zMax - zMin;

    // Read full source slices and extract cropped rows before one slab write.
    const uint64 srcSliceTuples = srcDimX * srcDimY;
    const uint64 dstSliceTuples = cropX * cropY;
    const uint64 rowTuples = cropX;
    const uint64 rowElements = rowTuples * numComps;
    const usize rowBytes = rowElements * sizeof(T);

    // Do not allocate a full batch for a shallow crop.
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

      // Reuse slab buffers. Grow only when a later batch is larger.
      if(batch > allocatedBatch)
      {
        srcSlab = std::make_unique<T[]>(batch * srcSliceTuples * numComps);
        dstSlab = std::make_unique<T[]>(batch * dstSliceTuples * numComps);
        allocatedBatch = batch;
      }

      const usize srcSlabElements = batch * srcSliceTuples * numComps;
      const usize dstSlabElements = batch * dstSliceTuples * numComps;

      // Read consecutive source slices in one transfer.
      const uint64 srcStartTuple = zStart * srcSliceTuples;
      m_OldCellStore.copyIntoBuffer(srcStartTuple * numComps, nonstd::span<T>(srcSlab.get(), srcSlabElements));

      // Extract cropped rows from the resident source slab.
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

      // Write consecutive destination slices in one transfer.
      const uint64 dstStartTuple = (zStart - zMin) * dstSliceTuples;
      m_NewCellStore.copyFromBuffer(dstStartTuple * numComps, nonstd::span<const T>(dstSlab.get(), dstSlabElements));
    }

    // Copy bounds already constrain Z. Suppress the unused dimension value.
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

  // Source and destination paths are resolved before array tasks start.
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

  // Each task owns one source and destination array. Arrays can copy in parallel.
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
  taskRunner.wait();

  if(m_ShouldCancel)
  {
    return {};
  }

  // Copy feature arrays before renumbering so each tuple supplies initial data.
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

    // DeepCopy replaces preflight outputs before renumbering resizes feature data.
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

    // Renumber copied feature data and cropped cell Feature IDs together.
    DataPath destFeatureIdsPath = destImagePath.createChildPath(srcCellDataAM.getName()).createChildPath(featureIdsArrayPath.getTargetName());
    return Sampling::RenumberFeatures(m_DataStructure, destImagePath, destCellFeatureAMPath, featureIdsArrayPath, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  // The deferred actions will take care of removing the original and renaming the output if
  // the user decided to do the crop "in place"
  return {};
}
