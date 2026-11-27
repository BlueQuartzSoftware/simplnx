#include "EdgeGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

using namespace nx::core;

EdgeGeom::EdgeGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry1D(dataStructure, std::move(name))
{
  m_UnitDimensionality = 1;
}

EdgeGeom::EdgeGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry1D(dataStructure, std::move(name), importId)
{
  m_UnitDimensionality = 1;
}

DataObject::Type EdgeGeom::getDataObjectType() const
{
  return DataObject::Type::EdgeGeom;
}

EdgeGeom* EdgeGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

EdgeGeom* EdgeGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type EdgeGeom::getGeomType() const
{
  return IGeometry::Type::Edge;
}

BaseGroup::GroupType EdgeGeom::getGroupType() const
{
  return GroupType::EdgeGeom;
}

std::string EdgeGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* EdgeGeom::shallowCopy()
{
  return new EdgeGeom(*this);
}

std::shared_ptr<DataObject> EdgeGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Construct without an identifier because insertion creates it.
  auto copy = std::shared_ptr<EdgeGeom>(new EdgeGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    INodeGeometry1D::copyMembersInto(*copy, copyPath);
    copy->m_EdgeDataArrayId = deepCopyOwnedChild(copyPath, getEdges());
    copy->m_CellContainingVertDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltsContainingVert);
    copy->m_CellNeighborsDataArrayId = adoptCopiedChild<ElementDynamicList>(copyPath, k_EltNeighbors);
    copy->m_CellCentroidsDataArrayId = adoptCopiedChild<Float32Array>(copyPath, k_EltCentroids);

    return copy;
  }
  return nullptr;
}

Result<> EdgeGeom::findElementSizes(bool recalculate)
{
  auto* sizesPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(sizesPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(sizesPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    sizesPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(sizesPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2430, "EdgeGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  m_ElementSizesId = sizesPtr->getId();

  std::array<FloatVec3, 2> verts = {FloatVec3(0.0f, 0.0f, 0.0f), FloatVec3(0.0f, 0.0f, 0.0f)};

  for(usize i = 0; i < INodeGeometry1D::getNumberOfCells(); i++)
  {
    getEdgeCoordinates(i, verts);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (verts[0][j] - verts[1][j]) * (verts[0][j] - verts[1][j]);
    }
    (*sizesPtr)[i] = std::sqrt(length);
  }

  return {};
}

usize EdgeGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

Result<> EdgeGeom::findElementsContainingVert(bool recalculate)
{
  auto* edgesContainingVertPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(edgesContainingVertPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(edgesContainingVertPtr == nullptr)
  {
    edgesContainingVertPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
    if(edgesContainingVertPtr == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2431, "EdgeGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), edgesContainingVertPtr, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = edgesContainingVertPtr->getId();

  return {};
}

Result<> EdgeGeom::findElementNeighbors(bool recalculate)
{
  auto* edgeNeighborsPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(edgeNeighborsPtr != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }
  if(edgeNeighborsPtr == nullptr)
  {
    edgeNeighborsPtr = ElementDynamicList::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(edgeNeighborsPtr == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2432, "EdgeGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = edgeNeighborsPtr->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighborsPtr, Type::Edge);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> EdgeGeom::findElementCentroids(bool recalculate)
{
  auto* edgeCentroidsPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(edgeCentroidsPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(edgeCentroidsPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    edgeCentroidsPtr = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
    if(edgeCentroidsPtr == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2433, "EdgeGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroidsPtr);
  m_CellCentroidsDataArrayId = edgeCentroidsPtr->getId();

  return {};
}

Vec3<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions([[maybe_unused]] const Vec3<float64>& pCoords, float64* shape) const
{
  shape[0] = -1.0;
  shape[1] = 1.0;
}
