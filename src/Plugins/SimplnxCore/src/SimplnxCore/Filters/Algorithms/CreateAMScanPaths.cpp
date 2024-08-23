#include "CreateAMScanPaths.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

using namespace nx::core;

namespace
{
std::array<float32, 9> ax2om(const std::array<float32, 4>& a)
{
  std::array<float32, 9> res = {};
  float32 q = 0.0L;
  float32 c = 0.0L;
  float32 s = 0.0L;
  float32 omc = 0.0L;

  c = cos(a[3]);
  s = sin(a[3]);

  omc = static_cast<float32>(1.0 - c);

  res[0] = a[0] * a[0] * omc + c;
  res[4] = a[1] * a[1] * omc + c;
  res[8] = a[2] * a[2] * omc + c;
  int _01 = 1;
  int _10 = 3;
  int _12 = 5;
  int _21 = 7;
  int _02 = 2;
  int _20 = 6;
  // Check to see if we need to transpose
  // if(Rotations::Constants::epsijk == 1.0L)
  {
    _01 = 3;
    _10 = 1;
    _12 = 7;
    _21 = 5;
    _02 = 6;
    _20 = 2;
  }

  q = omc * a[0] * a[1];
  res[_01] = q + s * a[2];
  res[_10] = q - s * a[2];
  q = omc * a[1] * a[2];
  res[_12] = q + s * a[0];
  res[_21] = q - s * a[0];
  q = omc * a[2] * a[0];
  res[_02] = q - s * a[1];
  res[_20] = q + s * a[1];

  return res;
}

ImageRotationUtilities::Matrix3fR toGMatrix(std::array<float32, 9> om)
{
  ImageRotationUtilities::Matrix3fR g;
  g(0, 0) = om[0];
  g(0, 1) = om[1];
  g(0, 2) = om[2];
  g(1, 0) = om[3];
  g(1, 1) = om[4];
  g(1, 2) = om[5];
  g(2, 0) = om[6];
  g(2, 1) = om[7];
  g(2, 2) = om[8];
  return g;
}

// -----------------------------------------------------------------------------
char determineIntersectCoord(const std::array<float32, 2>& p1, const std::array<float32, 2>& q1, const std::array<float32, 2>& p2, const std::array<float32, 2>& q2, float32& coordX)
{
  // assumes p1q1 is the hatch vector and p2q2 is the CAD edge
  // also assumes the p1q1 is in x direction only so can just check y coords for potential intersection
  float32 x1 = p1[0];
  float32 x2 = q1[0];
  float32 x3 = p2[0];
  float32 x4 = q2[0];
  float32 y1 = p1[1];
  //  float32 y2 = q1[1];
  float32 y3 = p2[1];
  float32 y4 = q2[1];

  if(y3 > y1 && y4 > y1)
  {
    return 'n';
  }
  if(y3 < y1 && y4 < y1)
  {
    return 'n';
  }
  if(y3 == y1 && y4 == y1)
  {
    return 'n';
  }
  if(y3 == y1)
  {
    coordX = x3;
    if(x3 >= x1 && x3 <= x2)
    {
      return 'c';
    }
    return 'n';
  }
  if(y4 == y1)
  {
    coordX = x4;
    if(x4 >= x1 && x4 <= x2)
    {
      return 'd';
    }
    return 'n';
  }
  float32 frac = (y1 - y3) / (y4 - y3);
  coordX = x3 + (frac * (x4 - x3));
  if(coordX >= x1 && coordX <= x2)
  {
    return 'i';
  }
  return 'n';
}
} // namespace

// -----------------------------------------------------------------------------
CreateAMScanPaths::CreateAMScanPaths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAMScanPathsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateAMScanPaths::~CreateAMScanPaths() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CreateAMScanPaths::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CreateAMScanPaths::operator()()
{
  // find number of CAD slices and regions
  auto& CADLayers = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->CADSliceDataContainerName);
  INodeGeometry1D::SharedEdgeList& CADLayerEdges = CADLayers.getEdgesRef();
  INodeGeometry0D::SharedVertexList& CADLayerVerts = CADLayers.getVerticesRef();
  auto& cadSliceIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADSliceIdsArrayPath)->getDataStoreRef();
  auto& cadRegionIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CADRegionIdsArrayPath)->getDataStoreRef();
  usize numCADLayerEdges = CADLayers.getNumberOfEdges();
  usize numCADLayerVerts = CADLayers.getNumberOfVertices();
  int32 numCADLayers = 0;
  int32 numCADRegions = 0;
  for(usize i = 0; i < numCADLayerEdges; i++)
  {
    int32 layer = cadSliceIds[i];
    int32 region = cadRegionIds[i];
    if(numCADLayers < layer)
    {
      numCADLayers = layer;
    }
    if(numCADRegions < region)
    {
      numCADRegions = region;
    }
  }
  numCADLayers += 1;
  numCADRegions += 1;

  // get CAD slice heights and find list of edges for each region on each slice
  std::vector<usize> cDims(1, 2);
  std::vector<std::vector<std::vector<int32>>> regionEdgeLists(numCADLayers);
  for(int32 i = 0; i < numCADLayers; i++)
  {
    regionEdgeLists[i].resize(numCADRegions);
  }
  std::vector<float32> CADLayerHeights(numCADLayers);
  for(usize i = 0; i < numCADLayerEdges; i++)
  {
    int32 layer = cadSliceIds[i];
    int32 region = cadRegionIds[i];
    regionEdgeLists[layer][region].push_back(i);
    CADLayerHeights[layer] = CADLayerVerts[3 * CADLayerEdges[2 * i] + 2];
  }
  if(getCancel())
  {
    return {};
  }
  // set rotations for each layer, determine number of stripes possible on each layer and determine minimum and maximum coordinates
  ImageRotationUtilities::Matrix3fR rotMat;
  rotMat.fill(0.0f);
  rotMat(0, 0) = 1.0f;
  rotMat(1, 1) = 1.0f;
  rotMat(2, 2) = 1.0f;
  Eigen::Vector3f coords = {0.0f, 0.0f, 0.0f};
  Eigen::Vector3f newcoords = {0.0f, 0.0f, 0.0f};
  //  usize numPossibleStripes = 0;
  std::vector<float32> rotationAngles(numCADLayers, 0.0f);
  std::vector<std::vector<std::vector<float32>>> boundingBoxes(numCADLayers);
  for(int32 i = 0; i < numCADLayers; i++)
  {
    boundingBoxes[i].resize(numCADRegions);
    for(int32 j = 0; j < numCADRegions; j++)
    {
      boundingBoxes[i][j].resize(4, 0.0);
      if(!regionEdgeLists[i][j].empty())
      {
        boundingBoxes[i][j][0] = std::numeric_limits<float32>::max();
        boundingBoxes[i][j][1] = std::numeric_limits<float32>::max();
        boundingBoxes[i][j][2] = -std::numeric_limits<float32>::max();
        boundingBoxes[i][j][3] = -std::numeric_limits<float32>::max();
      }
    }
    rotationAngles[i] = static_cast<float32>(i) * 67.0f * Constants::k_PiOver180D;
  }
  if(getCancel())
  {
    return {};
  }
  // rotate CAD slices so hatching direction will always be 100
  std::vector<bool> rotatedPtIds(numCADLayerVerts, false);
  for(int64 i = 0; i < numCADLayerEdges; i++)
  {
    int32 curLayer = cadSliceIds[i];
    int32 curRegion = cadRegionIds[i];
    if(curLayer < numCADLayers)
    {
      float32 angle = rotationAngles[curLayer];
      std::array<float32, 4> ax = {0.0f, 0.0f, 1.0f, -angle};
      std::array<float32, 9> om = ax2om(ax);
      rotMat = toGMatrix(om);
      for(int j = 0; j < 2; j++)
      {
        usize vertIndex = 2 * i + j;
        usize vert = CADLayerEdges[vertIndex];
        if(rotatedPtIds[vert])
        {
          continue;
        }
        coords[0] = CADLayerVerts[3 * vert];
        coords[1] = CADLayerVerts[3 * vert + 1];
        coords[2] = CADLayerVerts[3 * vert + 2];
        newcoords = rotMat * coords;
        CADLayerVerts[3 * vert] = newcoords[0];
        CADLayerVerts[3 * vert + 1] = newcoords[1];
        CADLayerVerts[3 * vert + 2] = newcoords[2];
        rotatedPtIds[vert] = true;
        if(boundingBoxes[curLayer][curRegion][0] > newcoords[0])
        {
          boundingBoxes[curLayer][curRegion][0] = newcoords[0];
        }
        if(boundingBoxes[curLayer][curRegion][1] > newcoords[1])
        {
          boundingBoxes[curLayer][curRegion][1] = newcoords[1];
        }
        if(boundingBoxes[curLayer][curRegion][2] < newcoords[0])
        {
          boundingBoxes[curLayer][curRegion][2] = newcoords[0];
        }
        if(boundingBoxes[curLayer][curRegion][3] < newcoords[1])
        {
          boundingBoxes[curLayer][curRegion][3] = newcoords[1];
        }
      }
    }
  }

  // determine hatch positions
  // create points to use for checking if hatch endpoints are inside of objects
  std::array<float32, 2> p1 = {0.0f, 0.0f};
  std::array<float32, 2> p2 = {0.0f, 0.0f};

  std::map<float32, int, std::less<float32>> intersectionCoords;
  //  int32 headIntersectionCount;
  //  int32 tailIntersectionCount;
  //  bool headIsIn = false;
  //  bool tailIsIn = false;
  usize hatchCount = 0;
  //  float32 oneOverHatchSpacing = 1.0f / m_InputValues->HatchSpacing;

  if(getCancel())
  {
    return {};
  }

  float32 coord;

  float32 stripeDir = m_InputValues->HatchSpacing;
  float32 hatchDir = 1.0;
  float32 xCoord1, xCoord2, yCoord;
  float64 timeBase = 0.0;
  //  float64 hatchTime = m_StripeWidth / m_Speed;
  float64 turnTime = 0.0005;

  std::vector<float64> vertCoords(60000000, 0.0);
  std::vector<int32> edgeVertIds(20000000, 0);
  std::vector<int32> edgeSliceIds(10000000, 0);
  std::vector<int32> edgeRegionIds(10000000, 0);
  std::vector<float32> edgePowers(10000000, 0);
  std::vector<float64> vertTimes(20000000, 0);
  usize numLayers = boundingBoxes.size();
  for(usize i = 0; i < numLayers; i++)
  {
    usize layer = i;
    float32 zCoord = CADLayerHeights[layer];
    timeBase += 10.0;
    usize numRegions = boundingBoxes[i].size();
    for(usize j = 0; j < numRegions; j++)
    {
      usize region = j;
      std::vector<int> curEdgeList = regionEdgeLists[layer][region];
      usize mSize = curEdgeList.size();
      if(mSize > 0)
      {
        float32 minX = (floor(boundingBoxes[i][j][0] / m_InputValues->StripeWidth) - 1) * m_InputValues->StripeWidth;
        float32 maxX = (ceil(boundingBoxes[i][j][2] / m_InputValues->StripeWidth) + 1) * m_InputValues->StripeWidth;
        float32 minY = (floor(boundingBoxes[i][j][1] / m_InputValues->HatchSpacing) - 1) * m_InputValues->HatchSpacing;
        float32 maxY = (ceil(boundingBoxes[i][j][3] / m_InputValues->HatchSpacing) + 1) * m_InputValues->HatchSpacing;
        xCoord1 = minX;
        yCoord = minY;
        while(xCoord1 < maxX)
        {
          xCoord2 = xCoord1 + m_InputValues->StripeWidth;
          while(yCoord >= minY && yCoord <= maxY)
          {
            intersectionCoords.clear();
            p1[0] = xCoord1 - 1000;
            p1[1] = yCoord;
            p2[0] = xCoord2 + 1000;
            p2[1] = yCoord;
            int* ptr = curEdgeList.data();
            for(usize m = 0; m < mSize; m++)
            {
              usize curEdge = *ptr;
              usize CADvert1 = CADLayerEdges[2 * curEdge];
              usize CADvert2 = CADLayerEdges[2 * curEdge + 1];
              usize vertOffset1 = 3 * CADvert1;
              usize vertOffset2 = 3 * CADvert2;
              std::array<float32, 2> vert1 = {CADLayerVerts[vertOffset1], CADLayerVerts[vertOffset1 + 1]};
              std::array<float32, 2> vert2 = {CADLayerVerts[vertOffset2], CADLayerVerts[vertOffset2 + 1]};
              char good = determineIntersectCoord(p1, p2, vert1, vert2, coord);
              if(good == 'i')
              {
                intersectionCoords.insert(std::pair<float, usize>(coord, 0));
              }
              else if(good == 'c' || good == 'd')
              {
                int hitType = 1;
                if(vert1[1] >= yCoord && vert2[1] >= yCoord)
                {
                  hitType = 1;
                }
                else if(vert1[1] <= yCoord && vert2[1] <= yCoord)
                {
                  hitType = -1;
                }
                auto it = intersectionCoords.find(coord);
                if(it == intersectionCoords.end())
                {
                  intersectionCoords.insert(std::pair<float32, int>(coord, hitType));
                }
                else
                {
                  int curHitType = it->second;
                  if(curHitType != hitType)
                  {
                    it->second = 0;
                  }
                  else
                  {
                    intersectionCoords.erase(coord);
                  }
                }
              }
              ptr++;
            }
            int32 intersectionSize = intersectionCoords.size();
            if(intersectionSize > 1)
            {
              bool addTurnTime = false;
              for(auto it = intersectionCoords.begin(); it != intersectionCoords.end(); ++it)
              {
                int hitType = it->second;
                float32 val = it->first;
                auto itNext = std::next(it);
                if(itNext != intersectionCoords.end())
                {
                  float32 nextVal = itNext->first;
                  if(hitType != 0)
                  {
                    if(hatchDir > 0)
                    {
                      intersectionCoords.erase(val);
                      itNext->second = 0;
                    }
                    else
                    {
                      intersectionCoords.erase(nextVal);
                      it->second = 0;
                    }
                    it = intersectionCoords.begin();
                  }
                }
              }
              if(hatchDir > 0)
              {
                usize count = 0;
                usize size = intersectionCoords.size();
                std::vector<float32> coordVector(size);
                for(auto it = intersectionCoords.begin(); it != intersectionCoords.end(); ++it)
                {
                  coordVector[count] = it->first;
                  count++;
                }
                float32 lastPos = coordVector[0];
                for(usize k = 0; k < size; k += 2)
                {
                  float32 x1 = coordVector[k];
                  float32 x2 = coordVector[k + 1];
                  if(x2 <= xCoord1)
                  {
                    continue;
                  }
                  if(x1 >= xCoord2)
                  {
                    continue;
                  }
                  if(x1 < xCoord1)
                  {
                    x1 = xCoord1;
                    if(k == 0)
                    {
                      lastPos = x1;
                    }
                  }
                  if(x2 > xCoord2)
                  {
                    x2 = xCoord2;
                  }
                  vertCoords[3 * (2 * hatchCount)] = x1;
                  vertCoords[3 * (2 * hatchCount) + 1] = yCoord;
                  vertCoords[3 * (2 * hatchCount) + 2] = zCoord;
                  vertCoords[3 * (2 * hatchCount + 1)] = x2;
                  vertCoords[3 * (2 * hatchCount + 1) + 1] = yCoord;
                  vertCoords[3 * (2 * hatchCount + 1) + 2] = zCoord;
                  edgeVertIds[2 * hatchCount] = 2 * hatchCount;
                  edgeVertIds[2 * hatchCount + 1] = 2 * hatchCount + 1;
                  edgeSliceIds[hatchCount] = layer;
                  edgeRegionIds[hatchCount] = region;
                  edgePowers[hatchCount] = m_InputValues->Power;
                  timeBase += (abs(x1 - lastPos) / m_InputValues->Speed);
                  lastPos = x1;
                  vertTimes[2 * hatchCount] = timeBase;
                  timeBase += (abs(x2 - lastPos) / m_InputValues->Speed);
                  lastPos = x2;
                  vertTimes[2 * hatchCount + 1] = timeBase;
                  hatchCount++;
                  addTurnTime = true;
                  if(6 * hatchCount >= vertCoords.size())
                  {
                    int64_t newSize = hatchCount + 1000000;
                    vertCoords.resize(newSize * 6);
                    edgeVertIds.resize(newSize * 2);
                    edgeSliceIds.resize(newSize);
                    edgeRegionIds.resize(newSize);
                    edgePowers.resize(newSize);
                    vertTimes.resize(newSize * 2);
                  }
                }
              }
              else
              {
                usize count = 0;
                usize size = intersectionCoords.size();
                std::vector<float32> coordVector(size);
                for(auto it = intersectionCoords.begin(); it != intersectionCoords.end(); ++it)
                {
                  coordVector[count] = it->first;
                  count++;
                }
                float32 lastPos = coordVector[size - 1];
                for(int64 k = size - 1; k > 0; k -= 2)
                {
                  float32 x1 = coordVector[k];
                  float32 x2 = coordVector[k - 1];
                  if(x2 >= xCoord2)
                  {
                    continue;
                  }
                  if(x1 <= xCoord1)
                  {
                    continue;
                  }
                  if(x1 > xCoord2)
                  {
                    x1 = xCoord2;
                    if(k == 0)
                    {
                      lastPos = x1;
                    }
                  }
                  if(x2 < xCoord1)
                  {
                    x2 = xCoord1;
                  }
                  vertCoords[3 * (2 * hatchCount)] = x1;
                  vertCoords[3 * (2 * hatchCount) + 1] = yCoord;
                  vertCoords[3 * (2 * hatchCount) + 2] = zCoord;
                  vertCoords[3 * (2 * hatchCount + 1)] = x2;
                  vertCoords[3 * (2 * hatchCount + 1) + 1] = yCoord;
                  vertCoords[3 * (2 * hatchCount + 1) + 2] = zCoord;
                  edgeVertIds[2 * hatchCount] = 2 * hatchCount;
                  edgeVertIds[2 * hatchCount + 1] = 2 * hatchCount + 1;
                  edgeSliceIds[hatchCount] = layer;
                  edgeRegionIds[hatchCount] = region;
                  edgePowers[hatchCount] = m_InputValues->Power;
                  timeBase += (abs(x1 - lastPos) / m_InputValues->Speed);
                  lastPos = x1;
                  vertTimes[2 * hatchCount] = timeBase;
                  timeBase += (abs(x2 - lastPos) / m_InputValues->Speed);
                  lastPos = x2;
                  vertTimes[2 * hatchCount + 1] = timeBase;
                  hatchCount++;
                  addTurnTime = true;
                  if(6 * hatchCount >= vertCoords.size())
                  {
                    int64 newSize = hatchCount + 1000000;
                    vertCoords.resize(newSize * 6);
                    edgeVertIds.resize(newSize * 2);
                    edgeSliceIds.resize(newSize);
                    edgeRegionIds.resize(newSize);
                    edgePowers.resize(newSize);
                    vertTimes.resize(newSize * 2);
                  }
                }
              }
              if(addTurnTime)
              {
                timeBase += turnTime;
              }
            }
            yCoord += stripeDir;
            hatchDir *= -1.0;
          }
          xCoord1 += m_InputValues->StripeWidth;
          stripeDir *= -1.0f;
          if(yCoord < minY)
          {
            yCoord = minY;
          }
          else if(yCoord > maxY)
          {
            yCoord = maxY;
          }
        }
      }
    }
    if(i % 10 == 0)
    {
      m_MessageHandler(fmt::format("Generating Hatches || Layer {} of {}", i, numLayers));
    }
  }

  // size hatch geometry correctly now
  auto& fitHatches = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->HatchDataContainerName);
  usize numHatchVerts = 2 * hatchCount;
  fitHatches.resizeVertexList(numHatchVerts);
  fitHatches.resizeEdgeList(hatchCount);
  AbstractDataStore<INodeGeometry0D::SharedVertexList::value_type>& hatchVerts = fitHatches.getVertices()->getDataStoreRef();
  AbstractDataStore<INodeGeometry1D::SharedEdgeList::value_type>& hatches = fitHatches.getEdges()->getDataStoreRef();
  std::vector<usize> tDims(1, numHatchVerts);
  fitHatches.getVertexAttributeMatrix()->resizeTuples(tDims);
  tDims[0] = hatchCount;
  fitHatches.getEdgeAttributeMatrix()->resizeTuples(tDims);

  auto& times = m_DataStructure.getDataAs<Float64Array>(m_InputValues->HatchDataContainerName.createChildPath(m_InputValues->VertexAttributeMatrixName).createChildPath(m_InputValues->TimeArrayName))
                    ->getDataStoreRef();
  const DataPath hatchAttributeMatrixPath = m_InputValues->HatchDataContainerName.createChildPath(m_InputValues->HatchAttributeMatrixName);
  auto& powers = m_DataStructure.getDataAs<Float32Array>(hatchAttributeMatrixPath.createChildPath(m_InputValues->PowersArrayName))->getDataStoreRef();
  auto& hatchSliceIds = m_DataStructure.getDataAs<Int32Array>(hatchAttributeMatrixPath.createChildPath(m_InputValues->CADSliceIdsArrayPath.getTargetName()))->getDataStoreRef();
  auto& hatchRegionIds = m_DataStructure.getDataAs<Int32Array>(hatchAttributeMatrixPath.createChildPath(m_InputValues->RegionIdsArrayName))->getDataStoreRef();

  if(getCancel())
  {
    return {};
  }
  for(usize i = 0; i < hatchCount; i++)
  {
    hatchVerts[3 * (2 * i) + 0] = vertCoords[3 * (2 * i) + 0];
    hatchVerts[3 * (2 * i) + 1] = vertCoords[3 * (2 * i) + 1];
    hatchVerts[3 * (2 * i) + 2] = vertCoords[3 * (2 * i) + 2];
    hatchVerts[3 * (2 * i + 1) + 0] = vertCoords[3 * (2 * i + 1) + 0];
    hatchVerts[3 * (2 * i + 1) + 1] = vertCoords[3 * (2 * i + 1) + 1];
    hatchVerts[3 * (2 * i + 1) + 2] = vertCoords[3 * (2 * i + 1) + 2];
    hatches[2 * i + 0] = edgeVertIds[2 * i + 0];
    hatches[2 * i + 1] = edgeVertIds[2 * i + 1];
    hatchSliceIds[i] = edgeSliceIds[i];
    hatchRegionIds[i] = edgeRegionIds[i];
    powers[i] = edgePowers[i];
    times[2 * i + 0] = vertTimes[2 * i + 0];
    times[2 * i + 1] = vertTimes[2 * i + 1];
  }

  // rotate all vertices back to original orientations of the hatching
  // rotate the CAD slices back too
  for(usize i = 0; i < hatchCount; i++)
  {
    int32 curLayer = hatchSliceIds[i];
    float32 angle = rotationAngles[curLayer];
    std::array<float32, 4> ax = {0.0f, 0.0f, 1.0f, angle};
    std::array<float32, 9> om = ax2om(ax);
    rotMat = toGMatrix(om);
    for(int j = 0; j < 2; j++)
    {
      int64 vert = hatches[2 * i + j];
      coords[0] = hatchVerts[3 * vert];
      coords[1] = hatchVerts[3 * vert + 1];
      coords[2] = hatchVerts[3 * vert + 2];
      newcoords = rotMat * coords;
      hatchVerts[3 * vert] = newcoords[0];
      hatchVerts[3 * vert + 1] = newcoords[1];
      hatchVerts[3 * vert + 2] = newcoords[2];
    }
  }
  for(int64 i = 0; i < numCADLayerEdges; i++)
  {
    int32 curLayer = cadSliceIds[i];
    if(curLayer < numCADLayers)
    {
      float32 angle = rotationAngles[curLayer];
      std::array<float32, 4> ax = {0.0f, 0.0f, 1.0f, angle};
      std::array<float32, 9> om = ax2om(ax);
      rotMat = toGMatrix(om);
      for(int j = 0; j < 2; j++)
      {
        usize vertIndex = 2 * i + j;
        int64 vert = CADLayerEdges[vertIndex];
        if(!rotatedPtIds[vert])
        {
          continue;
        }
        coords[0] = CADLayerVerts[3 * vert];
        coords[1] = CADLayerVerts[3 * vert + 1];
        coords[2] = CADLayerVerts[3 * vert + 2];
        newcoords = rotMat * coords;
        CADLayerVerts[3 * vert] = newcoords[0];
        CADLayerVerts[3 * vert + 1] = newcoords[1];
        CADLayerVerts[3 * vert + 2] = newcoords[2];
        rotatedPtIds[vert] = false;
      }
    }
  }

  m_MessageHandler("Complete");

  return {};
}
