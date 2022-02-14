#include "ApproximatePointCloudHull.hpp"

#include <cmath>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{
namespace
{
bool validNeighbor(const SizeVec3& dims, int64 neighborhood[78], usize index, int64 x, int64 y, int64 z)
{
  int64 modX = x + neighborhood[3 * index + 0];
  int64 modY = y + neighborhood[3 * index + 1];
  int64 modZ = z + neighborhood[3 * index + 2];

  return (modX >= 0 && modX < dims[0]) && (modY >= 0 && modY < dims[1]) && (modZ >= 0 && modZ < dims[2]);
}
} // namespace

std::string ApproximatePointCloudHull::name() const
{
  return FilterTraits<ApproximatePointCloudHull>::name;
}

std::string ApproximatePointCloudHull::className() const
{
  return FilterTraits<ApproximatePointCloudHull>::className;
}

Uuid ApproximatePointCloudHull::uuid() const
{
  return FilterTraits<ApproximatePointCloudHull>::uuid;
}

std::string ApproximatePointCloudHull::humanName() const
{
  return "Approximate Point Cloud Hull";
}

Parameters ApproximatePointCloudHull::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GridResolution_Key, "Grid Resolution", "Geometry resolution", std::vector<float32>{0, 0, 0}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<UInt64Parameter>(k_MinEmptyNeighbors_Key, "Minimum Number of Empty Neighbors", "Minimum number of empty neighbors", 0));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_VertexGeomPath_Key, "Vertex Geometry", "Path to the target Vertex geometry", DataPath()));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_HullVertexGeomPath_Key, "Hull Vertex Geometry", "Path to create the hull Vertex geometry", DataPath{}));
  return params;
}

IFilter::UniquePointer ApproximatePointCloudHull::clone() const
{
  return std::make_unique<ApproximatePointCloudHull>();
}

IFilter::PreflightResult ApproximatePointCloudHull::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto gridResolution = args.value<std::vector<float32>>(k_GridResolution_Key);
  auto numberOfEmptyNeighbors = args.value<uint64>(k_MinEmptyNeighbors_Key);
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeomPath_Key);
  auto hullVertexGeomPath = args.value<DataPath>(k_HullVertexGeomPath_Key);

  if(gridResolution[0] <= 0.0f || gridResolution[1] <= 0.0f || gridResolution[2] <= 0.0f)
  {
    std::string ss = fmt::format("Grid resolutions must be greater than zero");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11001, ss}})};
  }

  if(numberOfEmptyNeighbors < 0)
  {
    std::string ss = fmt::format("Minimum number of empty neighbors must be positive");
    return {nonstd::make_unexpected(std::vector<Error>{Error{-11001, ss}})};
  }

  auto vertexGeom = data.getDataAs<VertexGeom>(vertexGeomPath);

  int64 numVertices = vertexGeom->getNumberOfVertices();
  auto action = std::make_unique<CreateVertexGeometryAction>(hullVertexGeomPath, numVertices);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> ApproximatePointCloudHull::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  auto gridResolution = args.value<std::vector<float32>>(k_GridResolution_Key);
  auto numberOfEmptyNeighbors = args.value<uint64>(k_MinEmptyNeighbors_Key);
  auto vertexGeomPath = args.value<DataPath>(k_VertexGeomPath_Key);
  auto hullVertexGeomPath = args.value<DataPath>(k_HullVertexGeomPath_Key);

  float inverseResolution[3] = {1.0f / gridResolution[0], 1.0f / gridResolution[1], 1.0f / gridResolution[2]};

  auto* source = data.getDataAs<VertexGeom>(vertexGeomPath);
  auto* verts = source->getVertices();

  DataStructure temp;
  auto* samplingGrid = ImageGeom::Create(temp, "Image Geometry");
  samplingGrid->setSpacing(gridResolution[0], gridResolution[1], gridResolution[2]);

  int64 numVerts = source->getNumberOfVertices();
  auto* vertex = source->getVertices();

  std::vector<float32> meshMaxExtents;
  std::vector<float32> meshMinExtents;

  for(usize i = 0; i < 3; i++)
  {
    meshMaxExtents.push_back(std::numeric_limits<float>::lowest());
    meshMinExtents.push_back(std::numeric_limits<float>::max());
  }

  for(int64 i = 0; i < numVerts; i++)
  {
    if((*vertex)[3 * i] > meshMaxExtents[0])
    {
      meshMaxExtents[0] = (*vertex)[3 * i];
    }
    if((*vertex)[3 * i + 1] > meshMaxExtents[1])
    {
      meshMaxExtents[1] = (*vertex)[3 * i + 1];
    }
    if((*vertex)[3 * i + 2] > meshMaxExtents[2])
    {
      meshMaxExtents[2] = (*vertex)[3 * i + 2];
    }
    if((*vertex)[3 * i] < meshMinExtents[0])
    {
      meshMinExtents[0] = (*vertex)[3 * i];
    }
    if((*vertex)[3 * i + 1] < meshMinExtents[1])
    {
      meshMinExtents[1] = (*vertex)[3 * i + 1];
    }
    if((*vertex)[3 * i + 2] < meshMinExtents[2])
    {
      meshMinExtents[2] = (*vertex)[3 * i + 2];
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

  usize dims1 = static_cast<usize>(bboxMax[0] - bboxMin[0] + 1);
  usize dims2 = static_cast<usize>(bboxMax[1] - bboxMin[1] + 1);
  usize dims3 = static_cast<usize>(bboxMax[2] - bboxMin[2] + 1);
  SizeVec3 dims(std::vector<usize>{dims1, dims2, dims3});
  samplingGrid->setDimensions(dims);

  int64 multiplier[3] = {1, static_cast<int64>(samplingGrid->getNumXPoints()), static_cast<int64>(samplingGrid->getNumXPoints() * samplingGrid->getNumYPoints())};
  std::vector<std::vector<int64>> vertsInVoxels(samplingGrid->getNumberOfElements());

  int64 progIncrement = numVerts / 100;
  int64 prog = 1;
  int64 progressInt = 0;

  for(int64 v = 0; v < numVerts; v++)
  {
    int64 i = static_cast<int64>(std::floor((*verts)[3 * v + 0] * inverseResolution[0]) - static_cast<float>(bboxMin[0]));
    int64 j = static_cast<int64>(std::floor((*verts)[3 * v + 1] * inverseResolution[1]) - static_cast<float>(bboxMin[1]));
    int64 k = static_cast<int64>(std::floor((*verts)[3 * v + 2] * inverseResolution[2]) - static_cast<float>(bboxMin[2]));
    int64 index = i * multiplier[0] + j * multiplier[1] + k * multiplier[2];
    vertsInVoxels[index].push_back(v);

    if(v > prog)
    {
      progressInt = static_cast<int64>((static_cast<float>(v) / numVerts) * 100.0f);
      std::string ss = fmt::format("Mapping Vertices to Voxels || {}% Complete", progressInt);
      // notifyStatusMessage(ss);
      prog = prog + progIncrement;
    }
  }

  std::vector<float> tmpVerts;
  int64 neighborhood[78] = {1,  0, 0,  -1, 0, 0, 0, 1, 0,  0, -1, 0, 0, 0,  1,  0, 0, -1, 1, 1, 0,  -1, 1,  0, 1, -1, 0,  -1, -1, 0, 1,  0, 1,  1,  0,  -1, -1, 0,  1,
                            -1, 0, -1, 0,  1, 1, 0, 1, -1, 0, -1, 1, 0, -1, -1, 1, 1, 1,  1, 1, -1, 1,  -1, 1, 1, -1, -1, -1, 1,  1, -1, 1, -1, -1, -1, 1,  -1, -1, -1};

  progIncrement = (dims[0] * dims[1] * dims[2]) / 100;
  prog = 1;
  progressInt = 0;
  int64 counter = 0;
  int64 vertCounter = 0;
  float xAvg = 0.0f;
  float yAvg = 0.0f;
  float zAvg = 0.0f;

  for(int64 z = 0; z < dims[2]; z++)
  {
    for(int64 y = 0; y < dims[1]; y++)
    {
      for(int64 x = 0; x < dims[0]; x++)
      {
        usize index = (z * dims[1] * dims[0]) + (y * dims[0]) + x;
        if(vertsInVoxels[index].empty())
        {
          counter++;
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

        if(emtpyNeighbors > numberOfEmptyNeighbors)
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

        if(counter > prog)
        {
          progressInt = static_cast<int64>((static_cast<float>(counter) / (dims[0] * dims[1] * dims[2])) * 100.0f);
          std::string ss = fmt::format("Trimming Interior Voxels || {}% Complete", progressInt);
          // notifyStatusMessage(ss);
          prog = prog + progIncrement;
        }
        counter++;
      }
    }
  }

  auto* hull = data.getDataAs<VertexGeom>(hullVertexGeomPath);
  hull->resizeVertexList(tmpVerts.size() / 3);
  auto* hullVerts = hull->getVertices();
  auto tmpVertData = tmpVerts.data();
  for(usize i = 0; i < hull->getNumberOfVertices() * 3; i++)
  {
    (*hullVerts)[i] = tmpVertData[i];
  }

  return {};
}
} // namespace complex
