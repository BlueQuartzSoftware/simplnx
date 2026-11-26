#include "QuadGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

QuadGeom::QuadGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry2D(dataStructure, std::move(name))
{
  m_UnitDimensionality = 2;
}

QuadGeom::QuadGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry2D(dataStructure, std::move(name), importId)
{
  m_UnitDimensionality = 2;
}

IGeometry::Type QuadGeom::getGeomType() const
{
  return IGeometry::Type::Quad;
}

DataObject::Type QuadGeom::getDataObjectType() const
{
  return DataObject::Type::QuadGeom;
}

BaseGroup::GroupType QuadGeom::getGroupType() const
{
  return GroupType::QuadGeom;
}

QuadGeom* QuadGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

QuadGeom* QuadGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<QuadGeom>(new QuadGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string QuadGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* QuadGeom::shallowCopy()
{
  return new QuadGeom(*this);
}

std::shared_ptr<DataObject> QuadGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Construct without an identifier because insertion creates it.
  auto copy = std::shared_ptr<QuadGeom>(new QuadGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    INodeGeometry2D::copyMembersInto(*copy, copyPath);
    copy->m_FaceListId = deepCopyOwnedChild(copyPath, getFaces());
    copy->m_CellContainingVertDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltsContainingVert);
    copy->m_CellNeighborsDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltNeighbors);
    copy->m_CellCentroidsDataArrayId = adoptCopiedChild<Float32Array>(copyPath, k_EltCentroids);

    return copy;
  }
  return nullptr;
}

usize QuadGeom::getNumberOfCells() const
{
  return getNumberOfFaces();
}

usize QuadGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

usize QuadGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

Result<> QuadGeom::findElementSizes(bool recalculate)
{
  auto* quadSizesPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(quadSizesPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(quadSizesPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    quadSizesPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());

    if(quadSizesPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2630, "QuadGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), quadSizesPtr);
  m_ElementSizesId = quadSizesPtr->getId();

  return {};
}

Result<> QuadGeom::findElementsContainingVert(bool recalculate)
{
  auto* quadsContainingVertPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(quadsContainingVertPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(quadsContainingVertPtr == nullptr)
  {
    quadsContainingVertPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());

    if(quadsContainingVertPtr == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2631, "QuadGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), quadsContainingVertPtr, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = quadsContainingVertPtr->getId();

  return {};
}

Result<> QuadGeom::findElementNeighbors(bool recalculate)
{
  auto* quadNeighborsPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(quadNeighborsPtr != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }

  if(quadNeighborsPtr == nullptr)
  {
    quadNeighborsPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(quadNeighborsPtr == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2632, "QuadGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = quadNeighborsPtr->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), quadNeighborsPtr, Type::Quad);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> QuadGeom::findElementCentroids(bool recalculate)
{
  auto* quadCentroidsPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(quadCentroidsPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(quadCentroidsPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    quadCentroidsPtr = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
    if(quadCentroidsPtr == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2633, "QuadGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), quadCentroidsPtr);
  m_CellCentroidsDataArrayId = quadCentroidsPtr->getId();

  return {};
}

Point3D<float64> QuadGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.0};
}

void QuadGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
{
  float64 rm = 1.0 - pCoords[0];
  float64 sm = 1.0 - pCoords[1];

  shape[0] = -sm;
  shape[1] = sm;
  shape[2] = pCoords[1];
  shape[3] = -pCoords[1];
  shape[4] = -rm;
  shape[5] = -pCoords[0];
  shape[6] = pCoords[0];
  shape[7] = rm;
}

Result<> QuadGeom::findEdges(bool recalculate)
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
      return MakeErrorResult(-2634, "QuadGeom Error: Unable to find or create a valid shared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeListPtr);
  if(findResult.invalid())
  {
    m_EdgeDataArrayId.reset();
    return findResult;
  }
  m_EdgeDataArrayId = edgeListPtr->getId();

  return {};
}

Result<> QuadGeom::findUnsharedEdges(bool recalculate)
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
      return MakeErrorResult(-2635, "QuadGeom Error: Unable to find or create a valid unshared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::Find2DUnsharedEdges<MeshIndexType>(getFaces(), unsharedEdgeListPtr);
  if(findResult.invalid())
  {
    m_UnsharedEdgeListId.reset();
    return findResult;
  }
  m_UnsharedEdgeListId = unsharedEdgeListPtr->getId();

  return {};
}
