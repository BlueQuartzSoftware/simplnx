#include "ResampleImageGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
class ResampleImageGeomArrayImpl
{
public:
  ResampleImageGeomArrayImpl(ResampleImageGeom* algorithm, const IDataArray& srcArray, IDataArray& destArray, const ImageGeom& srcImageGeom, const ImageGeom& destImageGeom,
                             const std::atomic_bool& shouldCancel)
  : m_AlgorithmPtr(algorithm)
  , m_SrcArray(srcArray)
  , m_DestArray(destArray)
  , m_SrcImageGeom(srcImageGeom)
  , m_DestImageGeom(destImageGeom)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()() const
  {
    const auto& srcDataStore = m_SrcArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destDataStore = m_DestArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    usize counter = 0;

    usize numVoxels = m_DestImageGeom.getNumberOfCells();
    usize counterIncrement = numVoxels / 100 == 0 ? 100 : numVoxels / 100;

    for(usize destVoxelIdx = 0; destVoxelIdx < numVoxels; destVoxelIdx++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      // Get the destination voxel origin (min corner).
      Point3D<float64> coords = m_DestImageGeom.getPlaneCoords(destVoxelIdx);
      // Based on that position, figure out which source voxel we are in...
      std::optional<usize> srcIndex = m_SrcImageGeom.getIndex(coords[0], coords[1], coords[2]);

      if(srcIndex.has_value())
      {
        auto result = destDataStore.copyFrom(destVoxelIdx, srcDataStore, srcIndex.value(), 1);
        if(result.invalid())
        {
          destDataStore.fillTuple(destVoxelIdx, 0);
        }
      }
      else
      {
        destDataStore.fillTuple(destVoxelIdx, 0);
      }

      counter++;
      if(counter >= counterIncrement)
      {
        float progress = static_cast<float>(destVoxelIdx) / static_cast<float>(numVoxels) * 100.0f;
        m_AlgorithmPtr->sendThreadSafeProgressMessage(fmt::format("Resampling Data Array '{}' {:.0f}% Complete", m_DestArray.getName(), progress));
        counter = 0;
      }
    }
    m_AlgorithmPtr->sendThreadSafeProgressMessage(fmt::format("Resampling Data Array '{}' Complete", m_DestArray.getName()));
  }

private:
  ResampleImageGeom* m_AlgorithmPtr = nullptr;
  const IDataArray& m_SrcArray;
  IDataArray& m_DestArray;
  const ImageGeom& m_SrcImageGeom;
  const ImageGeom& m_DestImageGeom;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
ResampleImageGeom::ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
ResampleImageGeom::~ResampleImageGeom() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ResampleImageGeom::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ResampleImageGeom::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  m_ThrottledMessengerPtr = &throttledMessenger;

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SelectedImageGeometryPath);

  auto& destImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->CreatedImageGeometryPath);
  const auto& srcCellDataAM = selectedImageGeom.getCellDataRef();
  auto& destCellDataAM = destImageGeom.getCellDataRef();

  usize arrayIndex = 0;
  usize totalArrays = srcCellDataAM.getSize();

  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);

  for(const auto& [dataId, oldDataObject] : srcCellDataAM)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    arrayIndex++;
    const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
    const std::string srcName = oldDataArray.getName();
    auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(srcName));
    m_MessageHandler.sendInfoMessage(fmt::format("Resampling Data Array: '{}' ({}/{})", srcName, arrayIndex, totalArrays));

    ExecuteParallelFunction<ResampleImageGeomArrayImpl>(oldDataArray.getDataType(), taskRunner, this, oldDataArray, newDataArray, selectedImageGeom, destImageGeom, m_ShouldCancel);
  }

  taskRunner.wait(); // This will spill over if the number of geometries to process does not divide evenly by the number of threads.

  if(m_ShouldCancel)
  {
    return {};
  }

  // Careful with this next section. We purposefully copy in the original dataStructure arrays
  // into the destination feature attribute matrix so that we have somewhere to start.
  // During the renumbering phase is when those copied arrays will get potentially resized
  // to their proper number of tuples.
  DataPath cellFeatureAMPath = m_InputValues->CellFeatureAttributeMatrix;
  auto destImagePath = m_InputValues->CreatedImageGeometryPath;
  DataPath featureIdsArrayPath = m_InputValues->FeatureIdsArrayPath;

  if(m_InputValues->RenumberFeatures)
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
    for(size_t index = 0; index < sourceFeatureDataPaths.size(); index++)
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

  return {};
}

void ResampleImageGeom::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  if(nullptr != m_ThrottledMessengerPtr)
  {
    m_ThrottledMessengerPtr->sendThrottledMessage([&]() { return message; });
  }
}
