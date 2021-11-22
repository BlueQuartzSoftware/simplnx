#include "SegmentFeatures.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
inline constexpr int32 k_MissingGeomError = -440;
}

std::string SegmentFeatures::name() const
{
  return FilterTraits<SegmentFeatures>::name;
}

std::string SegmentFeatures::className() const
{
  return FilterTraits<SegmentFeatures>::className;
}

Uuid SegmentFeatures::uuid() const
{
  return FilterTraits<SegmentFeatures>::uuid;
}

std::string SegmentFeatures::humanName() const
{
  return "Delete Data";
}

Parameters SegmentFeatures::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GridGeomPath_Key, "Grid Geometry", "DataPath to target Grid Geometry", DataPath{}));
  return params;
}

IFilter::UniquePointer SegmentFeatures::clone() const
{
  return std::make_unique<SegmentFeatures>();
}

IFilter::PreflightResult SegmentFeatures::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto gridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);

  if(data.getDataAs<AbstractGeometryGrid>(gridGeomPath) == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeomError, "A Grid Geometry is required for SegmentFeatures"}})};
  }

  

  OutputActions actions;
  return {std::move(actions)};
}

Result<> SegmentFeatures::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto gridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);

  auto gridGeom = data.getDataAs<AbstractGeometryGrid>(gridGeomPath);
  SizeVec3 udims = gridGeom->getDimensions();

  int64 dims[3] = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};

  int32 gnum = 1;
  int64 seed = 0;
  int64 neighbor = 0;
  bool good = false;
  int64 col = 0, row = 0, plane = 0;
  usize size = 0;
  usize initialVoxelsListSize = 100000;
  std::vector<int64_t> voxelslist(initialVoxelsListSize, -1);
  int64 neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -(dims[0] * dims[1]);
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = (dims[0] * dims[1]);
  int64 nextSeed = 0;

  while(seed >= 0)
  {
    seed = getSeed(data, args, gnum, nextSeed);
    nextSeed = seed + 1;
    if(seed >= 0)
    {
      size = 0;
      voxelslist[size] = seed;
      size++;
      while(size > 0)
      {
        int64 currentpoint = voxelslist[size - 1];
        size -= 1;
        col = currentpoint % dims[0];
        row = (currentpoint / dims[0]) % dims[1];
        plane = currentpoint / (dims[0] * dims[1]);
        for(int32 i = 0; i < 6; i++)
        {
          good = true;
          neighbor = currentpoint + neighpoints[i];
          if(i == 0 && plane == 0)
          {
            good = false;
          }
          if(i == 5 && plane == (dims[2] - 1))
          {
            good = false;
          }
          if(i == 1 && row == 0)
          {
            good = false;
          }
          if(i == 4 && row == (dims[1] - 1))
          {
            good = false;
          }
          if(i == 2 && col == 0)
          {
            good = false;
          }
          if(i == 3 && col == (dims[0] - 1))
          {
            good = false;
          }
          if(good)
          {
            if(determineGrouping(data, args, currentpoint, neighbor, gnum))
            {
              voxelslist[size] = neighbor;
              size++;
              if(neighbor == nextSeed)
              {
                nextSeed = neighbor + 1;
              }
              if(size >= voxelslist.size())
              {
                size = voxelslist.size();
                voxelslist.resize(size + initialVoxelsListSize);
                for(std::vector<int64_t>::size_type j = size; j < voxelslist.size(); ++j)
                {
                  voxelslist[j] = -1;
                }
              }
            }
          }
        }
      }

      voxelslist.assign(initialVoxelsListSize, -1);
      gnum++;
      if(gnum % 100 == 0)
      {
        // std::string ss = fmt::format("Total Features: {}", gnum);
      }
    }
  }

  return {};
}

int64 SegmentFeatures::getSeed(DataStructure& data, const Arguments& args, int32 gnum, int64 nextSeed) const
{
  return -1;
}

bool SegmentFeatures::determineGrouping(const DataStructure& data, const Arguments& args, int64 referencePoint, int64 neighborPoint, int32 gnum) const
{
  return false;
}
} // namespace complex
