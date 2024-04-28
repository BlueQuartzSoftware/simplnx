#include "RemoveFlaggedFeatures.hpp"

#include "SimplnxCore/Filters/ComputeFeatureRectFilter.hpp"
#include "SimplnxCore/Filters/CropImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
bool IdentifyNeighbors(ImageGeom& imageGeom, Int32Array& featureIds, std::vector<int32>& storageArray, const std::atomic_bool& shouldCancel, RemoveFlaggedFeatures* filter)
{
  const usize totalPoints = featureIds.getNumberOfTuples();
  SizeVec3 uDims = imageGeom.getDimensions();

  int64 dims[3] = {static_cast<int64>(uDims[0]), static_cast<int64>(uDims[1]), static_cast<int64>(uDims[2])};

  const int64 neighborPoints[6] = {(-dims[0] * dims[1]), (-dims[0]), -1, 1, dims[0], (dims[0] * dims[1])};

  bool shouldLoop = false;

  auto progressIncrement = dims[2] / 100;
  usize progressCounter = 0;
  int32 featureName;
  int64 kStride = 0, jStride = 0;
  for(int64 k = 0; k < dims[2]; k++)
  {
    if(shouldCancel)
    {
      return false;
    }

    if(progressCounter > progressIncrement)
    {
      filter->sendThreadSafeProgressMessage(progressCounter);
      progressCounter = 0;
    }
    progressCounter++;

    kStride = dims[0] * dims[1] * k;
    for(int64 j = 0; j < dims[1]; j++)
    {
      jStride = dims[0] * j;
      for(int64 i = 0; i < dims[0]; i++)
      {
        int64 count = kStride + jStride + i;
        featureName = featureIds[count];
        if(featureName > 0)
        {
          continue;
        }
        shouldLoop = true;
        int32 neighbor;
        int32 current = 0;
        int32 most = 0;
        std::vector<int32> numHits(6, 0);
        std::vector<int32> discoveredFeatures = {};
        discoveredFeatures.reserve(6);
        for(int8 l = 0; l < 6; l++)
        {
          if(l == 5 && k == (dims[2] - 1))
          {
            continue;
          }
          if(l == 1 && j == 0)
          {
            continue;
          }
          if(l == 4 && j == (dims[1] - 1))
          {
            continue;
          }
          if(l == 2 && i == 0)
          {
            continue;
          }
          if(l == 3 && i == (dims[0] - 1))
          {
            continue;
          }
          if(l == 0 && k == 0)
          {
            continue;
          }

          int64 neighborPoint = count + neighborPoints[l];
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
                  storageArray[count] = static_cast<int32>(neighborPoint);
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

std::vector<bool> FlagFeatures(Int32Array& featureIds, std::unique_ptr<MaskCompare>& flaggedFeatures, const bool fillRemovedFeatures)
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

void FindVoxelArrays(const Int32Array& featureIds, const std::vector<int32>& neighbors, std::vector<std::shared_ptr<IDataArray>>& voxelArrays, const std::atomic_bool& shouldCancel)
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

class RunCropImageGeometryImpl
{
public:
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

  ~RunCropImageGeometryImpl() = default;

  void operator()() const
  {
    CropImageGeometry filter;

    Arguments args;

    args.insertOrAssign(CropImageGeometry::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(CropImageGeometry::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(m_ImageGeometryPath));
    args.insertOrAssign(CropImageGeometry::k_RenumberFeatures_Key, std::make_any<bool>(false));

    args.insertOrAssign(CropImageGeometry::k_MinVoxel_Key, std::make_any<std::vector<uint64>>(m_MinVoxelVector));
    args.insertOrAssign(CropImageGeometry::k_MaxVoxel_Key, std::make_any<std::vector<uint64>>(m_MaxVoxelVector));
    args.insertOrAssign(CropImageGeometry::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(m_CreatedImgGeomPath));

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

// -----------------------------------------------------------------------------
RemoveFlaggedFeatures::RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RemoveFlaggedFeatures::~RemoveFlaggedFeatures() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& RemoveFlaggedFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void RemoveFlaggedFeatures::sendThreadSafeProgressMessage(size_t counter)
{
  /* This filter is does not currently use multithreading so lock is unnecessary.
   * If this fact changes one MUST UNCOMMENT the below line! */
  // std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);

  m_ProgressCounter += counter;

  auto now = std::chrono::steady_clock::now();
  if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update
  {
    auto progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / static_cast<double>(m_TotalElements)) * 100.0);
    std::string progressMessage = "Processing Image... ";
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});
    m_InitialTime = std::chrono::steady_clock::now();
  }
}

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedFeatures::operator()()
{
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto function = static_cast<Functionality>(m_InputValues->ExtractFeatures);

  std::unique_ptr<MaskCompare> flaggedFeatures = nullptr;
  try
  {
    flaggedFeatures = InstantiateMaskCompare(m_DataStructure, m_InputValues->FlaggedFeaturesArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->FlaggedFeaturesArrayPath.toString());
    return MakeErrorResult(-53900, message);
  }

  if(getCancel())
  {
    return {};
  }

  // Valid values Functionality::Extract and Functionality::ExtractThenRemove
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

      if(getCancel())
      {
        return {};
      }

      auto executeResult = filter.execute(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        throw std::runtime_error("Execute failed when cropping the geometry in extract flagged features!");
      }
    }

    auto bounds = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->TempBoundsPath);

    if(getCancel())
    {
      return {};
    }

    ParallelTaskAlgorithm taskRunner;
    // This has to be run in serial for the time being because adding to the dataStructure is not thread-safe
    taskRunner.setParallelizationEnabled(false);

    usize maxTuple = flaggedFeatures->getNumberOfTuples();
    std::string paddingWidth = std::to_string(std::to_string(maxTuple).size());
    for(usize i = 1; i < maxTuple; i++)
    {
      if(getCancel())
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

      DataPath createdImgGeomPath({fmt::format(("{}-{:0" + paddingWidth + "d}"), m_InputValues->CreatedImageGeometryPrefix, i)});

      m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Now Extracting Feature {}", i)});
      taskRunner.execute(RunCropImageGeometryImpl(m_DataStructure, getCancel(), m_InputValues->ImageGeometryPath, minVoxels, maxVoxels, createdImgGeomPath));
    }
    taskRunner.wait();

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("All Features Successfully Extracted")});
  }

  if(getCancel())
  {
    return {};
  }

  // Valid values Functionality::Remove and Functionality::ExtractThenRemove
  if(function != Functionality::Extract)
  {
    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Beginning Feature Removal")});

    std::vector<int32> neighbors((featureIds.getNumberOfTuples() * featureIds.getNumberOfComponents()), -1);
    std::vector<bool> activeObjects = FlagFeatures(featureIds, flaggedFeatures, m_InputValues->FillRemovedFeatures);
    if(activeObjects.empty())
    {
      return MakeErrorResult(-45433, "All Features were flagged and would all be removed. The filter has quit.");
    }

    if(getCancel())
    {
      return {};
    }

    if(m_InputValues->FillRemovedFeatures)
    {
      bool shouldLoop = false;
      usize count = 0;
      do
      {
        count++;
        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Entering iteration number {}...", count)});
        std::fill(neighbors.begin(), neighbors.end(), -1);
        shouldLoop = IdentifyNeighbors(imageGeom, featureIds, neighbors, getCancel(), this);

        if(getCancel())
        {
          return {};
        }

        m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Filling bad voxels...")});
        std::vector<std::shared_ptr<IDataArray>> voxelArrays = GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);
        FindVoxelArrays(featureIds, neighbors, voxelArrays, getCancel());
      } while(shouldLoop);
    }

    if(getCancel())
    {
      return {};
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Info, fmt::format("Stripping excess inactive objects from model...")});
    DataPath featureGroupPath = m_InputValues->FlaggedFeaturesArrayPath.getParent();
    if(!RemoveInactiveObjects(m_DataStructure, featureGroupPath, activeObjects, featureIds, flaggedFeatures->getNumberOfTuples(), m_MessageHandler))
    {
      return MakeErrorResult(-45434, "The removal has failed!");
    }
  }

  return {};
}
