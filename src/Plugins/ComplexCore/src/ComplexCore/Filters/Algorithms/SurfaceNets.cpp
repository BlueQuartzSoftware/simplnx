#include "SurfaceNets.hpp"
#include "TupleTransfer.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "ComplexCore/SurfaceNets/MMCellFlag.h"
#include "ComplexCore/SurfaceNets/MMCellMap.h"
#include "ComplexCore/SurfaceNets/MMGeometryOBJ.h"
#include "ComplexCore/SurfaceNets/MMSurfaceNet.h"

using namespace complex;

namespace
{
//
// Private data class for storing quad data during OBJ data generation
//
class MMQuad
{
public:
  MMQuad()
  : m_VertexIndices{-1, -1, -1, -1}
  , m_Labels{0, 0}
  {
  }
  MMQuad(std::array<int32, 4> vi, std::array<int32, 2> labels)
  : m_VertexIndices{vi[0], vi[1], vi[2], vi[3]}
  , m_Labels{labels[0], labels[1]}
  {
  }

  void getVertexIndices(std::array<int32, 4>& vertexIndices)
  {
    std::copy(m_VertexIndices.begin(), m_VertexIndices.end(), vertexIndices.begin());
  }
  void getLabels(std::array<int32, 2>& labels)
  {
    std::copy(m_Labels.begin(), m_Labels.end(), labels.begin());
  }

private:
  std::array<int32, 4> m_VertexIndices;
  std::array<int32, 2> m_Labels;
};

struct VertexData
{
  int VertexId;
  std::array<float32, 3> Position;
};

void crossProduct(const std::array<float32, 3>& vert0, const std::array<float32, 3> vert1, std::array<float32, 3> result)
{
  // Cross product of vectors v0 and v1
  result[0] = vert0[1] * vert1[2] - vert0[2] * vert1[1];
  result[1] = vert0[2] * vert1[0] - vert0[0] * vert1[2];
  result[2] = vert0[0] * vert1[1] - vert0[1] * vert1[0];
}
float triangleArea(std::array<float32, 3>& vert0, std::array<float32, 3>& vert1, std::array<float32, 3>& vert2)
{
  // Area of triangle with vertex positions p0, p1, p2
  const std::array<float32, 3> v01 = {vert1[0] - vert0[0], vert1[1] - vert0[1], vert1[2] - vert0[2]};
  const std::array<float32, 3> v02 = {vert2[0] - vert0[0], vert2[1] - vert0[1], vert2[2] - vert0[2]};
  std::array<float32, 3> cross = {0.0f, 0.0f, 0.0f};
  crossProduct(v01, v02, cross);
  float const magCP = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
  return 0.5f * magCP;
}

void getQuadTriangleIDs(std::array<VertexData, 4>& vData, bool isQuadFrontFacing, std::array<int32, 6>& triangleVtxIDs)
{
  // Order quad vertices so quad is front facing
  if(!isQuadFrontFacing)
  {
    VertexData const temp = vData[3];
    vData[3] = vData[1];
    vData[1] = temp;
  }

  // Order quad vertices so that the two generated triangles have the minimal area. This
  // reduces self intersections in the surface.
  float const thisArea = triangleArea(vData[0].Position, vData[1].Position, vData[2].Position) + triangleArea(vData[0].Position, vData[2].Position, vData[3].Position);
  float const alternateArea = triangleArea(vData[1].Position, vData[2].Position, vData[3].Position) + triangleArea(vData[1].Position, vData[3].Position, vData[0].Position);
  if(alternateArea < thisArea)
  {
    VertexData const temp = vData[0];
    vData[0] = vData[1];
    vData[1] = vData[2];
    vData[2] = vData[3];
    vData[3] = temp;
  }

  // Generate vertex ids to triangulate the quad
  triangleVtxIDs[0] = vData[0].VertexId;
  triangleVtxIDs[1] = vData[1].VertexId;
  triangleVtxIDs[2] = vData[2].VertexId;
  triangleVtxIDs[3] = vData[0].VertexId;
  triangleVtxIDs[4] = vData[2].VertexId;
  triangleVtxIDs[5] = vData[3].VertexId;
}

void exportObjFiles(MMSurfaceNet* m_surfaceNet)
{
  if(m_surfaceNet == nullptr)
  {
    return;
  }
  std::shared_ptr<MMGeometryOBJ> const geometry = std::make_shared<MMGeometryOBJ>(m_surfaceNet);

  // Export an OBJ file for each material to the specified path
  std::vector<int> const materials = geometry->labels();
  for(const auto& itMatIdx : materials)
  {
    if(itMatIdx > 20 && itMatIdx < 65530)
    {
      continue;
    }
    const std::string filename = fmt::format("surface_mash_test_{}.obj", itMatIdx);
    std::cout << "Export file Obj file: " << filename << "\n";
    std::ofstream stream(filename, std::ios_base::binary);
    if(stream.is_open())
    {
      const MMGeometryOBJ::OBJData data = geometry->objData(itMatIdx);
      stream << "# vertices for feature id " << itMatIdx << "\n";
      for(const auto& vertex : data.vertexPositions)
      {
        stream << "v " << (vertex)[0] << ' ' << (vertex)[1] << ' ' << (vertex)[2] << "\n";
      }
      stream << "# triangles for feature id " << itMatIdx << "\n";
      for(const auto& face : data.triangles)
      {
        stream << "f " << (face)[0] << ' ' << (face)[1] << ' ' << (face)[2] << "\n";
      }
    }
  }
}

} // namespace
// -----------------------------------------------------------------------------
SurfaceNets::SurfaceNets(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SurfaceNets::~SurfaceNets() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SurfaceNets::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SurfaceNets::operator()()
{
  // Get the ImageGeometry
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GridGeomDataPath);

  // Get the Created Triangle Geometry
  TriangleGeom& triangleGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeometryPath);

  auto gridDimensions = imageGeom.getDimensions();
  auto voxelSize = imageGeom.getSpacing();
  auto origin = imageGeom.getOrigin();

  IntVec3 arraySize(static_cast<int32>(gridDimensions[0]), static_cast<int32>(gridDimensions[1]), static_cast<int32>(gridDimensions[2]));

  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  using LabelType = int32;
  std::vector<LabelType> labels(featureIds.getNumberOfTuples());
  for(size_t idx = 0; idx < featureIds.getNumberOfTuples(); idx++)
  {
    labels[idx] = static_cast<LabelType>(featureIds[idx]);
  }

  MMSurfaceNet surfaceNet(labels.data(), arraySize.data(), voxelSize.data());

  // Use current parameters to relax the SurfaceNet
  if(m_InputValues->ApplySmoothing)
  {
    MMSurfaceNet::RelaxAttrs relaxAttrs{};
    relaxAttrs.maxDistFromCellCenter = m_InputValues->MaxDistanceFromVoxel;
    relaxAttrs.numRelaxIterations = m_InputValues->SmoothingIterations;
    relaxAttrs.relaxFactor = m_InputValues->RelaxationFactor;

    surfaceNet.relax(relaxAttrs);
  }

  auto* cellMapPtr = surfaceNet.getCellMap();
  const int nodeCount = cellMapPtr->numVertices();

  std::array<int, 3> arraySize2 = {0, 0, 0};
  cellMapPtr->getArraySize(arraySize2.data());

  triangleGeom.resizeVertexList(nodeCount);
  triangleGeom.getVertexAttributeMatrix()->resizeTuples({static_cast<usize>(nodeCount)});

  LinkedGeometryData& linkedGeometryData = triangleGeom.getLinkedGeometryData();

  // Remove and then insert a properly sized int8 for the NodeTypes
  Int8Array& nodeTypes = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->NodeTypesDataPath);
  nodeTypes.resizeTuples({static_cast<usize>(nodeCount)});
  linkedGeometryData.addVertexData(m_InputValues->NodeTypesDataPath);

  Point3Df position = {0.0f, 0.0f, 0.0f};

  std::array<int, 3> vertCellIndex = {0, 0, 0};
  for(int32 vertIndex = 0; vertIndex < nodeCount; vertIndex++)
  {
    cellMapPtr->getVertexPosition(vertIndex, position.data());
    // Relocate the vertex correctly based on the origin of the ImageGeometry
    position = position + origin - Point3Df(0.5f * voxelSize[0], 0.5f * voxelSize[1], 0.5f * voxelSize[1]);

    triangleGeom.setVertexCoordinate(static_cast<usize>(vertIndex), position);
    cellMapPtr->getVertexCellIndex(vertIndex, vertCellIndex.data());
    MMCellMap::Cell* currentCellPtr = cellMapPtr->getCell(vertCellIndex.data());
    nodeTypes[static_cast<usize>(vertIndex)] = static_cast<int8>(currentCellPtr->flag.numJunctions());
  }
  usize triangleCount = 0;
  // First Pass through to just count the number of triangles:
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {
    std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
    std::array<LabelType, 2> quadLabels = {0, 0};
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data()))
    {
      if(quadLabels[0] == MMSurfaceNet::Padding || quadLabels[1] == MMSurfaceNet::Padding)
      {
        for(auto& vertIndex : vertexIndices)
        {
          if(nodeTypes[static_cast<usize>(vertIndex)] < 10)
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 10;
          }
          else
          {
            nodeTypes[static_cast<usize>(vertIndex)] += 1;
          }
        }
      }
      triangleCount += 2;
    }
  }
  triangleGeom.resizeFaceList(triangleCount);
  triangleGeom.getFaceAttributeMatrix()->resizeTuples({triangleCount});

  // Resize the face labels Int32Array
  Int32Array& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FaceLabelsDataPath);
  faceLabels.resizeTuples({triangleCount});
  linkedGeometryData.addFaceData(m_InputValues->FaceLabelsDataPath);

  // Create a vector of TupleTransferFunctions for each of the Triangle Face to VertexType Data Arrays
  std::vector<std::shared_ptr<AbstractTupleTransfer>> tupleTransferFunctions;
  for(size_t i = 0; i < m_InputValues->SelectedDataArrayPaths.size(); i++)
  {
    // Associate these arrays with the Triangle Face Data.
    linkedGeometryData.addFaceData(m_InputValues->SelectedDataArrayPaths[i]);
    ::AddTupleTransferInstance(m_DataStructure, m_InputValues->SelectedDataArrayPaths[i], m_InputValues->CreatedDataArrayPaths[i], tupleTransferFunctions);
  }

  usize faceIndex = 0;
  //   Create temporary storage for cell quads which are constructed around edges
  //   crossed by the surface. Handle 3 edges per cell. The other 9 cell edges will
  //   be handled when neighboring cells that share edges with this cell are visited.
  std::array<usize, 3> t1 = {0, 0, 0};
  std::array<usize, 3> t2 = {0, 0, 0};
  std::array<int, 6> triangleVtxIDs = {0, 0, 0, 0, 0, 0};
  std::array<int32, 4> vertexIndices = {0, 0, 0, 0};
  std::array<LabelType, 2> quadLabels = {0, 0};
  std::array<VertexData, 4> vData{};
  for(int idxVtx = 0; idxVtx < nodeCount; idxVtx++)
  {

    // Back-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::BackBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }

      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }

      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }
      faceIndex++;
    }

    // Left-bottom edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBottomEdge, vertexIndices.data(), quadLabels.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]); ///
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }
      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }
      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }
      faceIndex++;
    }

    // Left-back edge
    if(cellMapPtr->getEdgeQuad(idxVtx, MMCellFlag::Edge::LeftBackEdge, vertexIndices.data(), quadLabels.data()))
    {
      vData[0] = {vertexIndices[0], 00.0f, 0.0f, 0.0f};
      vData[1] = {vertexIndices[1], 00.0f, 0.0f, 0.0f};
      vData[2] = {vertexIndices[2], 00.0f, 0.0f, 0.0f};
      vData[3] = {vertexIndices[3], 00.0f, 0.0f, 0.0f};

      const bool isQuadFrontFacing = (quadLabels[0] < quadLabels[1]);
      if(quadLabels[0] == MMSurfaceNet::Padding)
      {
        quadLabels[0] = 0;
      }
      if(quadLabels[1] == MMSurfaceNet::Padding)
      {
        quadLabels[1] = 0;
      }
      getQuadTriangleIDs(vData, isQuadFrontFacing, triangleVtxIDs);
      t1 = {static_cast<usize>(triangleVtxIDs[0]), static_cast<usize>(triangleVtxIDs[1]), static_cast<usize>(triangleVtxIDs[2])};
      t2 = {static_cast<usize>(triangleVtxIDs[3]), static_cast<usize>(triangleVtxIDs[4]), static_cast<usize>(triangleVtxIDs[5])};

      triangleGeom.setFacePointIds(faceIndex, t1);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }
      faceIndex++;

      triangleGeom.setFacePointIds(faceIndex, t2);
      if(quadLabels[0] < quadLabels[1])
      {
        faceLabels[faceIndex * 2] = quadLabels[0];
        faceLabels[faceIndex * 2 + 1] = quadLabels[1];
      }
      else
      {
        faceLabels[faceIndex * 2] = quadLabels[1];
        faceLabels[faceIndex * 2 + 1] = quadLabels[0];
      }
      // Copy any Cell Data to the Triangle Mesh
      for(size_t dataVectorIndex = 0; dataVectorIndex < m_InputValues->SelectedDataArrayPaths.size(); dataVectorIndex++)
      {
        tupleTransferFunctions[dataVectorIndex]->transfer(faceIndex, quadLabels[0], quadLabels[1], faceLabels);
      }
      faceIndex++;
    }
  }

  return {};
}
