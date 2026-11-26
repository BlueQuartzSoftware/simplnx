#include "CombineNodeBasedGeometries.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ConcatenateDataArrays.hpp"

#include <algorithm>
#include <filesystem>

using namespace nx::core;

namespace
{
template <typename NodeGeomType, typename GetArrayFunc>
usize CalculateTotalTuples(std::vector<NodeGeomType*>& geomPtrs, GetArrayFunc getArray)
{
  usize tupleCount = 0;
  for(const auto& geomPtr : geomPtrs)
  {
    auto* arrayPtr = getArray(geomPtr);
    tupleCount += arrayPtr->getNumberOfTuples();
  }
  return tupleCount;
}

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
std::map<std::string, std::vector<DataPath>> RetrieveGeometryElementPaths(std::vector<NodeGeomType*>& geomPtrs, GetArrayFunc getArray, const std::string& elementsArrayName,
                                                                          GetAttrMatrixFunc getAttrMatrix)
{
  std::map<std::string, std::vector<DataPath>> pathsMap;
  for(usize geomIdx = 0; geomIdx < geomPtrs.size(); ++geomIdx)
  {
    auto* geomPtr = geomPtrs[geomIdx];

    // Retrieve the data array paths (edge arrays, face arrays, polyhedra arrays, etc.)
    AttributeMatrix* attrMatrixPtr = getAttrMatrix(geomPtr);
    if(attrMatrixPtr != nullptr)
    {
      for(const auto& pair : *attrMatrixPtr)
      {
        auto& pathsVector = pathsMap[pair.second->getName()];
        pathsVector.push_back(pair.second->getDataPaths()[0]);
      }
    }

    // Retrieve the array path (edges, faces, polyhedra, etc.)
    auto* arrayPtr = getArray(geomPtr);
    auto& pathsVector = pathsMap[elementsArrayName];
    pathsVector.push_back(arrayPtr->getDataPaths()[0]);
  }

  return pathsMap;
}

template <typename NodeGeomType, typename GetVerticesArrayFunc, typename GetCellArrayFunc>
void UpdateCellArrayIndices(DataStructure& dataStructure, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetVerticesArrayFunc getVerticesArray, GetCellArrayFunc getCellArray,
                            const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  // Update the cell array indices to correctly map to the expanded vertex array
  uint64 verticesOffset = 0;
  usize cellsOffset = 0;

  auto outputCellArrayPtr = getCellArray(outputGeomPtr);
  for(const auto& inputGeom : inputGeoms)
  {
    if(shouldCancel)
    {
      return;
    }

    auto inputVertexArrayPtr = getVerticesArray(inputGeom);
    auto inputCellArrayPtr = getCellArray(inputGeom);

    // Increase each value in the cell array by the verticesOffset
    // The verticesOffset is simply the total number of vertices for all the preceding geometries
    // The cellsOffset is calculated to make sure we are updating the values at the correct location in the output cell array
    std::transform(outputCellArrayPtr->begin() + cellsOffset, outputCellArrayPtr->begin() + cellsOffset + inputCellArrayPtr->getSize(), outputCellArrayPtr->begin() + cellsOffset,
                   [verticesOffset](uint64 value) -> uint64 { return value + verticesOffset; });
    verticesOffset += inputVertexArrayPtr->getNumberOfTuples();
    cellsOffset += inputCellArrayPtr->getSize();
  }
}

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
Result<> CombineGeometryElements(DataStructure& dataStructure, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetArrayFunc getArray, const std::string& elementsArrayName,
                                 GetAttrMatrixFunc getAttrMatrix, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  // Each entry in this map contains the array name as the key and all the paths to the array in each input geometry
  std::map<std::string, std::vector<DataPath>> inputArrayPathsMap = RetrieveGeometryElementPaths(inputGeoms, getArray, elementsArrayName, getAttrMatrix);

  // Each entry in this map contains the array name as the key and the path to the array in the output geometry
  auto outputGeomPtrs = std::vector<NodeGeomType*>{outputGeomPtr};
  std::map<std::string, std::vector<DataPath>> outputArrayPathsMap = RetrieveGeometryElementPaths(outputGeomPtrs, getArray, elementsArrayName, getAttrMatrix);

  // Calculate the total tuples across all input geometries
  usize totalTuples = CalculateTotalTuples(inputGeoms, getArray);

  // Resize the vertex/cell array
  auto* arrayPtr = getArray(outputGeomPtr);
  Result<> resizeResult = arrayPtr->resizeTuples({totalTuples});
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  // Resize the vertex/cell attribute matrix
  auto* attrMatrixPtr = getAttrMatrix(outputGeomPtr);
  if(attrMatrixPtr != nullptr)
  {
    resizeResult = attrMatrixPtr->resizeTuples({totalTuples});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }

  // For each array name in the map, concatenate all the arrays from the input geometries
  for(const auto& [arrayName, arrayPaths] : inputArrayPathsMap)
  {
    if(shouldCancel)
    {
      return {};
    }

    ConcatenateDataArraysInputValues inputValues;
    inputValues.InputArrayPaths = arrayPaths;
    inputValues.OutputArrayPath = outputArrayPathsMap[arrayName][0];
    auto result = ConcatenateDataArrays(dataStructure, msgHandler, shouldCancel, &inputValues)();
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

/**
 * @brief Concatenates the vertex arrays of every input geometry into the output geometry.
 * @param ds Owns the input and output geometries.
 * @param outputGeomPath Identifies the combined output geometry.
 * @param inputGeometryPaths Identifies the geometries to concatenate, in order.
 * @param msgHandler Receives progress messages.
 * @param shouldCancel Stops before later input geometries when true.
 * @return The first resize or copy error from the underlying stores, or success.
 *
 * The output arrays are resized before the copies, so a storage failure must reach the
 * caller: a discarded error leaves an undersized output that later filters read past.
 */
[[nodiscard]] Result<> CombineVertexElements(DataStructure& dataStructure, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                                             const std::atomic_bool& shouldCancel)
{
  auto getVerticesArrayFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertices(); };
  auto getVertexAttrMatrixFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertexAttributeMatrix(); };

  auto* outputGeom0dPtr = dataStructure.getDataAs<INodeGeometry0D>(outputGeomPath);

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining vertex data..."));
  std::vector<INodeGeometry0D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&dataStructure](const DataPath& path) { return dataStructure.getDataAs<INodeGeometry0D>(path); });
  return CombineGeometryElements<INodeGeometry0D>(dataStructure, outputGeom0dPtr, inputGeoms, getVerticesArrayFunc, INodeGeometry0D::k_SharedVertexListName, getVertexAttrMatrixFunc, msgHandler,
                                                  shouldCancel);
}

/**
 * @brief Concatenates the edge arrays of every input geometry into the output geometry.
 * @param ds Owns the input and output geometries.
 * @param outputGeomPath Identifies the combined output geometry.
 * @param inputGeometryPaths Identifies the geometries to concatenate, in order.
 * @param msgHandler Receives progress messages.
 * @param shouldCancel Stops before later input geometries when true.
 * @return The first resize or copy error from the underlying stores, or success.
 *
 * The output arrays are resized before the copies, so a storage failure must reach the
 * caller: a discarded error leaves an undersized output that later filters read past.
 */
[[nodiscard]] Result<> CombineEdgeElements(DataStructure& dataStructure, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                                           const std::atomic_bool& shouldCancel)
{
  auto getEdgesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdges(); };
  auto getEdgeAttrMatrixFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdgeAttributeMatrix(); };

  auto* outputGeom1dPtr = dataStructure.getDataAs<INodeGeometry1D>(outputGeomPath);
  if(outputGeom1dPtr == nullptr)
  {
    // This geometry is not at least a 1D geometry, so just return
    return {};
  }

  auto* outputEdgesArrayPtr = getEdgesArrayFunc(outputGeom1dPtr);
  if(outputEdgesArrayPtr == nullptr)
  {
    auto* edgesArrayPtr = dataStructure.getDataAs<DataArray<INodeGeometry1D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName));
    if(edgesArrayPtr == nullptr)
    {
      // There are no edges, so just return
      return {};
    }

    // Set the edges array into the geometry (it may have been created via an action in preflight)
    outputGeom1dPtr->setEdgeList(*edgesArrayPtr);
  }

  auto* outputEdgeAttrMatrixPtr = getEdgeAttrMatrixFunc(outputGeom1dPtr);
  if(outputEdgeAttrMatrixPtr == nullptr)
  {
    auto* edgeAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry1D::k_EdgeAttributeMatrixName));
    if(edgeAttrMatrixPtr != nullptr)
    {
      // Set the edge attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom1dPtr->setEdgeAttributeMatrix(*edgeAttrMatrixPtr);
    }
  }

  std::vector<INodeGeometry1D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&dataStructure](const DataPath& path) { return dataStructure.getDataAs<INodeGeometry1D>(path); });

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining edge data..."));
  auto result =
      CombineGeometryElements<INodeGeometry1D>(dataStructure, outputGeom1dPtr, inputGeoms, getEdgesArrayFunc, INodeGeometry1D::k_SharedEdgeListName, getEdgeAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the edges array values.  For example, an edge geometry will need its edges
  // array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating edge values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(dataStructure, outputGeom1dPtr, inputGeoms, getVerticesArrayFunc, getEdgesArrayFunc, msgHandler, shouldCancel);
  return {};
}

/**
 * @brief Concatenates the face arrays of every input geometry into the output geometry.
 * @param ds Owns the input and output geometries.
 * @param outputGeomPath Identifies the combined output geometry.
 * @param inputGeometryPaths Identifies the geometries to concatenate, in order.
 * @param msgHandler Receives progress messages.
 * @param shouldCancel Stops before later input geometries when true.
 * @return The first resize or copy error from the underlying stores, or success.
 *
 * The output arrays are resized before the copies, so a storage failure must reach the
 * caller: a discarded error leaves an undersized output that later filters read past.
 */
[[nodiscard]] Result<> CombineFaceElements(DataStructure& dataStructure, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                                           const std::atomic_bool& shouldCancel)
{
  auto getFacesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaces(); };
  auto getFaceAttrMatrixFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaceAttributeMatrix(); };

  auto* outputGeom2dPtr = dataStructure.getDataAs<INodeGeometry2D>(outputGeomPath);
  if(outputGeom2dPtr == nullptr)
  {
    // This geometry is not at least a 2D geometry, so just return
    return {};
  }

  auto* outputFacesArrayPtr = getFacesArrayFunc(outputGeom2dPtr);
  if(outputFacesArrayPtr == nullptr)
  {
    auto* facesArrayPtr = dataStructure.getDataAs<DataArray<INodeGeometry2D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName));
    if(facesArrayPtr == nullptr)
    {
      // There are no faces, so just return
      return {};
    }

    // Set the faces array into the geometry (it may have been created via an action in preflight)
    outputGeom2dPtr->setFaceList(*facesArrayPtr);
  }

  auto* outputFacesAttrMatrixPtr = getFaceAttrMatrixFunc(outputGeom2dPtr);
  if(outputFacesAttrMatrixPtr == nullptr)
  {
    auto* facesAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName));
    if(facesAttrMatrixPtr != nullptr)
    {
      // Set the face attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom2dPtr->setFaceAttributeMatrix(*facesAttrMatrixPtr);
    }
  }

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining face data..."));
  std::vector<INodeGeometry2D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&dataStructure](const DataPath& path) { return dataStructure.getDataAs<INodeGeometry2D>(path); });
  auto result =
      CombineGeometryElements<INodeGeometry2D>(dataStructure, outputGeom2dPtr, inputGeoms, getFacesArrayFunc, INodeGeometry2D::k_SharedFacesListName, getFaceAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the faces array values.  For example, a triangle geometry will need its faces
  // array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating face values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(dataStructure, outputGeom2dPtr, inputGeoms, getVerticesArrayFunc, getFacesArrayFunc, msgHandler, shouldCancel);
  return {};
}

/**
 * @brief Concatenates the polyhedra arrays of every input geometry into the output geometry.
 * @param ds Owns the input and output geometries.
 * @param outputGeomPath Identifies the combined output geometry.
 * @param inputGeometryPaths Identifies the geometries to concatenate, in order.
 * @param msgHandler Receives progress messages.
 * @param shouldCancel Stops before later input geometries when true.
 * @return The first resize or copy error from the underlying stores, or success.
 *
 * The output arrays are resized before the copies, so a storage failure must reach the
 * caller: a discarded error leaves an undersized output that later filters read past.
 */
[[nodiscard]] Result<> CombinePolyElements(DataStructure& dataStructure, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                                           const std::atomic_bool& shouldCancel)
{
  auto getPolyArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedra(); };
  auto getPolyAttrMatrixFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedraAttributeMatrix(); };

  auto* outputGeom3dPtr = dataStructure.getDataAs<INodeGeometry3D>(outputGeomPath);
  if(outputGeom3dPtr == nullptr)
  {
    // This geometry is not at least a 3D geometry, so just return
    return {};
  }

  auto* outputPolyhedraArrayPtr = getPolyArrayFunc(outputGeom3dPtr);
  if(outputPolyhedraArrayPtr == nullptr)
  {
    auto* polyArrayPtr = dataStructure.getDataAs<DataArray<INodeGeometry3D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry3D::k_SharedPolyhedronListName));
    if(polyArrayPtr == nullptr)
    {
      // There are no polyhedra, so just return
      return {};
    }

    // Set the polyhedra array into the geometry (it may have been created via an action in preflight)
    outputGeom3dPtr->setPolyhedraList(*polyArrayPtr);
  }

  auto* outputPolyAttrMatrixPtr = getPolyAttrMatrixFunc(outputGeom3dPtr);
  if(outputPolyAttrMatrixPtr == nullptr)
  {
    auto* polyAttrMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName));
    if(polyAttrMatrixPtr != nullptr)
    {
      // Set the polyhedra attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom3dPtr->setPolyhedraAttributeMatrix(*polyAttrMatrixPtr);
    }
  }

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining polyhedron data..."));
  std::vector<INodeGeometry3D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&dataStructure](const DataPath& path) { return dataStructure.getDataAs<INodeGeometry3D>(path); });
  auto result = CombineGeometryElements<INodeGeometry3D>(dataStructure, outputGeom3dPtr, inputGeoms, getPolyArrayFunc, INodeGeometry3D::k_SharedPolyhedronListName, getPolyAttrMatrixFunc, msgHandler,
                                                         shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the polyhedra array values.  For example, a tetrahedral geometry will need its
  // polyhedra array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating polyhedron values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(dataStructure, outputGeom3dPtr, inputGeoms, getVerticesArrayFunc, getPolyArrayFunc, msgHandler, shouldCancel);
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
CombineNodeBasedGeometries::CombineNodeBasedGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                       CombineNodeBasedGeometriesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineNodeBasedGeometries::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CombineNodeBasedGeometries::operator()()
{
  // Each stage resizes the output arrays before it copies into them, so the first failure
  // must stop the chain: continuing would copy into arrays that were never resized.
  Result<> vertexResult = CombineVertexElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  if(vertexResult.invalid())
  {
    return vertexResult;
  }

  Result<> edgeResult = CombineEdgeElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  if(edgeResult.invalid())
  {
    return edgeResult;
  }

  Result<> faceResult = CombineFaceElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  if(faceResult.invalid())
  {
    return faceResult;
  }

  return CombinePolyElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
}
