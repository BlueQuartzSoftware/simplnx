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
  auto* quadSizes = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(quadSizes != nullptr && !recalculate)
  {
    return {};
  }

  if(quadSizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    quadSizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());

    if(quadSizes == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2630, "QuadGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  GeometryHelpers::Topology::Find2DElementAreas(getFaces(), getVertices(), quadSizes);
  m_ElementSizesId = quadSizes->getId();

  return {};
}

Result<> QuadGeom::findElementsContainingVert(bool recalculate)
{
  auto* quadsContainingVert = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(quadsContainingVert != nullptr && !recalculate)
  {
    return {};
  }

  if(quadsContainingVert == nullptr)
  {
    quadsContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());

    if(quadsContainingVert == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2631, "QuadGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), quadsContainingVert, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = quadsContainingVert->getId();

  return {};
}

Result<> QuadGeom::findElementNeighbors(bool recalculate)
{
  auto* quadNeighbors = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(quadNeighbors != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }

  if(quadNeighbors == nullptr)
  {
    quadNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(quadNeighbors == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2632, "QuadGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = quadNeighbors->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), quadNeighbors, Type::Quad);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> QuadGeom::findElementCentroids(bool recalculate)
{
  auto* quadCentroids = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(quadCentroids != nullptr && !recalculate)
  {
    return {};
  }

  if(quadCentroids == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    quadCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
    if(quadCentroids == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2633, "QuadGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  GeometryHelpers::Topology::FindElementCentroids(getFaces(), getVertices(), quadCentroids);
  m_CellCentroidsDataArrayId = quadCentroids->getId();

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
  auto* edgeList = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_EdgeDataArrayId);
  if(edgeList != nullptr && !recalculate)
  {
    return {};
  }

  if(edgeList == nullptr)
  {
    edgeList = createSharedEdgeList(0);
    if(edgeList == nullptr)
    {
      m_EdgeDataArrayId.reset();
      return MakeErrorResult(-2634, "QuadGeom Error: Unable to find or create a valid shared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::Find2DElementEdges(getFaces(), edgeList);
  if(findResult.invalid())
  {
    m_EdgeDataArrayId.reset();
    return findResult;
  }
  m_EdgeDataArrayId = edgeList->getId();

  return {};
}

Result<> QuadGeom::findUnsharedEdges(bool recalculate)
{
  auto* unsharedEdgeList = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_UnsharedEdgeListId);
  if(unsharedEdgeList != nullptr && !recalculate)
  {
    return {};
  }

  if(unsharedEdgeList == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
    unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedEdgesListName, std::move(dataStore), getId());

    if(unsharedEdgeList == nullptr)
    {
      m_UnsharedEdgeListId.reset();
      return MakeErrorResult(-2635, "QuadGeom Error: Unable to find or create a valid unshared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::Find2DUnsharedEdges<MeshIndexType>(getFaces(), unsharedEdgeList);
  if(findResult.invalid())
  {
    m_UnsharedEdgeListId.reset();
    return findResult;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();

  return {};
}
