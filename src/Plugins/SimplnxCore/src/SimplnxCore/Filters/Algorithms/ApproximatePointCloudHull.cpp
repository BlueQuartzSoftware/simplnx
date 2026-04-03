#include "ApproximatePointCloudHull.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

using namespace nx::core;

namespace
{
bool validNeighbor(const SizeVec3& dims, const int64 neighborhood[78], usize index, int64 x, int64 y, int64 z)
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
ApproximatePointCloudHull::~ApproximatePointCloudHull() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ApproximatePointCloudHull::operator()()
{
  float inverseResolution[3] = {1.0f / m_InputValues->GridResolution[0], 1.0f / m_InputValues->GridResolution[1], 1.0f / m_InputValues->GridResolution[2]};

  auto* source = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->InputVertexGeometryPath);
  auto* verts = source->getVertices();

  DataStructure temp;
  auto* samplingGrid = ImageGeom::Create(temp, "Image Geometry");
  samplingGrid->setSpacing(m_InputValues->GridResolution[0], m_InputValues->GridResolution[1], m_InputValues->GridResolution[2]);

  usize numVerts = source->getNumberOfVertices();
  auto& vertex = source->getVertices()->getDataStoreRef();

  std::vector<float32> meshMaxExtents;
  std::vector<float32> meshMinExtents;

  for(usize i = 0; i < 3; i++)
  {
    meshMaxExtents.push_back(std::numeric_limits<float>::lowest());
    meshMinExtents.push_back(std::numeric_limits<float>::max());
  }

  for(int64 i = 0; i < numVerts; i++)
  {
    if(vertex[3 * i] > meshMaxExtents[0])
    {
      meshMaxExtents[0] = vertex[3 * i];
    }
    if(vertex[3 * i + 1] > meshMaxExtents[1])
    {
      meshMaxExtents[1] = vertex[3 * i + 1];
    }
    if(vertex[3 * i + 2] > meshMaxExtents[2])
    {
      meshMaxExtents[2] = vertex[3 * i + 2];
    }
    if(vertex[3 * i] < meshMinExtents[0])
    {
      meshMinExtents[0] = vertex[3 * i];
    }
    if(vertex[3 * i + 1] < meshMinExtents[1])
    {
      meshMinExtents[1] = vertex[3 * i + 1];
    }
    if(vertex[3 * i + 2] < meshMinExtents[2])
    {
      meshMinExtents[2] = vertex[3 * i + 2];
    }
  }

  for(auto i = 0; i < 3; i++)
  {
    meshMinExtents[i] -= (inverseResolution[i] / 2.0f);
    meshMaxExtents[i] += (inverseResolution[i] / 2.0f);
  }

  int64 bboxMin[3] = {0, 0, 0};
  int64 bboxMax[3] = {0, 0, 0};

  bboxMin[0] = static_cast<int64>(std::floor(meshMinExtents[0] * inverseResolution[0]));
  bboxMin[1] = static_cast<int64>(std::floor(meshMinExtents[1] * inverseResolution[1]));
  bboxMin[2] = static_cast<int64>(std::floor(meshMinExtents[2] * inverseResolution[2]));

  bboxMax[0] = static_cast<int64>(std::floor(meshMaxExtents[0] * inverseResolution[0]));
  bboxMax[1] = static_cast<int64>(std::floor(meshMaxExtents[1] * inverseResolution[1]));
  bboxMax[2] = static_cast<int64>(std::floor(meshMaxExtents[2] * inverseResolution[2]));

  auto dims1 = static_cast<usize>(bboxMax[0] - bboxMin[0] + 1);
  auto dims2 = static_cast<usize>(bboxMax[1] - bboxMin[1] + 1);
  auto dims3 = static_cast<usize>(bboxMax[2] - bboxMin[2] + 1);
  SizeVec3 dims(std::vector<usize>{dims1, dims2, dims3});
  samplingGrid->setDimensions(dims);

  int64 multiplier[3] = {1, static_cast<int64>(samplingGrid->getNumXCells()), static_cast<int64>(samplingGrid->getNumXCells() * samplingGrid->getNumYCells())};
  std::vector<std::vector<int64>> vertsInVoxels(samplingGrid->getNumberOfCells());

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(numVerts);
  progressHelper.setProgressMessageTemplate("Mapping Vertices to Voxels: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(int64 v = 0; v < numVerts; v++)
  {
    auto i = static_cast<int64>(std::floor((*verts)[3 * v + 0] * inverseResolution[0]) - static_cast<float>(bboxMin[0]));
    auto j = static_cast<int64>(std::floor((*verts)[3 * v + 1] * inverseResolution[1]) - static_cast<float>(bboxMin[1]));
    auto k = static_cast<int64>(std::floor((*verts)[3 * v + 2] * inverseResolution[2]) - static_cast<float>(bboxMin[2]));
    int64 index = i * multiplier[0] + j * multiplier[1] + k * multiplier[2];
    vertsInVoxels[index].push_back(v);
    progressMessenger.sendProgressMessage(1);
  }

  std::vector<float> tmpVerts;
  int64 neighborhood[78] = {1,  0, 0,  -1, 0, 0, 0, 1, 0,  0, -1, 0, 0, 0,  1,  0, 0, -1, 1, 1, 0,  -1, 1,  0, 1, -1, 0,  -1, -1, 0, 1,  0, 1,  1,  0,  -1, -1, 0,  1,
                            -1, 0, -1, 0,  1, 1, 0, 1, -1, 0, -1, 1, 0, -1, -1, 1, 1, 1,  1, 1, -1, 1,  -1, 1, 1, -1, -1, -1, 1,  1, -1, 1, -1, -1, -1, 1,  -1, -1, -1};

  progressHelper.resetProgress();
  progressHelper.setMaxProgresss(dims[2]);
  progressHelper.setProgressMessageTemplate("Trimming Interior Voxels: {:.1f}% Complete");
  auto trimProgressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

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
          if(validNeighbor(dims, neighborhood, n, x, y, z))
          {
            usize neighborIndex = ((z + neighborhood[3 * n + 2]) * dims[1] * dims[0]) + ((y + neighborhood[3 * n + 1]) * dims[0]) + (x + neighborhood[3 * n + 0]);
            if(vertsInVoxels[neighborIndex].empty())
            {
              emtpyNeighbors++;
            }
          }
        }

        if(emtpyNeighbors > m_InputValues->MinEmptyNeighbors)
        {
          for(auto vert : vertsInVoxels[index])
          {
            vertCounter++;
            xAvg += (*verts)[3 * vert + 0];
            yAvg += (*verts)[3 * vert + 1];
            zAvg += (*verts)[3 * vert + 2];
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
    trimProgressMessenger.sendProgressMessage(1);
  }

  auto* hull = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->OutputVertexGeometryPath);
  hull->resizeVertexList(tmpVerts.size() / 3);
  if(hull->getVertexAttributeMatrix() != nullptr)
  {
    hull->getVertexAttributeMatrix()->resizeTuples({tmpVerts.size() / 3});
  }
  auto* hullVerts = hull->getVertices();
  auto tmpVertData = tmpVerts.data();
  for(usize i = 0; i < hull->getNumberOfVertices() * 3; i++)
  {
    (*hullVerts)[i] = tmpVertData[i];
  }

  return {};
}
