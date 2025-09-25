#include "SegmentFeatures.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
SegmentFeatures::SegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHelper(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SegmentFeatures::~SegmentFeatures() = default;

// -----------------------------------------------------------------------------
Result<> SegmentFeatures::execute(IGridGeometry* gridGeom)
{
  SizeVec3 udims = gridGeom->getDimensions();

  int64 dims[3] = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};

  // Initialize sequence of execution modifiers
  int32 gnum = 1;
  int64 nextSeed = 0;
  int64 seed = getSeed(gnum, nextSeed);
  usize size = 0;

  // Initialize calculation modifiers
  int64 neighbor = 0;
  bool good = false;
  bool hasNonContiguousFeature = false;
  int64 col = 0, row = 0, plane = 0;

  // Initialize containers
  usize initialVoxelsListSize = 100000;
  std::vector<int64_t> voxelsList(initialVoxelsListSize, -1);

  int64 neighPoints[6] = {0, 0, 0, 0, 0, 0};
  { // Initialize neighPoints in a readable fashion
    neighPoints[0] = -(dims[0] * dims[1]);
    neighPoints[1] = -dims[0];
    neighPoints[2] = -1;
    neighPoints[3] = 1;
    neighPoints[4] = dims[0];
    neighPoints[5] = (dims[0] * dims[1]);
  }
  int64 periodicPoints[6] = {0, 0, 0, 0, 0, 0};
  {
    periodicPoints[0] = (dims[0] * dims[1]) * (dims[2] - 1);
    periodicPoints[1] = dims[0] * (dims[1] - 1);
    periodicPoints[2] = dims[0] - 1;
    periodicPoints[3] = -dims[0] + 1;
    periodicPoints[4] = -(dims[0] * (dims[1] - 1));
    periodicPoints[5] = -(dims[0] * dims[1]) * (dims[2] - 1);
  }

  ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
  while(seed >= 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    size = 0;
    voxelsList[size] = seed;
    size++;
    while(size > 0)
    {
      int64 currentPoint = voxelsList[size - 1];
      size -= 1;
      col = currentPoint % dims[0];
      row = (currentPoint / dims[0]) % dims[1];
      plane = currentPoint / (dims[0] * dims[1]);
      for(int32 i = 0; i < 6; i++)
      {
        good = true;
        neighbor = currentPoint + neighPoints[i];
        if(i == 0 && plane == 0)
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(i == 5 && plane == (dims[2] - 1))
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(i == 1 && row == 0)
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(i == 4 && row == (dims[1] - 1))
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(i == 2 && col == 0)
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(i == 3 && col == (dims[0] - 1))
        {
          good = false;
          neighbor = currentPoint + periodicPoints[i];
        }
        if(good || m_IsPeriodic)
        {
          if(determineGrouping(currentPoint, neighbor, gnum))
          {
            voxelsList[size] = neighbor;
            size++;
            if(neighbor == nextSeed)
            {
              nextSeed = neighbor + 1;
            }
            if(size >= voxelsList.size())
            {
              size = voxelsList.size();
              voxelsList.resize(size + initialVoxelsListSize);
              for(std::vector<int64_t>::size_type j = size; j < voxelsList.size(); ++j)
              {
                voxelsList[j] = -1;
              }
            }
            if(!good)
            {
              hasNonContiguousFeature = true;
            }
          }
        }
      }
    }

    voxelsList.assign(initialVoxelsListSize, -1);
    gnum++;

    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Features Found: {}", gnum); });

    nextSeed = seed + 1;
    seed = getSeed(gnum, nextSeed);
  }

  if(hasNonContiguousFeature)
  {
    m_MessageHelper.sendMessage("SegmentFeatures found Non-Contiguous Features.");
  }

  m_MessageHelper.sendMessage(fmt::format("Total Features Found: {}", gnum));
  m_FoundFeatures = gnum;
  return {};
}

int64 SegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  return -1;
}

bool SegmentFeatures::determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const
{
  return false;
}

// -----------------------------------------------------------------------------
SegmentFeatures::SeedGenerator SegmentFeatures::initializeStaticVoxelSeedGenerator() const
{
  return SeedGenerator(SeedGenerator::default_seed);
}

// -----------------------------------------------------------------------------
void SegmentFeatures::randomizeFeatureIds(nx::core::Int32Array* featureIds, uint64 totalFeatures)
{
  m_MessageHelper.sendMessage("Randomizing Feature Ids");
  ClusterUtilities::RandomizeFeatureIds(featureIds->getDataStoreRef(), totalFeatures);
}
