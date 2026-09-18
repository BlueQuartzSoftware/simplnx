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
  auto* sizes = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(sizes != nullptr && !recalculate)
  {
    return {};
  }

  if(sizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(getNumberOfCells(), 0.0f);
    sizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(sizes == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2430, "EdgeGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  m_ElementSizesId = sizes->getId();

  std::array<Point3Df, 2> verts = {Point3Df(0.0f, 0.0f, 0.0f), Point3Df(0.0f, 0.0f, 0.0f)};

  for(usize i = 0; i < INodeGeometry1D::getNumberOfCells(); i++)
  {
    getEdgeCoordinates(i, verts);
    float32 length = 0.0f;
    for(usize j = 0; j < 3; j++)
    {
      length += (verts[0][j] - verts[1][j]) * (verts[0][j] - verts[1][j]);
    }
    (*sizes)[i] = std::sqrt(length);
  }

  return {};
}

usize EdgeGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

Result<> EdgeGeom::findElementsContainingVert(bool recalculate)
{
  auto* edgesContainingVert = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(edgesContainingVert != nullptr && !recalculate)
  {
    return {};
  }

  if(edgesContainingVert == nullptr)
  {
    edgesContainingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
    if(edgesContainingVert == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2431, "EdgeGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getEdges(), edgesContainingVert, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = edgesContainingVert->getId();

  return {};
}

Result<> EdgeGeom::findElementNeighbors(bool recalculate)
{
  auto* edgeNeighbors = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(edgeNeighbors != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }
  if(edgeNeighbors == nullptr)
  {
    edgeNeighbors = ElementDynamicList::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(edgeNeighbors == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2432, "EdgeGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = edgeNeighbors->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getEdges(), getElementsContainingVert(), edgeNeighbors, Type::Edge);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> EdgeGeom::findElementCentroids(bool recalculate)
{
  auto* edgeCentroids = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(edgeCentroids != nullptr && !recalculate)
  {
    return {};
  }

  if(edgeCentroids == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    edgeCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
    if(edgeCentroids == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2433, "EdgeGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  GeometryHelpers::Topology::FindElementCentroids(getEdges(), getVertices(), edgeCentroids);
  m_CellCentroidsDataArrayId = edgeCentroids->getId();

  return {};
}

Point3D<float64> EdgeGeom::getParametricCenter() const
{
  return {0.5, 0.0, 0.0};
}

void EdgeGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = -1.0;
  shape[1] = 1.0;
}
