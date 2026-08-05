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

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
template <typename NodeGeomType, typename GetArrayFunc>
usize CalculateTotalTuples(std::vector<NodeGeomType*>& geomPtrs, GetArrayFunc getArray)
{
  usize tupleCount = 0;
  for(const auto& geomPtr : geomPtrs)
  {
    auto* array = getArray(geomPtr);
    tupleCount += array->getNumberOfTuples();
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
    AttributeMatrix* attrMatrix = getAttrMatrix(geomPtr);
    if(attrMatrix != nullptr)
    {
      for(const auto& pair : *attrMatrix)
      {
        auto& pathsVector = pathsMap[pair.second->getName()];
        pathsVector.push_back(pair.second->getDataPaths()[0]);
      }
    }

    // Retrieve the array path (edges, faces, polyhedra, etc.)
    auto* array = getArray(geomPtr);
    auto& pathsVector = pathsMap[elementsArrayName];
    pathsVector.push_back(array->getDataPaths()[0]);
  }

  return pathsMap;
}

template <typename NodeGeomType, typename GetVerticesArrayFunc, typename GetCellArrayFunc>
void UpdateCellArrayIndices(DataStructure& ds, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetVerticesArrayFunc getVerticesArray, GetCellArrayFunc getCellArray,
                            const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel)
{
  // Update the cell array indices to correctly map to the expanded vertex array
  uint64 verticesOffset = 0;
  usize cellsOffset = 0;

  auto outputCellArray = getCellArray(outputGeomPtr);
  for(const auto& inputGeom : inputGeoms)
  {
    if(shouldCancel)
    {
      return;
    }

    auto inputVertexArray = getVerticesArray(inputGeom);
    auto inputCellArray = getCellArray(inputGeom);

    // Increase each value in the cell array by the verticesOffset
    // The verticesOffset is simply the total number of vertices for all the preceding geometries
    // The cellsOffset is calculated to make sure we are updating the values at the correct location in the output cell array
    std::transform(outputCellArray->begin() + cellsOffset, outputCellArray->begin() + cellsOffset + inputCellArray->getSize(), outputCellArray->begin() + cellsOffset,
                   [verticesOffset](uint64 value) -> uint64 { return value + verticesOffset; });
    verticesOffset += inputVertexArray->getNumberOfTuples();
    cellsOffset += inputCellArray->getSize();
  }
}

template <typename NodeGeomType, typename GetArrayFunc, typename GetAttrMatrixFunc>
Result<> CombineGeometryElements(DataStructure& ds, NodeGeomType* outputGeomPtr, std::vector<NodeGeomType*>& inputGeoms, GetArrayFunc getArray, const std::string& elementsArrayName,
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
  auto* array = getArray(outputGeomPtr);
  array->resizeTuples({totalTuples});

  // Resize the vertex/cell attribute matrix
  auto* attrMatrix = getAttrMatrix(outputGeomPtr);
  if(attrMatrix != nullptr)
  {
    attrMatrix->resizeTuples({totalTuples});
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
    auto result = ConcatenateDataArrays(ds, msgHandler, shouldCancel, &inputValues)();
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> CombineVertexElements(DataStructure& ds, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                               const std::atomic_bool& shouldCancel)
{
  auto getVerticesArrayFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertices(); };
  auto getVertexAttrMatrixFunc = [](INodeGeometry0D* ptr) -> auto { return ptr->getVertexAttributeMatrix(); };

  auto* outputGeom0d = ds.getDataAs<INodeGeometry0D>(outputGeomPath);

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining vertex data..."));
  std::vector<INodeGeometry0D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry0D>(path); });
  return CombineGeometryElements<INodeGeometry0D>(ds, outputGeom0d, inputGeoms, getVerticesArrayFunc, INodeGeometry0D::k_SharedVertexListName, getVertexAttrMatrixFunc, msgHandler, shouldCancel);
}

Result<> CombineEdgeElements(DataStructure& ds, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                             const std::atomic_bool& shouldCancel)
{
  auto getEdgesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdges(); };
  auto getEdgeAttrMatrixFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getEdgeAttributeMatrix(); };

  auto* outputGeom1d = ds.getDataAs<INodeGeometry1D>(outputGeomPath);
  if(outputGeom1d == nullptr)
  {
    // This geometry is not at least a 1D geometry, so just return
    return {};
  }

  auto* outputEdgesArray = getEdgesArrayFunc(outputGeom1d);
  if(outputEdgesArray == nullptr)
  {
    auto* edgesArray = ds.getDataAs<DataArray<INodeGeometry1D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry1D::k_SharedEdgeListName));
    if(edgesArray == nullptr)
    {
      // There are no edges, so just return
      return {};
    }

    // Set the edges array into the geometry (it may have been created via an action in preflight)
    outputGeom1d->setEdgeList(*edgesArray);
  }

  auto* outputEdgeAttrMatrix = getEdgeAttrMatrixFunc(outputGeom1d);
  if(outputEdgeAttrMatrix == nullptr)
  {
    auto* edgeAttrMatrix = ds.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry1D::k_EdgeAttributeMatrixName));
    if(edgeAttrMatrix != nullptr)
    {
      // Set the edge attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom1d->setEdgeAttributeMatrix(*edgeAttrMatrix);
    }
  }

  std::vector<INodeGeometry1D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry1D>(path); });

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining edge data..."));
  auto result = CombineGeometryElements<INodeGeometry1D>(ds, outputGeom1d, inputGeoms, getEdgesArrayFunc, INodeGeometry1D::k_SharedEdgeListName, getEdgeAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the edges array values.  For example, an edge geometry will need its edges
  // array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating edge values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry1D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom1d, inputGeoms, getVerticesArrayFunc, getEdgesArrayFunc, msgHandler, shouldCancel);
  return {};
}

Result<> CombineFaceElements(DataStructure& ds, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                             const std::atomic_bool& shouldCancel)
{
  auto getFacesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaces(); };
  auto getFaceAttrMatrixFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getFaceAttributeMatrix(); };

  auto* outputGeom2d = ds.getDataAs<INodeGeometry2D>(outputGeomPath);
  if(outputGeom2d == nullptr)
  {
    // This geometry is not at least a 2D geometry, so just return
    return {};
  }

  auto* outputFacesArray = getFacesArrayFunc(outputGeom2d);
  if(outputFacesArray == nullptr)
  {
    auto* facesArray = ds.getDataAs<DataArray<INodeGeometry2D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName));
    if(facesArray == nullptr)
    {
      // There are no faces, so just return
      return {};
    }

    // Set the faces array into the geometry (it may have been created via an action in preflight)
    outputGeom2d->setFaceList(*facesArray);
  }

  auto* outputFacesAttrMatrix = getFaceAttrMatrixFunc(outputGeom2d);
  if(outputFacesAttrMatrix == nullptr)
  {
    auto* facesAttrMatrix = ds.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry2D::k_FaceAttributeMatrixName));
    if(facesAttrMatrix != nullptr)
    {
      // Set the face attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom2d->setFaceAttributeMatrix(*facesAttrMatrix);
    }
  }

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining face data..."));
  std::vector<INodeGeometry2D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry2D>(path); });
  auto result = CombineGeometryElements<INodeGeometry2D>(ds, outputGeom2d, inputGeoms, getFacesArrayFunc, INodeGeometry2D::k_SharedFacesListName, getFaceAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the faces array values.  For example, a triangle geometry will need its faces
  // array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating face values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry2D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom2d, inputGeoms, getVerticesArrayFunc, getFacesArrayFunc, msgHandler, shouldCancel);
  return {};
}

Result<> CombinePolyElements(DataStructure& ds, const DataPath& outputGeomPath, const std::vector<DataPath>& inputGeometryPaths, const IFilter::MessageHandler& msgHandler,
                             const std::atomic_bool& shouldCancel)
{
  auto getPolyArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedra(); };
  auto getPolyAttrMatrixFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getPolyhedraAttributeMatrix(); };

  auto* outputGeom3d = ds.getDataAs<INodeGeometry3D>(outputGeomPath);
  if(outputGeom3d == nullptr)
  {
    // This geometry is not at least a 3D geometry, so just return
    return {};
  }

  auto* outputPolyhedraArray = getPolyArrayFunc(outputGeom3d);
  if(outputPolyhedraArray == nullptr)
  {
    auto* polyArray = ds.getDataAs<DataArray<INodeGeometry3D::MeshIndexType>>(outputGeomPath.createChildPath(INodeGeometry3D::k_SharedPolyhedronListName));
    if(polyArray == nullptr)
    {
      // There are no polyhedra, so just return
      return {};
    }

    // Set the polyhedra array into the geometry (it may have been created via an action in preflight)
    outputGeom3d->setPolyhedraList(*polyArray);
  }

  auto* outputPolyAttrMatrix = getPolyAttrMatrixFunc(outputGeom3d);
  if(outputPolyAttrMatrix == nullptr)
  {
    auto* polyAttrMatrix = ds.getDataAs<AttributeMatrix>(outputGeomPath.createChildPath(INodeGeometry3D::k_PolyhedronDataName));
    if(polyAttrMatrix != nullptr)
    {
      // Set the polyhedra attribute matrix into the geometry (it may have been created via an action in preflight)
      outputGeom3d->setPolyhedraAttributeMatrix(*polyAttrMatrix);
    }
  }

  // Combine the data
  msgHandler.sendInfoMessage(fmt::format("Combining polyhedron data..."));
  std::vector<INodeGeometry3D*> inputGeoms(inputGeometryPaths.size());
  std::transform(inputGeometryPaths.begin(), inputGeometryPaths.end(), inputGeoms.begin(), [&ds](const DataPath& path) { return ds.getDataAs<INodeGeometry3D>(path); });
  auto result = CombineGeometryElements<INodeGeometry3D>(ds, outputGeom3d, inputGeoms, getPolyArrayFunc, INodeGeometry3D::k_SharedPolyhedronListName, getPolyAttrMatrixFunc, msgHandler, shouldCancel);
  if(result.invalid())
  {
    return result;
  }

  // Update the polyhedra array values.  For example, a tetrahedral geometry will need its
  // polyhedra array updated since all the vertex indices will be different after concatenation
  msgHandler.sendInfoMessage(fmt::format("Updating polyhedron values to use new vertex indices..."));
  auto getVerticesArrayFunc = [](INodeGeometry3D* ptr) -> auto { return ptr->getVertices(); };
  UpdateCellArrayIndices(ds, outputGeom3d, inputGeoms, getVerticesArrayFunc, getPolyArrayFunc, msgHandler, shouldCancel);
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
CombineNodeBasedGeometries::~CombineNodeBasedGeometries() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineNodeBasedGeometries::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void CombineNodeBasedGeometries::sendMessage(const std::string& message)
{
  m_MessageHandler.sendInfoMessage(message);
}

// -----------------------------------------------------------------------------
Result<> CombineNodeBasedGeometries::operator()()
{
  CombineVertexElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombineEdgeElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombineFaceElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);
  CombinePolyElements(m_DataStructure, m_InputValues->OutputGeometryPath, m_InputValues->InputGeometryPaths, m_MessageHandler, m_ShouldCancel);

  return {};
}
