/**
 * @file SurfaceNetsScanline.cpp
 * @brief Implements Surface Nets with slice I/O and external padded-cell records.
 */

#include <array>

#include "SurfaceNetsScanline.hpp"

#include "SurfaceNets.hpp"
#include "TupleTransfer.hpp"

#include "SimplnxCore/SurfaceNets/MMCellFlag.h"
#include "SimplnxCore/SurfaceNets/MMSurfaceNet.h"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/InMemoryTemporaryRecordStore.hpp"
#include "simplnx/Utilities/Meshing/TriangleUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <nonstd/span.hpp>
#include <vector>

using namespace nx::core;

namespace
{
using LabelType = int32;

/**
 * @brief Packs a padded-grid (i,j,k) coordinate into a fixed record-store index.
 * @param i Padded X coordinate.
 * @param j Padded Y coordinate.
 * @param k Padded Z coordinate.
 * @param paddedX Padded X dimension.
 * @param paddedXY Product of padded X and Y dimensions.
 * @return Flat padded-cell record index.
 */
inline uint64 cellRecordIndex(int32 i, int32 j, int32 k, int32 paddedX, int32 paddedXY)
{
  return static_cast<uint64>(i) + static_cast<uint64>(j) * static_cast<uint64>(paddedX) + static_cast<uint64>(k) * static_cast<uint64>(paddedXY);
}

/** @brief Returns true when the selected mode omits a padding-to-background quad. */
bool SkipPaddingQuad(ChoicesParameter::ValueType mode, const std::array<LabelType, 2>& quadLabels)
{
  if(mode != BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    return false;
  }
  return (quadLabels[0] == MMSurfaceNet::Padding && quadLabels[1] == 0) || (quadLabels[1] == MMSurfaceNet::Padding && quadLabels[0] == 0);
}

/**
 * @struct VertexData
 * @brief Stores a mesh vertex ID and position for quad triangulation.
 */
struct VertexData
{
  usize VertexId = 0;
  std::array<float32, 3> Position;
};

/**
 * @brief Computes a three-dimensional cross product.
 * @param vert0 First vector.
 * @param vert1 Second vector.
 * @param result Local output copy. The caller's array is unchanged.
 */
void crossProduct(const std::array<float32, 3>& vert0, const std::array<float32, 3> vert1, std::array<float32, 3> result)
{
  result[0] = vert0[1] * vert1[2] - vert0[2] * vert1[1];
  result[1] = vert0[2] * vert1[0] - vert0[0] * vert1[2];
  result[2] = vert0[0] * vert1[1] - vert0[1] * vert1[0];
}

/**
 * @brief Computes a triangle area from three positions.
 * @param vert0 First position.
 * @param vert1 Second position.
 * @param vert2 Third position.
 * @return Zero because crossProduct() receives its result by value.
 */
float32 triangleArea(std::array<float32, 3>& vert0, std::array<float32, 3>& vert1, std::array<float32, 3>& vert2)
{
  const std::array<float32, 3> v01 = {vert1[0] - vert0[0], vert1[1] - vert0[1], vert1[2] - vert0[2]};
  const std::array<float32, 3> v02 = {vert2[0] - vert0[0], vert2[1] - vert0[1], vert2[2] - vert0[2]};
  std::array<float32, 3> cross = {0.0f, 0.0f, 0.0f};
  crossProduct(v01, v02, cross);
  float32 const magCP = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  return 0.5f * magCP;
}

/**
 * @brief Orients a quad and selects its lower-area triangulation.
 * @param vData Quad vertices, reordered in place.
 * @param isQuadFrontFacing True when the initial order faces forward.
 * @param triangleVtxIDs Receives two three-vertex triangles.
 *
 * Both area results are zero because crossProduct() cannot update its caller.
 * Current call sites also supply zero positions. The first diagonal remains.
 */
void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<usize, 6>& triangleVtxIDs)
{
  // Swap side vertices when label order indicates back-facing winding.
  if(!isQuadFrontFacing)
  {
    VertexData const temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Prefer the lower-area diagonal when positions distinguish the alternatives.
  float32 const thisArea = triangleArea(vData[0].Position, vData[1].Position, vData[2].Position) + triangleArea(vData[0].Position, vData[2].Position, vData[3].Position);
  float32 const alternateArea = triangleArea(vData[1].Position, vData[2].Position, vData[3].Position) + triangleArea(vData[1].Position, vData[3].Position, vData[0].Position);
  if(alternateArea < thisArea)
  {
    VertexData const temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Emit two triangles as a fan from vertex zero.
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
}

/**
 * @brief Converts a padded cell coordinate to a flat ImageGeom cell index.
 * @param ci Padded X coordinate.
 * @param cj Padded Y coordinate.
 * @param ck Padded Z coordinate.
 * @param paddedX Preserves the padded-dimension call signature.
 * @param paddedY Preserves the padded-dimension call signature.
 * @param paddedZ Preserves the padded-dimension call signature.
 * @param dimX ImageGeom X dimension.
 * @param dimY ImageGeom Y dimension.
 * @param dimZ ImageGeom Z dimension.
 * @return Flat ImageGeom index, or usize max outside the image.
 */
inline usize edgeCellNxIndex(int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedY, int32 paddedZ, usize dimX, usize dimY, usize dimZ)
{
  const int32 nxX = ci - 1;
  const int32 nxY = cj - 1;
  const int32 nxZ = ck - 1;
  if(nxX < 0 || nxX >= static_cast<int32>(dimX) || nxY < 0 || nxY >= static_cast<int32>(dimY) || nxZ < 0 || nxZ >= static_cast<int32>(dimZ))
  {
    return std::numeric_limits<usize>::max();
  }
  return static_cast<usize>(nxZ) * dimY * dimX + static_cast<usize>(nxY) * dimX + static_cast<usize>(nxX);
}

/**
 * @brief Gets one padded corner label from two ImageGeom slices.
 *
 * Boundary corners (any coordinate at 0 or >= paddedDim-1) return MMSurfaceNet::Padding.
 * Interior corners look up the label from the appropriate slice buffer.
 *
 * @param ci Padded X corner coordinate.
 * @param cj Padded Y corner coordinate.
 * @param ck Padded Z corner coordinate.
 * @param paddedX Padded X dimension.
 * @param paddedY Padded Y dimension.
 * @param paddedZ Padded Z dimension.
 * @param dimX ImageGeom X dimension.
 * @param dimY ImageGeom Y dimension.
 * @param slice0 First buffered Feature ID slice.
 * @param slice0Z ImageGeom Z index in slice0.
 * @param slice1 Second buffered Feature ID slice.
 * @param slice1Z ImageGeom Z index in slice1.
 * @return Feature ID, or the padding label at the exterior or on a buffer miss.
 */
inline int32 cornerLabel(int32 ci, int32 cj, int32 ck, int32 paddedX, int32 paddedY, int32 paddedZ, usize dimX, usize dimY, const std::vector<int32>& slice0, int32 slice0Z,
                         const std::vector<int32>& slice1, int32 slice1Z)
{
  // Exterior padded corners close the mesh at the volume boundary.
  if(ci <= 0 || cj <= 0 || ck <= 0 || ci >= paddedX - 1 || cj >= paddedY - 1 || ck >= paddedZ - 1)
  {
    return MMSurfaceNet::ReservedLabel::Padding;
  }

  // Convert to ImageGeom cell coordinates.
  const int32 nxX = ci - 1;
  const int32 nxY = cj - 1;
  const int32 nxZ = ck - 1;

  const usize sliceOffset = static_cast<usize>(nxY) * dimX + static_cast<usize>(nxX);

  if(nxZ == slice0Z)
  {
    return slice0[sliceOffset];
  }
  if(nxZ == slice1Z)
  {
    return slice1[sliceOffset];
  }

  // Treat an unexpected buffer miss as exterior padding.
  return MMSurfaceNet::ReservedLabel::Padding;
}
} // namespace

SurfaceNetsScanline::SurfaceNetsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

SurfaceNetsScanline::~SurfaceNetsScanline() noexcept = default;

Result<> SurfaceNetsScanline::operator()()
{
  const auto& sentinelFeatureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  Result<> sentinelCheck = MeshingUtilities::ValidateFeatureIdsAgainstSentinels(sentinelFeatureIdsStore, m_InputValues->FeatureIdsArrayPath, true, m_ShouldCancel, m_MessageHandler);
  if(sentinelCheck.invalid())
  {
    return sentinelCheck;
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // One padding cell on each side closes surfaces at the image exterior.
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);
  auto gridDimensions = imageGeom.getDimensions();

  const usize dimX = gridDimensions[0];
  const usize dimY = gridDimensions[1];
  const usize dimZ = gridDimensions[2];

  if(dimX > static_cast<usize>(std::numeric_limits<int32>::max() - 2) || dimY > static_cast<usize>(std::numeric_limits<int32>::max() - 2) ||
     dimZ > static_cast<usize>(std::numeric_limits<int32>::max() - 2))
  {
    return MakeErrorResult(-62048, "SurfaceNets image dimensions exceed the padded-cell coordinate range.");
  }
  if(dimY != 0 && dimX > std::numeric_limits<usize>::max() / dimY)
  {
    return MakeErrorResult(-62052, "SurfaceNets image slice size overflows usize.");
  }

  const int32 paddedX = static_cast<int32>(dimX) + 2;
  const int32 paddedY = static_cast<int32>(dimY) + 2;
  const int32 paddedZ = static_cast<int32>(dimZ) + 2;

  if(static_cast<uint64>(paddedX) > std::numeric_limits<uint64>::max() / static_cast<uint64>(paddedY) ||
     static_cast<uint64>(paddedX) * static_cast<uint64>(paddedY) > std::numeric_limits<uint64>::max() / static_cast<uint64>(paddedZ) ||
     static_cast<uint64>(paddedX) * static_cast<uint64>(paddedY) > static_cast<uint64>(std::numeric_limits<int32>::max()))
  {
    return MakeErrorResult(-62049, "SurfaceNets padded-cell dimensions exceed the supported record-index range.");
  }
  const int32 paddedXY = paddedX * paddedY;

  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();
  const auto anyDispatchTargetOutOfCore = [&]() {
    const auto isOoc = [](const IDataArray* array) { return array != nullptr && IsOutOfCore(*array); };
    if(isOoc(featureIdsArray) || isOoc(m_DataStructure.getDataAs<IDataArray>(m_InputValues->NodeTypesDataPath)) || isOoc(m_DataStructure.getDataAs<IDataArray>(m_InputValues->FaceLabelsDataPath)))
    {
      return true;
    }
    for(const auto& path : m_InputValues->SelectedCellDataArrayPaths)
    {
      if(isOoc(m_DataStructure.getDataAs<IDataArray>(path)))
      {
        return true;
      }
    }
    for(const auto& path : m_InputValues->SelectedFeatureDataArrayPaths)
    {
      if(isOoc(m_DataStructure.getDataAs<IDataArray>(path)))
      {
        return true;
      }
    }
    for(const auto& path : m_InputValues->CreatedDataArrayPaths)
    {
      if(isOoc(m_DataStructure.getDataAs<IDataArray>(path)))
      {
        return true;
      }
    }
    if(const auto* triangleGeom = m_DataStructure.getDataAs<TriangleGeom>(m_InputValues->TriangleGeometryPath); triangleGeom != nullptr)
    {
      return isOoc(triangleGeom->getVertices()) || isOoc(triangleGeom->getFaces());
    }
    return false;
  };

  const uint64 paddedCellCount = static_cast<uint64>(paddedX) * static_cast<uint64>(paddedY) * static_cast<uint64>(paddedZ);
  TemporaryRecordStoreConfig recordConfig;
  recordConfig.recordSize = sizeof(SurfaceCellRecord);
  recordConfig.maxRecordsPerBatch = 4096;
  recordConfig.initialRecordCount = paddedCellCount;
  // Actual OOC targets require external records. Resident forced-path tests may
  // use the explicit O(padded volume) in-memory fallback.
  auto recordStoreResult = DataStoreUtilities::GetIOCollection().createTemporaryRecordStore(recordConfig);
  if(recordStoreResult.valid())
  {
    m_SurfaceCells = std::move(recordStoreResult.value());
  }
  else if(!anyDispatchTargetOutOfCore())
  {
    auto fallbackResult = InMemoryTemporaryRecordStore::Create(recordConfig);
    if(fallbackResult.invalid())
    {
      return ConvertResult(std::move(fallbackResult));
    }
    m_SurfaceCells = std::move(fallbackResult.value());
  }
  else
  {
    return ConvertResult(std::move(recordStoreResult));
  }
  if(m_SurfaceCells == nullptr)
  {
    return MakeErrorResult(-62050, "SurfaceNets temporary-record-store provider returned a null padded-cell store.");
  }
  try
  {
    m_SurfaceCellCache = std::make_unique<BoundedRecordPageCache<SurfaceCellRecord>>(*m_SurfaceCells, recordConfig.maxRecordsPerBatch, 8);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62051, "SurfaceNets failed to allocate its bounded padded-cell page cache.");
  }

  SurfaceCellRecord inactiveRecord;
  inactiveRecord.Flag.clear();
  inactiveRecord.Label = MMSurfaceNet::ReservedLabel::Padding;
  auto initializeResult = m_SurfaceCells->fill(0, paddedCellCount, nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(&inactiveRecord), sizeof(SurfaceCellRecord)), m_ShouldCancel);
  if(initializeResult.invalid())
  {
    return initializeResult;
  }

  // Two Feature ID slices supply all eight corners of a padded cell.
  const usize sliceSize = dimX * dimY;
  std::vector<int32> sliceBufA;
  std::vector<int32> sliceBufB;
  try
  {
    sliceBufA.resize(sliceSize);
    sliceBufB.resize(sliceSize);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62053, "SurfaceNets failed to allocate its bounded feature-id slice buffers.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-62053, "SurfaceNets failed to allocate its bounded feature-id slice buffers.");
  }

  // Ping-pong pointers reuse the prior upper slice as the next lower slice.
  std::vector<int32>* slice0 = &sliceBufA;
  std::vector<int32>* slice1 = &sliceBufB;
  int32 slice0Z = -1;
  int32 slice1Z = -1;

  // Match MMCellMap padded raster order and corner order. A label change among
  // eight corners creates one sequential vertex ID.
  const usize totalPaddedZSlices = static_cast<usize>(paddedZ - 1);
  usize numVertices = 0;
  for(int32 k = 0; k < paddedZ - 1; k++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    // Bottom and top corners map to ImageGeom Z slices k-1 and k.
    const int32 needZ0 = k - 1;
    const int32 needZ1 = k;

    if(needZ0 >= 0 && needZ0 < static_cast<int32>(dimZ))
    {
      if(slice0Z != needZ0 && slice1Z != needZ0)
      {
        auto copyResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(needZ0) * sliceSize, nonstd::span<int32>(slice0->data(), sliceSize));
        if(copyResult.invalid())
        {
          return copyResult;
        }
        slice0Z = needZ0;
      }
    }
    if(needZ1 >= 0 && needZ1 < static_cast<int32>(dimZ))
    {
      if(slice0Z != needZ1 && slice1Z != needZ1)
      {
        auto copyResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(needZ1) * sliceSize, nonstd::span<int32>(slice1->data(), sliceSize));
        if(copyResult.invalid())
        {
          return copyResult;
        }
        slice1Z = needZ1;
      }
    }

    for(int32 j = 0; j < paddedY - 1; j++)
    {
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        // Corner order matches MMCellMap::setCellVertices():
        //   [0] = (i,   j,   k  )   left-back-bottom
        //   [1] = (i+1, j,   k  )   right-back-bottom
        //   [2] = (i+1, j+1, k  )   right-front-bottom
        //   [3] = (i,   j+1, k  )   left-front-bottom
        //   [4] = (i,   j,   k+1)   left-back-top
        //   [5] = (i+1, j,   k+1)   right-back-top
        //   [6] = (i+1, j+1, k+1)   right-front-top
        //   [7] = (i,   j+1, k+1)   left-front-top
        std::array<int32, 8> cellLabels;

        cellLabels[0] = cornerLabel(i, j, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[1] = cornerLabel(i + 1, j, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[2] = cornerLabel(i + 1, j + 1, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[3] = cornerLabel(i, j + 1, k, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[4] = cornerLabel(i, j, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[5] = cornerLabel(i + 1, j, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[6] = cornerLabel(i + 1, j + 1, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);
        cellLabels[7] = cornerLabel(i, j + 1, k + 1, paddedX, paddedY, paddedZ, dimX, dimY, *slice0, slice0Z, *slice1, slice1Z);

        MMCellFlag flag;
        flag.clear();
        flag.set(cellLabels.data());

        SurfaceCellRecord record;
        record.Flag = flag;
        record.Label = cellLabels[0];
        if(flag.vertexType() != MMCellFlag::VertexType::NoVertex)
        {
          if(numVertices == std::numeric_limits<usize>::max())
          {
            return MakeErrorResult(-56335, "SurfaceNets vertex count exceeds the supported tuple range.");
          }
          record.VertexId = static_cast<uint64>(numVertices);
          ++numVertices;
        }
        auto writeResult = m_SurfaceCellCache->write(cellRecordIndex(i, j, k, paddedX, paddedXY), record, m_ShouldCancel);
        if(writeResult.invalid())
        {
          return writeResult;
        }
      }
    }

    // Reuse the upper slice and make the other buffer available for k+1.
    std::swap(slice0, slice1);
    std::swap(slice0Z, slice1Z);
  }

  // Resize vertex output after classification gives the exact count.
  auto cacheFlushResult = m_SurfaceCellCache->flush(m_ShouldCancel);
  if(cacheFlushResult.invalid())
  {
    return cacheFlushResult;
  }

  auto& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);
  auto& verticesStore = triangleGeom.getVerticesRef().getDataStoreRef();
  auto& nodeTypes = m_DataStructure.getDataAs<Int8Array>(m_InputValues->NodeTypesDataPath)->getDataStoreRef();
  if(numVertices > std::numeric_limits<usize>::max() / 3)
  {
    return MakeErrorResult(-56336, "SurfaceNets vertex value count exceeds the supported tuple range.");
  }
  try
  {
    verticesStore.resizeTuples(ShapeType{numVertices});
    triangleGeom.getVertexAttributeMatrix()->resizeTuples({numVertices});
    nodeTypes.resizeTuples({numVertices});
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-56333, "SurfaceNets could not resize its vertex outputs.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-56333, "SurfaceNets could not resize its vertex outputs.");
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-56333, fmt::format("SurfaceNets could not resize its vertex outputs: {}", exception.what()));
  }

  // Relax in the same padded raster order as MMSurfaceNet. Surface vertices use
  // crossed faces. Junction vertices use only junction-crossed faces. Each local
  // coordinate blends toward the neighbor mean and remains within the clamp.
  static constexpr std::array<std::array<int32, 3>, 6> k_FaceOffsets = {{
      {-1, 0, 0}, // LeftFace
      {+1, 0, 0}, // RightFace
      {0, -1, 0}, // BackFace
      {0, +1, 0}, // FrontFace
      {0, 0, -1}, // BottomFace
      {0, 0, +1}  // TopFace
  }};

  if(m_InputValues->ApplySmoothing)
  {
    const float32 alpha = m_InputValues->RelaxationFactor;
    const float32 maxDist = m_InputValues->MaxDistanceFromVoxel;
    const float32 minClamp = 0.5f - maxDist;
    const float32 maxClamp = 0.5f + maxDist;

    for(int32 iter = 0; iter < m_InputValues->SmoothingIterations; iter++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      for(int32 cellK = 0; cellK < paddedZ - 1; cellK++)
      {
        for(int32 cellJ = 0; cellJ < paddedY - 1; cellJ++)
        {
          for(int32 cellI = 0; cellI < paddedX - 1; cellI++)
          {
            const uint64 recordIndex = cellRecordIndex(cellI, cellJ, cellK, paddedX, paddedXY);
            auto recordResult = m_SurfaceCellCache->read(recordIndex, m_ShouldCancel);
            if(recordResult.invalid())
            {
              return ConvertResult(std::move(recordResult));
            }
            auto record = recordResult.value();
            if(record.VertexId == std::numeric_limits<uint64>::max())
            {
              continue;
            }

            int32 numNeighbors = 0;
            std::array<float32, 3> avgP = {0.0f, 0.0f, 0.0f};

            for(MMCellFlag::Face face = MMCellFlag::Face::LeftFace; face <= MMCellFlag::Face::TopFace; ++face)
            {
              // Vertex type selects ordinary or junction-only crossed faces.
              bool participates = false;
              if(record.Flag.vertexType() == MMCellFlag::VertexType::SurfaceVertex)
              {
                participates = (record.Flag.faceCrossingType(face) != MMCellFlag::FaceCrossingType::NoFaceCrossing);
              }
              else
              {
                participates = (record.Flag.faceCrossingType(face) == MMCellFlag::FaceCrossingType::JunctionFaceCrossing);
              }

              if(!participates)
              {
                continue;
              }

              const int32 faceIdx = static_cast<int32>(face);
              const int32 nbrI = cellI + k_FaceOffsets[faceIdx][0];
              const int32 nbrJ = cellJ + k_FaceOffsets[faceIdx][1];
              const int32 nbrK = cellK + k_FaceOffsets[faceIdx][2];

              float32 nbrPosX = 0.5f;
              float32 nbrPosY = 0.5f;
              float32 nbrPosZ = 0.5f;
              if(nbrI >= 0 && nbrJ >= 0 && nbrK >= 0 && nbrI < paddedX && nbrJ < paddedY && nbrK < paddedZ)
              {
                auto neighborResult = m_SurfaceCellCache->read(cellRecordIndex(nbrI, nbrJ, nbrK, paddedX, paddedXY), m_ShouldCancel);
                if(neighborResult.invalid())
                {
                  return ConvertResult(std::move(neighborResult));
                }
                const auto& neighbor = neighborResult.value();
                if(neighbor.VertexId != std::numeric_limits<uint64>::max())
                {
                  nbrPosX = neighbor.LocalPosition[0];
                  nbrPosY = neighbor.LocalPosition[1];
                  nbrPosZ = neighbor.LocalPosition[2];
                }
              }

              // Convert the neighbor's local position into the current cell frame.
              avgP[0] += nbrPosX + static_cast<float32>(nbrI - cellI);
              avgP[1] += nbrPosY + static_cast<float32>(nbrJ - cellJ);
              avgP[2] += nbrPosZ + static_cast<float32>(nbrK - cellK);
              numNeighbors++;
            }

            // Blend with the average and clamp around the cell center.
            if(numNeighbors > 0)
            {
              avgP[0] /= static_cast<float32>(numNeighbors);
              avgP[1] /= static_cast<float32>(numNeighbors);
              avgP[2] /= static_cast<float32>(numNeighbors);

              record.LocalPosition[0] = std::clamp((1.0f - alpha) * record.LocalPosition[0] + alpha * avgP[0], minClamp, maxClamp);
              record.LocalPosition[1] = std::clamp((1.0f - alpha) * record.LocalPosition[1] + alpha * avgP[1], minClamp, maxClamp);
              record.LocalPosition[2] = std::clamp((1.0f - alpha) * record.LocalPosition[2] + alpha * avgP[2], minClamp, maxClamp);

              auto writeResult = m_SurfaceCellCache->write(recordIndex, record, m_ShouldCancel);
              if(writeResult.invalid())
              {
                return writeResult;
              }
            }
          }
        }
      }
    }
  }

  // Keep padded records authoritative. A resident surface map would make a
  // dense OOC surface require O(surface size) RAM.
  auto smoothingFlushResult = m_SurfaceCellCache->flush(m_ShouldCancel);
  if(smoothingFlushResult.invalid())
  {
    return smoothingFlushResult;
  }
  const auto hasCellRecord = [=](int32 i, int32 j, int32 k) { return i >= 0 && j >= 0 && k >= 0 && i < paddedX - 1 && j < paddedY - 1 && k < paddedZ - 1; };
  const auto recordAt = [&](int32 i, int32 j, int32 k) -> Result<SurfaceCellRecord> {
    if(!hasCellRecord(i, j, k))
    {
      return MakeErrorResult<SurfaceCellRecord>(-62055, "SurfaceNets encountered an invalid padded-cell neighbor.");
    }
    return m_SurfaceCellCache->read(cellRecordIndex(i, j, k, paddedX, paddedXY), m_ShouldCancel);
  };
  const auto labelAt = [&](int32 i, int32 j, int32 k) -> Result<LabelType> {
    if(i <= 0 || j <= 0 || k <= 0 || i >= paddedX - 1 || j >= paddedY - 1 || k >= paddedZ - 1)
    {
      return {MMSurfaceNet::ReservedLabel::Padding};
    }
    auto recordResult = recordAt(i, j, k);
    if(recordResult.invalid())
    {
      return ConvertInvalidResult<LabelType>(std::move(recordResult));
    }
    return {recordResult.value().Label};
  };
  const auto nodeTypeAt = [&](int32 i, int32 j, int32 k) -> Result<int8> {
    const std::array<std::array<int32, 3>, 8> corners = {{{i, j, k}, {i + 1, j, k}, {i, j + 1, k}, {i + 1, j + 1, k}, {i, j, k + 1}, {i + 1, j, k + 1}, {i + 1, j + 1, k + 1}, {i, j + 1, k + 1}}};
    std::array<int32, 8> distinct{};
    usize distinctCount = 0;
    bool hasPadding = false;
    for(const auto& corner : corners)
    {
      auto labelResult = labelAt(corner[0], corner[1], corner[2]);
      if(labelResult.invalid())
      {
        return ConvertInvalidResult<int8>(std::move(labelResult));
      }
      const int32 label = labelResult.value();
      hasPadding = hasPadding || label == MMSurfaceNet::Padding;
      if(std::find(distinct.begin(), distinct.begin() + distinctCount, label) == distinct.begin() + distinctCount)
      {
        distinct[distinctCount++] = label;
      }
    }
    const usize cappedCount = std::min<usize>(distinctCount, 4);
    return {static_cast<int8>(cappedCount + (hasPadding ? 10 : 0))};
  };

  // Initialize node types in the same padded raster order as Direct.
  for(int32 k = 0; k < paddedZ - 1; k++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(int32 j = 0; j < paddedY - 1; j++)
    {
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        auto recordResult = recordAt(i, j, k);
        if(recordResult.invalid())
        {
          return ConvertResult(std::move(recordResult));
        }
        auto record = recordResult.value();
        if(record.VertexId != std::numeric_limits<uint64>::max())
        {
          auto nodeTypeResult = nodeTypeAt(i, j, k);
          if(nodeTypeResult.invalid())
          {
            return ConvertResult(std::move(nodeTypeResult));
          }
          record.NodeType = nodeTypeResult.value();
          auto writeResult = m_SurfaceCellCache->write(cellRecordIndex(i, j, k, paddedX, paddedXY), record, m_ShouldCancel);
          if(writeResult.invalid())
          {
            return writeResult;
          }
        }
      }
    }
  }

  // Count faces and apply exterior-node changes before output allocation.
  usize triangleCount = 0;
  usize suppressedFaceCount = 0;
  const auto countEdge = [&](int32 i, int32 j, int32 k, const SurfaceCellRecord& record, MMCellFlag::Edge edge, const std::array<std::array<int32, 3>, 4>& quadCells,
                             const std::array<int32, 3>& otherLabelOffset) -> Result<> {
    if(!record.Flag.isEdgeCrossing(edge))
    {
      return {};
    }
    auto label0 = labelAt(i, j, k);
    auto label1 = labelAt(i + otherLabelOffset[0], j + otherLabelOffset[1], k + otherLabelOffset[2]);
    if(label0.invalid() || label1.invalid())
    {
      return label0.invalid() ? ConvertResult(std::move(label0)) : ConvertResult(std::move(label1));
    }
    const std::array<LabelType, 2> quadLabels = {label0.value(), label1.value()};
    if(SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
    {
      suppressedFaceCount++;
      return {};
    }
    for(const auto& cell : quadCells)
    {
      auto quadRecordResult = recordAt(cell[0], cell[1], cell[2]);
      if(quadRecordResult.invalid())
      {
        return ConvertResult(std::move(quadRecordResult));
      }
      auto quadRecord = quadRecordResult.value();
      quadRecord.IsReferenced = true;
      auto writeResult = m_SurfaceCellCache->write(cellRecordIndex(cell[0], cell[1], cell[2], paddedX, paddedXY), quadRecord, m_ShouldCancel);
      if(writeResult.invalid())
      {
        return writeResult;
      }
    }
    if(triangleCount > std::numeric_limits<usize>::max() - 2)
    {
      return MakeErrorResult(-56332, "SurfaceNets triangle count exceeds the supported tuple range.");
    }
    triangleCount += 2;
    return {};
  };
  for(int32 k = 0; k < paddedZ - 1; k++)
  {
    for(int32 j = 0; j < paddedY - 1; j++)
    {
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        auto recordResult = recordAt(i, j, k);
        if(recordResult.invalid())
        {
          return ConvertResult(std::move(recordResult));
        }
        const auto& record = recordResult.value();
        if(record.VertexId == std::numeric_limits<uint64>::max())
        {
          continue;
        }
        auto result = countEdge(i, j, k, record, MMCellFlag::Edge::BackBottomEdge, {{{i, j, k}, {i, j - 1, k}, {i, j - 1, k - 1}, {i, j, k - 1}}}, {{1, 0, 0}});
        if(result.invalid())
          return result;
        result = countEdge(i, j, k, record, MMCellFlag::Edge::LeftBottomEdge, {{{i, j, k}, {i, j, k - 1}, {i - 1, j, k - 1}, {i - 1, j, k}}}, {{0, 1, 0}});
        if(result.invalid())
          return result;
        result = countEdge(i, j, k, record, MMCellFlag::Edge::LeftBackEdge, {{{i, j, k}, {i - 1, j, k}, {i - 1, j - 1, k}, {i, j - 1, k}}}, {{0, 0, 1}});
        if(result.invalid())
          return result;
      }
    }
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly)
  {
    usize survivingVertexCount = 0;
    for(int32 k = 0; k < paddedZ - 1; k++)
    {
      for(int32 j = 0; j < paddedY - 1; j++)
      {
        for(int32 i = 0; i < paddedX - 1; i++)
        {
          auto recordResult = recordAt(i, j, k);
          if(recordResult.invalid())
          {
            return ConvertResult(std::move(recordResult));
          }
          auto record = recordResult.value();
          if(record.VertexId == std::numeric_limits<uint64>::max())
          {
            continue;
          }
          if(record.IsReferenced)
          {
            record.VertexId = survivingVertexCount++;
          }
          else
          {
            record.VertexId = std::numeric_limits<uint64>::max();
          }
          auto writeResult = m_SurfaceCellCache->write(cellRecordIndex(i, j, k, paddedX, paddedXY), record, m_ShouldCancel);
          if(writeResult.invalid())
          {
            return writeResult;
          }
        }
      }
    }
    numVertices = survivingVertexCount;
    verticesStore.resizeTuples(ShapeType{numVertices});
    triangleGeom.getVertexAttributeMatrix()->resizeTuples({numVertices});
    nodeTypes.resizeTuples({numVertices});
  }

  auto recordFlushResult = m_SurfaceCellCache->flush(m_ShouldCancel);
  if(recordFlushResult.invalid())
    return recordFlushResult;

  // Stream vertices and node types in 4,096-vertex batches by vertex ID.
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();
  const Point3Df halfVoxelOffset(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[2]);
  constexpr usize kOutputChunkSize = 4096;
  std::vector<float32> vertexBuffer;
  std::vector<int8> nodeBuffer;
  try
  {
    vertexBuffer.reserve(kOutputChunkSize * 3);
    nodeBuffer.reserve(kOutputChunkSize);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62054, "SurfaceNets failed to allocate its bounded vertex output buffers.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-62054, "SurfaceNets failed to allocate its bounded vertex output buffers.");
  }
  usize vertexStart = 0;
  const auto flushVertices = [&]() -> Result<> {
    if(vertexBuffer.empty())
      return {};
    if(vertexStart > numVertices || vertexStart > std::numeric_limits<usize>::max() / 3 || vertexBuffer.size() > (numVertices - vertexStart) * 3)
      return MakeErrorResult(-56336, "SurfaceNets vertex output range exceeds the supported value range.");
    auto vertexResult = verticesStore.copyFromBuffer(vertexStart * 3, nonstd::span<const float32>(vertexBuffer.data(), vertexBuffer.size()));
    if(vertexResult.invalid())
      return vertexResult;
    auto nodeResult = nodeTypes.copyFromBuffer(vertexStart, nonstd::span<const int8>(nodeBuffer.data(), nodeBuffer.size()));
    if(nodeResult.invalid())
      return nodeResult;
    vertexStart += nodeBuffer.size();
    vertexBuffer.clear();
    nodeBuffer.clear();
    return {};
  };
  for(int32 k = 0; k < paddedZ - 1; k++)
  {
    if(m_ShouldCancel)
      return {};
    for(int32 j = 0; j < paddedY - 1; j++)
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        auto recordResult = recordAt(i, j, k);
        if(recordResult.invalid())
          return ConvertResult(std::move(recordResult));
        const auto& record = recordResult.value();
        if(record.VertexId == std::numeric_limits<uint64>::max())
          continue;
        vertexBuffer.push_back(voxelSize[0] * (static_cast<float32>(i) + record.LocalPosition[0]) + origin[0] - halfVoxelOffset[0]);
        vertexBuffer.push_back(voxelSize[1] * (static_cast<float32>(j) + record.LocalPosition[1]) + origin[1] - halfVoxelOffset[1]);
        vertexBuffer.push_back(voxelSize[2] * (static_cast<float32>(k) + record.LocalPosition[2]) + origin[2] - halfVoxelOffset[2]);
        nodeBuffer.push_back(record.NodeType);
        if(nodeBuffer.size() == kOutputChunkSize)
        {
          auto result = flushVertices();
          if(result.invalid())
            return result;
        }
      }
  }
  auto vertexFlushResult = flushVertices();
  if(vertexFlushResult.invalid())
    return vertexFlushResult;
  if(vertexStart != numVertices)
    return MakeErrorResult(-62057, "SurfaceNets streamed an unexpected number of vertices.");

  if(triangleCount > std::numeric_limits<usize>::max() / 3)
  {
    return MakeErrorResult(-56337, "SurfaceNets face value count exceeds the supported tuple range.");
  }
  auto& faceLabels = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef();
  try
  {
    triangleGeom.resizeFaceList(triangleCount);
    triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});
    faceLabels.resizeTuples({triangleCount});
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-56334, "SurfaceNets could not resize its face outputs.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-56334, "SurfaceNets could not resize its face outputs.");
  } catch(const std::exception& exception)
  {
    return MakeErrorResult(-56334, fmt::format("SurfaceNets could not resize its face outputs: {}", exception.what()));
  }

  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(usize i = 0; i < m_InputValues->SelectedCellDataArrayPaths.size(); i++)
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedCellDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  const auto numSelectedCellArrayPaths = m_InputValues->SelectedCellDataArrayPaths.size();
  for(usize i = 0; i < m_InputValues->SelectedFeatureDataArrayPaths.size(); i++)
    ::AddFeatureTupleTransferInstance(m_DataStructure, m_InputValues->SelectedFeatureDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i + numSelectedCellArrayPaths],
                                      m_InputValues->FeatureIdsArrayPath, tupleTransferFunctions);

  using MeshIndexType = IGeometry::MeshIndexType;
  auto& facesStore = triangleGeom.getFacesRef().getDataStoreRef();
  std::vector<MeshIndexType> faceBuffer;
  std::vector<int32> faceLabelBuffer;
  std::vector<SurfaceNetsTransferData> transferBuffer;
  try
  {
    faceBuffer.reserve(kOutputChunkSize * 3);
    faceLabelBuffer.reserve(kOutputChunkSize * 2);
    transferBuffer.reserve(kOutputChunkSize);
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-62054, "SurfaceNets failed to allocate its bounded face output buffers.");
  } catch(const std::length_error&)
  {
    return MakeErrorResult(-62054, "SurfaceNets failed to allocate its bounded face output buffers.");
  }
  usize faceStart = 0;
  const auto flushFaces = [&]() -> Result<> {
    if(faceBuffer.empty())
      return {};
    if(faceStart > triangleCount || faceStart > std::numeric_limits<usize>::max() / 3 || faceBuffer.size() > (triangleCount - faceStart) * 3 ||
       faceLabelBuffer.size() > (triangleCount - faceStart) * 2)
      return MakeErrorResult(-56337, "SurfaceNets face output range exceeds the supported value range.");
    auto faceResult = facesStore.copyFromBuffer(faceStart * 3, nonstd::span<const MeshIndexType>(faceBuffer.data(), faceBuffer.size()));
    if(faceResult.invalid())
      return faceResult;
    auto labelResult = faceLabels.copyFromBuffer(faceStart * 2, nonstd::span<const int32>(faceLabelBuffer.data(), faceLabelBuffer.size()));
    if(labelResult.invalid())
      return labelResult;
    for(const auto& transfer : tupleTransferFunctions)
    {
      auto transferResult = transfer->surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData>(transferBuffer.data(), transferBuffer.size()));
      if(transferResult.invalid())
      {
        return transferResult;
      }
    }
    faceStart += transferBuffer.size();
    faceBuffer.clear();
    faceLabelBuffer.clear();
    transferBuffer.clear();
    return {};
  };
  const auto emitEdge = [&](int32 i, int32 j, int32 k, const SurfaceCellRecord& record, MMCellFlag::Edge edge, const std::array<std::array<int32, 3>, 4>& quadCells,
                            const std::array<int32, 3>& otherLabelOffset) -> Result<> {
    if(!record.Flag.isEdgeCrossing(edge))
      return {};
    auto label0 = labelAt(i, j, k);
    auto label1 = labelAt(i + otherLabelOffset[0], j + otherLabelOffset[1], k + otherLabelOffset[2]);
    if(label0.invalid() || label1.invalid())
      return label0.invalid() ? ConvertResult(std::move(label0)) : ConvertResult(std::move(label1));
    const std::array<LabelType, 2> quadLabels = {label0.value(), label1.value()};
    if(SkipPaddingQuad(m_InputValues->BoundingBoxSkinMode, quadLabels))
      return {};
    std::array<VertexData, 4> vData{};
    // Both implementations supply zero positions and therefore use the default diagonal.
    for(usize n = 0; n < quadCells.size(); n++)
    {
      auto quadRecord = recordAt(quadCells[n][0], quadCells[n][1], quadCells[n][2]);
      if(quadRecord.invalid())
        return ConvertResult(std::move(quadRecord));
      if(quadRecord.value().VertexId == std::numeric_limits<uint64>::max())
        return MakeErrorResult(-62056, "SurfaceNets edge quad is missing one of its required vertices.");
      vData[n] = {static_cast<usize>(quadRecord.value().VertexId), {0.0f, 0.0f, 0.0f}};
    }
    const bool frontFacing = label0.value() < label1.value();
    LabelType firstLabel = label0.value() == MMSurfaceNet::Padding ? -1 : label0.value();
    LabelType secondLabel = label1.value() == MMSurfaceNet::Padding ? -1 : label1.value();
    std::array<usize, 6> triangleVertexIds{};
    getQuadTriangleIDs(vData, frontFacing, triangleVertexIds);
    const std::array<usize, 2> nxIndices = {edgeCellNxIndex(i, j, k, paddedX, paddedY, paddedZ, dimX, dimY, dimZ),
                                            edgeCellNxIndex(i + otherLabelOffset[0], j + otherLabelOffset[1], k + otherLabelOffset[2], paddedX, paddedY, paddedZ, dimX, dimY, dimZ)};
    const auto appendTriangle = [&](usize offset) {
      faceBuffer.insert(faceBuffer.end(), triangleVertexIds.begin() + offset, triangleVertexIds.begin() + offset + 3);
      faceLabelBuffer.push_back(std::min(firstLabel, secondLabel));
      faceLabelBuffer.push_back(std::max(firstLabel, secondLabel));
      transferBuffer.push_back({faceStart + transferBuffer.size(), nxIndices});
    };
    appendTriangle(0);
    appendTriangle(3);
    if(transferBuffer.size() >= kOutputChunkSize)
    {
      auto flushResult = flushFaces();
      if(flushResult.invalid())
        return flushResult;
    }
    return {};
  };
  for(int32 k = 0; k < paddedZ - 1; k++)
    for(int32 j = 0; j < paddedY - 1; j++)
      for(int32 i = 0; i < paddedX - 1; i++)
      {
        if(m_ShouldCancel)
          return {};
        auto recordResult = recordAt(i, j, k);
        if(recordResult.invalid())
          return ConvertResult(std::move(recordResult));
        const auto& record = recordResult.value();
        if(record.VertexId == std::numeric_limits<uint64>::max())
          continue;
        auto result = emitEdge(i, j, k, record, MMCellFlag::Edge::BackBottomEdge, {{{i, j, k}, {i, j - 1, k}, {i, j - 1, k - 1}, {i, j, k - 1}}}, {{1, 0, 0}});
        if(result.invalid())
          return result;
        result = emitEdge(i, j, k, record, MMCellFlag::Edge::LeftBottomEdge, {{{i, j, k}, {i, j, k - 1}, {i - 1, j, k - 1}, {i - 1, j, k}}}, {{0, 1, 0}});
        if(result.invalid())
          return result;
        result = emitEdge(i, j, k, record, MMCellFlag::Edge::LeftBackEdge, {{{i, j, k}, {i - 1, j, k}, {i - 1, j - 1, k}, {i, j - 1, k}}}, {{0, 0, 1}});
        if(result.invalid())
          return result;
      }
  auto faceFlushResult = flushFaces();
  if(faceFlushResult.invalid())
    return faceFlushResult;
  if(faceStart != triangleCount)
    return MakeErrorResult(-62058, "SurfaceNets streamed an unexpected number of faces.");

  // Prefer bounded external winding repair when the provider supports it.
  Result<> windingResult = {};
  if(m_InputValues->RepairTriangleWinding)
  {
    auto& ioCollection = DataStoreUtilities::GetIOCollection();
    if(ioCollection.hasExternalSortCapability() && ioCollection.hasTemporaryRecordStoreCapability())
    {
      windingResult = MeshingUtilities::RepairTriangleWindingExternal(triangleGeom.getFaces()->getDataStoreRef(),
                                                                      m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);
    }
    else
    {
      if(anyDispatchTargetOutOfCore())
      {
        return MakeErrorResult(-62059,
                               "SurfaceNets cannot repair triangle winding for an out-of-core target because the active I/O provider does not support external sorting and temporary record stores.");
      }
      // Resident forced-path tests can use temporary in-memory connectivity.
      m_MessageHandler("Generating Connectivity and Triangle Neighbors...");
      triangleGeom.findElementNeighbors(true);
      const auto optionalId = triangleGeom.getElementNeighborsId();
      if(!optionalId.has_value())
      {
        return MakeErrorResult(-56331, fmt::format("Unable to generate the connectivity list for {} geometry.", triangleGeom.getName()));
      }
      const auto& connectivity = m_DataStructure.getDataRefAs<IGeometry::ElementDynamicList>(optionalId.value());
      m_MessageHandler("Repairing Windings...");
      windingResult = MeshingUtilities::RepairTriangleWinding(triangleGeom.getFaces()->getDataStoreRef(), connectivity,
                                                              m_DataStructure.getDataAs<Int32Array>(m_InputValues->FaceLabelsDataPath)->getDataStoreRef(), m_ShouldCancel, m_MessageHandler);
      m_DataStructure.removeData(triangleGeom.getElementContainingVertId().value());
      m_DataStructure.removeData(triangleGeom.getElementNeighborsId().value());
    }
  }

  if(m_InputValues->BoundingBoxSkinMode == BoundingBoxSkinMode::k_BackgroundBackedWallsOnly && windingResult.valid())
  {
    if(triangleGeom.getNumberOfFaces() == 0)
    {
      return MeshingUtilities::MakeEmptyMeshWarning(m_InputValues->TriangleGeometryPath, m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getNumberOfTuples(),
                                                    triangleGeom.getNumberOfVertices());
    }
    if(suppressedFaceCount == 0)
    {
      return MeshingUtilities::MakeNoFacesPrunedWarning(m_InputValues->TriangleGeometryPath);
    }
  }

  return windingResult;
}
