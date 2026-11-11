#include "ApproximatePointCloudHull.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <cmath>

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_GridSizeRepresentation = -86133;
constexpr nx::core::int32 k_VoxelBoundsRepresentation = -86132;

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

  std::array<float64, 3> minimumVoxelCoordinates = {};
  std::array<float64, 3> maximumVoxelCoordinates = {};
  for(usize axis = 0; axis < 3; ++axis)
  {
    minimumVoxelCoordinates[axis] = std::floor(meshMinExtents[axis] * inverseResolution[axis]);
    maximumVoxelCoordinates[axis] = std::floor(meshMaxExtents[axis] * inverseResolution[axis]);
  }
  // The returned dimensions include an empty one-voxel border on each side of the occupied grid.
  auto gridResult = ValidateGridRepresentation(minimumVoxelCoordinates, maximumVoxelCoordinates);
  if(gridResult.invalid())
  {
    return ConvertResult(std::move(gridResult));
  }
  std::array<int64, 3> bboxMin = {};

  // Validation guarantees each lower bound is an integer above the int64 minimum, so the border is subtracted exactly.
  for(usize axis = 0; axis < 3; ++axis)
  {
    bboxMin[axis] = static_cast<int64>(minimumVoxelCoordinates[axis]) - 1;
  }

  const SizeVec3 dims = gridResult.value();
  samplingGrid->setDimensions(dims);

  int64 multiplier[3] = {1, static_cast<int64>(samplingGrid->getNumXCells()), static_cast<int64>(samplingGrid->getNumXCells() * samplingGrid->getNumYCells())};
  std::vector<std::vector<int64>> vertsInVoxels(samplingGrid->getNumberOfCells());

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
    auto i = static_cast<int64>(std::floor((*verts)[3 * v + 0] * inverseResolution[0])) - bboxMin[0];
    auto j = static_cast<int64>(std::floor((*verts)[3 * v + 1] * inverseResolution[1])) - bboxMin[1];
    auto k = static_cast<int64>(std::floor((*verts)[3 * v + 2] * inverseResolution[2])) - bboxMin[2];
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
    progressThrottle.updateCount(z + 1);
  }

  auto* hull = m_DataStructure.getDataAs<VertexGeom>(m_InputValues->OutputVertexGeometryPath);
  Result<> resizeResult = hull->resizeVertexList(tmpVerts.size() / 3);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  if(hull->getVertexAttributeMatrix() != nullptr)
  {
    resizeResult = hull->getVertexAttributeMatrix()->resizeTuples({tmpVerts.size() / 3});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }
  auto* hullVerts = hull->getVertices();
  auto tmpVertData = tmpVerts.data();
  for(usize i = 0; i < hull->getNumberOfVertices() * 3; i++)
  {
    (*hullVerts)[i] = tmpVertData[i];
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<SizeVec3> ApproximatePointCloudHull::ValidateGridRepresentation(const std::array<float64, 3>& minimumVoxelCoordinates, const std::array<float64, 3>& maximumVoxelCoordinates)
{
  const float64 exclusiveBound = std::ldexp(1.0, 63);
  const uint64 signedLimit = static_cast<uint64>(std::numeric_limits<int64>::max());
  SizeVec3 dimensions;
  usize product = 1;
  for(usize axis = 0; axis < 3; ++axis)
  {
    const float64 minimum = minimumVoxelCoordinates[axis];
    const float64 maximum = maximumVoxelCoordinates[axis];
    if(!std::isfinite(minimum) || !std::isfinite(maximum) || minimum < -exclusiveBound || maximum < -exclusiveBound || minimum >= exclusiveBound || maximum >= exclusiveBound ||
       std::floor(minimum) != minimum || std::floor(maximum) != maximum)
    {
      return MakeErrorResult<SizeVec3>(k_VoxelBoundsRepresentation, fmt::format("Grid Resolution produces {} voxel bounds minimum={}, maximum={}. Bounds must be finite integers in [{}, {}).",
                                                                                "XYZ"[axis], minimum, maximum, -exclusiveBound, exclusiveBound));
    }
    if(minimum == -exclusiveBound)
    {
      return MakeErrorResult<SizeVec3>(
          k_VoxelBoundsRepresentation,
          fmt::format("Grid Resolution produces {} voxel bounds minimum={}, maximum={}. The one-voxel border below the minimum does not fit int64.", "XYZ"[axis], minimum, maximum));
    }
    // The border is added in integer arithmetic so float rounding cannot remove it. The upper bound is below 2^63, so adding 1 cannot overflow.
    const int64 lower = static_cast<int64>(minimum) - 1;
    const int64 upper = static_cast<int64>(maximum) + 1;
    // Unsigned subtraction preserves the exact nonnegative difference across the signed zero boundary.
    const uint64 difference = static_cast<uint64>(upper) - static_cast<uint64>(lower);
    if(upper < lower || difference >= signedLimit || difference >= std::numeric_limits<usize>::max())
    {
      return MakeErrorResult<SizeVec3>(k_GridSizeRepresentation,
                                       fmt::format("Hull grid {} inclusive width cannot fit signed indices: minimum={}, maximum={}, int64 limit={}.", "XYZ"[axis], lower, upper, signedLimit));
    }
    dimensions[axis] = static_cast<usize>(difference + 1);
    if(dimensions[axis] > std::numeric_limits<usize>::max() / product)
    {
      return MakeErrorResult<SizeVec3>(k_GridSizeRepresentation,
                                       fmt::format("Hull grid tuple product overflows usize at {} width {} and preceding product {}.", "XYZ"[axis], dimensions[axis], product));
    }
    product *= dimensions[axis];
  }
  if(product > signedLimit || product > std::numeric_limits<usize>::max() / sizeof(std::vector<int64>) || product > std::vector<std::vector<int64>>{}.max_size())
  {
    return MakeErrorResult<SizeVec3>(k_GridSizeRepresentation, fmt::format("Hull grid product {} cannot fit signed indices or container bytes (element bytes={}, container limit={}): {}.", product,
                                                                           sizeof(std::vector<int64>), std::vector<std::vector<int64>>{}.max_size(), StringUtilities::formatDimensions3D(dimensions)));
  }
  return {dimensions};
}
