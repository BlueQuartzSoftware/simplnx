#include "ApproximatePointCloudHull.hpp"

#include <algorithm>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;

namespace
{
bool ValidNeighbor(const SizeVec3Type& dims, const int64 neighborhood[78], usize index, int64 x, int64 y, int64 z)
{
  int64 modX = x + neighborhood[3 * index + 0];
  int64 modY = y + neighborhood[3 * index + 1];
  int64 modZ = z + neighborhood[3 * index + 2];

  return (modX >= 0 && modX < dims[0]) && (modY >= 0 && modY < dims[1]) && (modZ >= 0 && modZ < dims[2]);
}
} // namespace

// -----------------------------------------------------------------------------
ApproximatePointCloudHull::ApproximatePointCloudHull(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     ApproximatePointCloudHullInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
Result<> ApproximatePointCloudHull::operator()()
{
  float inverseResolution[3] = {1.0f / m_InputValues->grid_resolution[0], 1.0f / m_InputValues->grid_resolution[1], 1.0f / m_InputValues->grid_resolution[2]};

  auto* sourcePtr = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->input_vertex_geometry_path);
  auto* vertsPtr = sourcePtr->getVertices();

  DataStructure temp;
  auto* samplingGridPtr = ImageGeom::Create(temp, "Image Geometry");
  if(auto result = samplingGridPtr->setSpacing(m_InputValues->grid_resolution[0], m_InputValues->grid_resolution[1], m_InputValues->grid_resolution[2]); result.invalid())
  {
    return result;
  }

  usize numVerts = sourcePtr->getNumberOfVertices();
  auto& vertex = sourcePtr->getVertices()->getDataStoreRef();

  std::vector<float32> meshMaxExtents;
  std::vector<float32> meshMinExtents;

  for(usize i = 0; i < 3; i++)
  {
    meshMaxExtents.push_back(std::numeric_limits<float>::lowest());
    meshMinExtents.push_back(std::numeric_limits<float>::max());
  }

  for(int64 i = 0; i < numVerts; i++)
  {
    meshMaxExtents[0] = std::max<float>(vertex[3 * i], meshMaxExtents[0]);
    meshMaxExtents[1] = std::max<float>(vertex[3 * i + 1], meshMaxExtents[1]);
    meshMaxExtents[2] = std::max<float>(vertex[3 * i + 2], meshMaxExtents[2]);
    meshMinExtents[0] = std::min<float>(vertex[3 * i], meshMinExtents[0]);
    meshMinExtents[1] = std::min<float>(vertex[3 * i + 1], meshMinExtents[1]);
    meshMinExtents[2] = std::min<float>(vertex[3 * i + 2], meshMinExtents[2]);
  }

  int64 bboxMin[3] = {0, 0, 0};
  int64 bboxMax[3] = {0, 0, 0};

  // Add an empty one-voxel border on each side of the occupied grid.
  // Both padding and vertex mapping use integer voxel indices so float rounding cannot remove the border.
  for(usize i = 0; i < 3; i++)
  {
    bboxMin[i] = static_cast<int64>(std::floor(meshMinExtents[i] * inverseResolution[i])) - 1;
    bboxMax[i] = static_cast<int64>(std::floor(meshMaxExtents[i] * inverseResolution[i])) + 1;
  }

  auto dims1 = static_cast<usize>(bboxMax[0] - bboxMin[0] + 1);
  auto dims2 = static_cast<usize>(bboxMax[1] - bboxMin[1] + 1);
  auto dims3 = static_cast<usize>(bboxMax[2] - bboxMin[2] + 1);
  SizeVec3Type dims(std::vector<usize>{dims1, dims2, dims3});
  samplingGridPtr->setDimensions(dims);

  int64 multiplier[3] = {1, static_cast<int64>(samplingGridPtr->getNumXCells()), static_cast<int64>(samplingGridPtr->getNumXCells() * samplingGridPtr->getNumYCells())};
  std::vector<std::vector<int64>> vertsInVoxels(samplingGridPtr->getNumberOfCells());

  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  progressThrottle.reset(numVerts, "Mapping Vertices to Voxels");

  for(int64 v = 0; v < numVerts; v++)
  {
    if(v % 4096 == 0)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(v > 0)
      {
        progressThrottle.updatePercent(v);
      }
    }
    auto i = static_cast<int64>(std::floor((*vertsPtr)[3 * v + 0] * inverseResolution[0])) - bboxMin[0];
    auto j = static_cast<int64>(std::floor((*vertsPtr)[3 * v + 1] * inverseResolution[1])) - bboxMin[1];
    auto k = static_cast<int64>(std::floor((*vertsPtr)[3 * v + 2] * inverseResolution[2])) - bboxMin[2];
    int64 index = i * multiplier[0] + j * multiplier[1] + k * multiplier[2];
    vertsInVoxels[index].push_back(v);
  }

  progressThrottle.updatePercent(numVerts);

  std::vector<float> tmpVerts;
  int64 neighborhood[78] = {1,  0, 0,  -1, 0, 0, 0, 1, 0,  0, -1, 0, 0, 0,  1,  0, 0, -1, 1, 1, 0,  -1, 1,  0, 1, -1, 0,  -1, -1, 0, 1,  0, 1,  1,  0,  -1, -1, 0,  1,
                            -1, 0, -1, 0,  1, 1, 0, 1, -1, 0, -1, 1, 0, -1, -1, 1, 1, 1,  1, 1, -1, 1,  -1, 1, 1, -1, -1, -1, 1,  1, -1, 1, -1, -1, -1, 1,  -1, -1, -1};

  progressThrottle.reset(dims[2], "Trimming Interior Voxels");
  int64 vertCounter = 0;
  float xAvg = 0.0f;
  float yAvg = 0.0f;
  float zAvg = 0.0f;

  for(int64 z = 0; z < dims[2]; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        usize index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
        if(vertsInVoxels[index].empty())
        {
          continue;
        }

        usize emtpyNeighbors = 0;

        for(usize n = 0; n < 26; n++)
        {
          if(ValidNeighbor(dims, neighborhood, n, x, y, z))
          {
            usize neighborIndex = ((z + neighborhood[3 * n + 2]) * dims[1] * dims[0]) + ((y + neighborhood[3 * n + 1]) * dims[0]) + (x + neighborhood[3 * n + 0]);
            if(vertsInVoxels[neighborIndex].empty())
            {
              emtpyNeighbors++;
            }
          }
        }

        if(emtpyNeighbors > m_InputValues->min_empty_neighbors)
        {
          for(auto vert : vertsInVoxels[index])
          {
            vertCounter++;
            xAvg += (*vertsPtr)[3 * vert + 0];
            yAvg += (*vertsPtr)[3 * vert + 1];
            zAvg += (*vertsPtr)[3 * vert + 2];
          }
          xAvg /= static_cast<float>(vertCounter);
          yAvg /= static_cast<float>(vertCounter);
          zAvg /= static_cast<float>(vertCounter);
          tmpVerts.push_back(xAvg);
          tmpVerts.push_back(yAvg);
          tmpVerts.push_back(zAvg);
          vertCounter = 0;
          xAvg = 0.0f;
          yAvg = 0.0f;
          zAvg = 0.0f;
        }
      }
    }
    progressThrottle.updateCount(z + 1);
  }

  auto* hullPtr = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->output_vertex_geometry_path);
  Result<> resizeResult = hullPtr->resizeVertexList(tmpVerts.size() / 3);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  if(hullPtr->getVertexAttributeMatrix() != nullptr)
  {
    resizeResult = hullPtr->getVertexAttributeMatrix()->resizeTuples({tmpVerts.size() / 3});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }
  auto* hullVertsPtr = hullPtr->getVertices();
  auto* tmpVertDataPtr = tmpVerts.data();
  for(usize i = 0; i < hullPtr->getNumberOfVertices() * 3; i++)
  {
    (*hullVertsPtr)[i] = tmpVertDataPtr[i];
  }

  return {};
}
