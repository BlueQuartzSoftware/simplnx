#include "TetrahedralGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

TetrahedralGeom::TetrahedralGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry3D(dataStructure, std::move(name))
{
}

TetrahedralGeom::TetrahedralGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry3D(dataStructure, std::move(name), importId)
{
}

IGeometry::Type TetrahedralGeom::getGeomType() const
{
  return IGeometry::Type::Tetrahedral;
}

DataObject::Type TetrahedralGeom::getDataObjectType() const
{
  return DataObject::Type::TetrahedralGeom;
}

BaseGroup::GroupType TetrahedralGeom::getGroupType() const
{
  return GroupType::TetrahedralGeom;
}

usize TetrahedralGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

TetrahedralGeom* TetrahedralGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TetrahedralGeom* TetrahedralGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TetrahedralGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* TetrahedralGeom::shallowCopy()
{
  return new TetrahedralGeom(*this);
}

std::shared_ptr<DataObject> TetrahedralGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Construct without an identifier because insertion creates it.
  auto copy = std::shared_ptr<TetrahedralGeom>(new TetrahedralGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    INodeGeometry3D::copyMembersInto(*copy, copyPath);
    copy->m_PolyhedronListId = deepCopyOwnedChild(copyPath, getPolyhedra());
    copy->m_CellContainingVertDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltsContainingVert);
    copy->m_CellNeighborsDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltNeighbors);
    copy->m_CellCentroidsDataArrayId = adoptCopiedChild<Float32Array>(copyPath, k_EltCentroids);

    return copy;
  }
  return nullptr;
}

usize TetrahedralGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

usize TetrahedralGeom::getNumberOfVerticesPerCell() const
{
  return k_NumVerts;
}

usize TetrahedralGeom::getNumberOfCells() const
{
  const auto& tets = getPolyhedraRef();
  return tets.getNumberOfTuples();
}

Result<> TetrahedralGeom::findElementSizes(bool recalculate)
{
  auto* tetSizesPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(tetSizesPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(tetSizesPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
    tetSizesPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(tetSizesPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2130, "TetrahedralGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  GeometryHelpers::Topology::FindTetVolumes(getPolyhedra(), getVertices(), tetSizesPtr);
  m_ElementSizesId = tetSizesPtr->getId();

  return {};
}

Result<> TetrahedralGeom::findElementsContainingVert(bool recalculate)
{
  auto* tetsContainingVertPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(tetsContainingVertPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(tetsContainingVertPtr == nullptr)
  {
    tetsContainingVertPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
    if(tetsContainingVertPtr == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2131, "TetrahedralGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getPolyhedra(), tetsContainingVertPtr, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = tetsContainingVertPtr->getId();

  return {};
}

Result<> TetrahedralGeom::findElementNeighbors(bool recalculate)
{
  auto* tetNeighborsPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(tetNeighborsPtr != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }

  if(tetNeighborsPtr == nullptr)
  {
    tetNeighborsPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(tetNeighborsPtr == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2132, "TetrahedralGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = tetNeighborsPtr->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getPolyhedra(), getElementsContainingVert(), tetNeighborsPtr, Type::Tetrahedral);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> TetrahedralGeom::findElementCentroids(bool recalculate)
{
  auto* tetCentroidsPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(tetCentroidsPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(tetCentroidsPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    tetCentroidsPtr = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());

    if(tetCentroidsPtr == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2133, "TetrahedralGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  GeometryHelpers::Topology::FindElementCentroids(getPolyhedra(), getVertices(), tetCentroidsPtr);
  m_CellCentroidsDataArrayId = tetCentroidsPtr->getId();

  return {};
}

Point3D<float64> TetrahedralGeom::getParametricCenter() const
{
  return {0.25, 0.25, 0.25};
}

void TetrahedralGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, double* shape) const
{
  // r-derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;
  shape[3] = 0.0;

  // s-derivatives
  shape[4] = -1.0;
  shape[5] = 0.0;
  shape[6] = 1.0;
  shape[7] = 0.0;

  // t-derivatives
  shape[8] = -1.0;
  shape[9] = 0.0;
  shape[10] = 0.0;
  shape[11] = 1.0;
}

Result<> TetrahedralGeom::findEdges(bool recalculate)
{
  auto* edgeListPtr = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_EdgeDataArrayId);
  if(edgeListPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(edgeListPtr == nullptr)
  {
    edgeListPtr = createSharedEdgeList(0);
    if(edgeListPtr == nullptr)
    {
      m_EdgeDataArrayId.reset();
      return MakeErrorResult(-2134, "TetrahedralGeom Error: Unable to find or create a valid shared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindTetEdges(getPolyhedra(), edgeListPtr);
  if(findResult.invalid())
  {
    m_EdgeDataArrayId.reset();
    return findResult;
  }
  m_EdgeDataArrayId = edgeListPtr->getId();

  return {};
}

Result<> TetrahedralGeom::findFaces(bool recalculate)
{
  auto* triListPtr = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_FaceListId);
  if(triListPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(triListPtr == nullptr)
  {
    triListPtr = createSharedTriList(0);
    if(triListPtr == nullptr)
    {
      m_FaceListId.reset();
      return MakeErrorResult(-2135, "TetrahedralGeom Error: Unable to find or create a valid shared faces array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindTetFaces(getPolyhedra(), triListPtr);
  if(findResult.invalid())
  {
    m_FaceListId.reset();
    return findResult;
  }
  m_FaceListId = triListPtr->getId();

  return {};
}

Result<> TetrahedralGeom::findUnsharedEdges(bool recalculate)
{
  auto* unsharedEdgeListPtr = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_UnsharedEdgeListId);
  if(unsharedEdgeListPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(unsharedEdgeListPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
    unsharedEdgeListPtr = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedEdgesListName, std::move(dataStore), getId());
    if(unsharedEdgeListPtr == nullptr)
    {
      m_UnsharedEdgeListId.reset();
      return MakeErrorResult(-2136, "TetrahedralGeom Error: Unable to find or create a valid unshared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindUnsharedTetEdges<MeshIndexType>(getPolyhedra(), unsharedEdgeListPtr);
  if(findResult.invalid())
  {
    m_UnsharedEdgeListId.reset();
    return findResult;
  }
  m_UnsharedEdgeListId = unsharedEdgeListPtr->getId();

  return {};
}

Result<> TetrahedralGeom::findUnsharedFaces(bool recalculate)
{
  auto* unsharedTriListPtr = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_UnsharedFaceListId);
  if(unsharedTriListPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(unsharedTriListPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{3}, 0);
    unsharedTriListPtr = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedFacesListName, std::move(dataStore), getId());
    if(unsharedTriListPtr == nullptr)
    {
      m_UnsharedFaceListId.reset();
      return MakeErrorResult(-2137, "TetrahedralGeom Error: Unable to find or create a valid unshared faces array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindUnsharedTetFaces<MeshIndexType>(getPolyhedra(), unsharedTriListPtr);
  if(findResult.invalid())
  {
    m_UnsharedFaceListId.reset();
    return findResult;
  }
  m_UnsharedFaceListId = unsharedTriListPtr->getId();

  return {};
}
