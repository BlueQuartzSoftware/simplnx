#include "RemoveFlaggedFeaturesDirect.hpp"

#include "RemoveFlaggedFeatures.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief Selects one majority face-neighbor source for each nonpositive voxel.
 * @param imageGeom Defines voxel dimensions.
 * @param featureIds Provides current Feature IDs.
 * @param storageArray Receives flat source-voxel indexes.
 * @param shouldCancel Stops before later Z slices when true.
 * @param messageHelper Creates a throttled progress messenger.
 * @return True if any nonpositive Feature ID exists; false after cancellation or none.
 * @pre Flat voxel indexes fit in int32.
 */
bool IdentifyNeighbors(ImageGeom& imageGeom, Int32AbstractDataStore& featureIds, std::vector<int32>& storageArray, const std::atomic_bool& shouldCancel, MessageHelper& messageHelper)
{
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(uDims[0]),
      static_cast<int64>(uDims[1]),
      static_cast<int64>(uDims[2]),
  };

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  bool shouldLoop = false;

  auto progressIncrement = dims[2] / 100;
  usize progressCounter = 0;
  int32 featureName;
  int64 kStride, jStride;
  for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
  {
    if(shouldCancel)
    {
      return false;
    }

    if(progressCounter > progressIncrement)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing Image... {:.2f}%", CalculatePercentComplete(zIdx, dims[2])); });
      progressCounter = 0;
    }
    progressCounter++;

    kStride = dims[0] * dims[1] * zIdx;
    for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
    {
      jStride = dims[0] * yIdx;
      for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
      {
        int64 voxelIndex = kStride + jStride + xIdx;
        featureName = featureIds[voxelIndex];
        if(featureName > 0)
        {
          continue;
        }
        shouldLoop = true;
        int32 current;
        int32 most = 0;
        std::vector<int32> numHits(6, 0);
        std::vector<int32> discoveredFeatures = {};
        discoveredFeatures.reserve(6);
        // Check six face neighbors in the shared NeighborUtilities order.
        const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
        for(const auto& faceIndex : faceNeighborInternalIdx)
        {
          if(!isValidFaceNeighbor[faceIndex])
          {
            continue;
          }

          int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
          int32 feature = featureIds[neighborPoint];
          if(feature >= 0)
          {
            bool found = false;
            for(usize featIndex = 0; featIndex < discoveredFeatures.size(); featIndex++)
            {
              if(discoveredFeatures[featIndex] == feature)
              {
                found = true;
                numHits[featIndex]++;
                current = numHits[featIndex];
                if(current > most)
                {
                  most = current;
                  storageArray[voxelIndex] = static_cast<int32>(neighborPoint);
                }
                break;
              }
            }
            if(!found)
            {
              discoveredFeatures.push_back(feature);
            }
          }
        }
      }
    }
  }
  return shouldLoop;
}

/**
 * @brief Marks cells that belong to flagged features.
 * @param featureIds Provides and receives cell Feature IDs.
 * @param flaggedFeatures Identifies features selected for removal.
 * @param fillRemovedFeatures Uses -1 marks for later filling when true.
 * @return Active-feature flags, or an empty vector if removal selects all features.
 * @pre Feature IDs are nonnegative and less than the feature-flag tuple count.
 */
std::vector<bool> FlagFeatures(Int32AbstractDataStore& featureIds, std::unique_ptr<MaskCompareUtilities::MaskCompare>& flaggedFeatures, const bool fillRemovedFeatures)
{
  bool good = false;
  usize totalPoints = featureIds.getNumberOfTuples();
  usize totalFeatures = flaggedFeatures->getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, true);
  for(usize i = 1; i < totalFeatures; i++)
  {
    if(!flaggedFeatures->isTrue(i))
    {
      good = true;
    }
    else
    {
      activeObjects[i] = false;
    }
  }
  if(!good)
  {
    return {};
  }
  for(usize i = 0; i < totalPoints; i++)
  {
    if(activeObjects[featureIds[i]])
    {
      continue;
    }

    if(fillRemovedFeatures)
    {
      featureIds[i] = -1;
    }
    else
    {
      featureIds[i] = 0;
    }
  }
  return activeObjects;
}

/**
 * @brief Copies companion tuples from selected resident source neighbors.
 * @param featureIds Provides current Feature IDs.
 * @param neighbors Provides one flat source index per destination cell.
 * @param voxelArrays Receives source tuples for negative Feature IDs.
 * @param shouldCancel Stops before later destination cells when true.
 */
void FindVoxelArrays(const Int32AbstractDataStore& featureIds, const std::vector<int32>& neighbors, std::vector<std::shared_ptr<IDataArray>>& voxelArrays, const std::atomic_bool& shouldCancel)
{
  const usize totalPoints = featureIds.getNumberOfTuples();

  int32 featureName, neighbor;
  for(usize j = 0; j < totalPoints; j++)
  {
    if(shouldCancel)
    {
      return;
    }

    featureName = featureIds[j];
    neighbor = neighbors[j];
    if(neighbor >= 0)
    {
      if(featureName < 0 && featureIds[neighbor] >= 0)
      {
        for(const auto& voxelArray : voxelArrays)
        {
          voxelArray->copyTuple(neighbor, j);
        }
      }
    }
  }
}

/**
 * @class RunCropImageGeometryImpl
 * @brief Runs one CropImageGeometryFilter task for a flagged feature.
 *
 * The caller executes each task synchronously. This protects DataStructure mutation
 * and keeps borrowed loop-local paths and bounds alive.
 */
class RunCropImageGeometryImpl
{
public:
  /**
   * @brief Creates one borrowed crop task.
   * @param dataStructure Receives the cropped geometry.
   * @param shouldCancel Stops before delegated execution when true.
   * @param imageGeometryPath Identifies the source ImageGeom.
   * @param minVoxelVector Specifies inclusive minimum voxel indexes.
   * @param maxVoxelVector Specifies inclusive maximum voxel indexes.
   * @param createdImgGeomPath Identifies the cropped ImageGeom.
   */
  RunCropImageGeometryImpl(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const DataPath& imageGeometryPath, const std::vector<uint64>& minVoxelVector,
                           const std::vector<uint64>& maxVoxelVector, const DataPath& createdImgGeomPath)
  : m_DataStructure(dataStructure)
  , m_ShouldCancel(shouldCancel)
  , m_ImageGeometryPath(imageGeometryPath)
  , m_MinVoxelVector(minVoxelVector)
  , m_MaxVoxelVector(maxVoxelVector)
  , m_CreatedImgGeomPath(createdImgGeomPath)
  {
  }

  /**
   * @brief Destroys the borrowed crop task.
   */
  ~RunCropImageGeometryImpl() = default;

  /**
   * @brief Preflights and executes the delegated crop.
   *
   * The task throws after a preflight failure. The implementation does not inspect
   * the execute result and can therefore ignore an execution failure.
   */
  void operator()() const
  {
    CropImageGeometryFilter filter;

    Arguments args;

    args.insertOrAssign(CropImageGeometryFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_ImageGeometryPath));
    args.insertOrAssign(CropImageGeometryFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometryFilter::k_UsePhysicalBounds_Key, std::make_any<bool>(false));

    args.insertOrAssign(CropImageGeometryFilter::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(m_MinVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(m_MaxVoxelVector));
    args.insertOrAssign(CropImageGeometryFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(m_CreatedImgGeomPath));

    auto preflightResult = filter.preflight(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      throw std::runtime_error("Preflight failed when cropping the geometry in extract flagged features!");
    }

    if(m_ShouldCancel)
    {
      return;
    }

    auto executeResult = filter.execute(m_DataStructure, args);
    if(preflightResult.outputActions.invalid())
    {
      throw std::runtime_error("Execute failed when cropping the geometry in extract flagged features!");
    }
  }

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const DataPath& m_ImageGeometryPath;
  const std::vector<uint64>& m_MinVoxelVector;
  const std::vector<uint64>& m_MaxVoxelVector;
  const DataPath& m_CreatedImgGeomPath;
};
} // namespace

RemoveFlaggedFeaturesDirect::RemoveFlaggedFeaturesDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         const RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

RemoveFlaggedFeaturesDirect::~RemoveFlaggedFeaturesDirect() noexcept = default;

Result<> RemoveFlaggedFeaturesDirect::operator()()
{
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto function = static_cast<Functionality>(m_InputValues->ExtractFeatures);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> flaggedFeatures = nullptr;
  try
  {
    flaggedFeatures = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->FlaggedFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // Normal filter execution validates this path. Direct algorithm callers can
    // still supply a missing or unsupported mask.
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->FlaggedFeaturesArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  MessageHelper messageHelper(m_MessageHandler);

  // Extract and ExtractThenRemove create one cropped geometry per flagged feature.
  if(function != Functionality::Remove)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Extraction")});

    {
      ComputeFeatureRectFilter filter;
      Arguments args;

      args.insert(ComputeFeatureRectFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(m_InputValues->FeatureIdsArrayPath));
      args.insert(ComputeFeatureRectFilter::k_FeatureDataAttributeMatrixPath_Key, std::make_any<DataPath>(m_InputValues->TempBoundsPath.getParent()));
      args.insert(ComputeFeatureRectFilter::k_FeatureRectArrayName_Key, std::make_any<std::string>(m_InputValues->TempBoundsPath.getTargetName()));

      auto preflightResult = filter.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        throw std::runtime_error("Preflight failed when cropping the geometry in extract flagged features!");
      }

      if(m_ShouldCancel)
      {
        return {};
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      // Only preflight status controls this path. The delegated execute result is not inspected.
      if(preflightResult.outputActions.invalid())
      {
        throw std::runtime_error("Execute failed when cropping the geometry in extract flagged features!");
      }
    }

    auto bounds = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath);

    if(m_ShouldCancel)
    {
      return {};
    }

    ParallelTaskAlgorithm taskRunner;
    // Crop tasks mutate DataStructure and borrow loop-local bounds. Synchronous
    // execution satisfies both thread-safety and lifetime requirements.
    taskRunner.setParallelizationEnabled(false);

    usize maxTuple = flaggedFeatures->getNumberOfTuples();
    std::string paddingWidth = std::to_string(std::to_string(maxTuple).size());
    for(usize i = 1; i < maxTuple; i++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      if(!flaggedFeatures->isTrue(i))
      {
        continue;
      }

      usize index = 6 * i;
      std::vector<uint64> minVoxels = {static_cast<uint64>(bounds[index]), static_cast<uint64>(bounds[index + 1]), static_cast<uint64>(bounds[index + 2])};
      std::vector<uint64> maxVoxels = {static_cast<uint64>(bounds[index + 3]), static_cast<uint64>(bounds[index + 4]), static_cast<uint64>(bounds[index + 5])};

      DataPath createdImgGeomPath({fmt::format(fmt::runtime("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      taskRunner.execute(RunCropImageGeometryImpl(m_DataStructure, m_ShouldCancel, m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath));
    }
    taskRunner.wait();

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // Remove and ExtractThenRemove modify the source feature data.
  if(function != Functionality::Extract)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

    std::vector<int32> neighbors((featureIds.getNumberOfTuples() * featureIds.getNumberOfComponents()), -1);
    std::vector<bool> activeObjects = FlagFeatures(featureIds, flaggedFeatures, m_InputValues->FillRemovedFeatures);
    if(activeObjects.empty())
    {
      return MakeErrorResult(-45433, "All Features were flagged and would all be removed. The filter has quit.");
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    if(m_InputValues->FillRemovedFeatures)
    {
      bool shouldLoop;
      usize count = 0;
      do
      {
        count++;
        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});
        std::fill(neighbors.begin(), neighbors.end(), -1);
        shouldLoop = IdentifyNeighbors(imageGeom, featureIds, neighbors, m_ShouldCancel, messageHelper);

        if(m_ShouldCancel)
        {
          return {};
        }

        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Filling bad voxels...")});
        std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
        FindVoxelArrays(featureIds, neighbors, voxelArrays, m_ShouldCancel);
      } while(shouldLoop);
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
    DataPath featureGroupPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    if(!RemoveInactiveObjects(m_DataStructure, featureGroupPath, activeObjects, featureIds, flaggedFeatures->getNumberOfTuples(), m_MessageHandler, m_ShouldCancel))
    {
      return MakeErrorResult(-45434, fmt::format("Failed to remove inactive objects from feature group at path '{}'.", featureGroupPath.toString()));
    }
  }

  return {};
}
