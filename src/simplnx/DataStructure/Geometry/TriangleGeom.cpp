#include "TriangleGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

TriangleGeom::TriangleGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry2D(dataStructure, std::move(name))
{
  m_UnitDimensionality = 2;
}

TriangleGeom::TriangleGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry2D(dataStructure, std::move(name), importId)
{
  m_UnitDimensionality = 2;
}

IGeometry::Type TriangleGeom::getGeomType() const
{
  return IGeometry::Type::Triangle;
}

DataObject::Type TriangleGeom::getDataObjectType() const
{
  return DataObject::Type::TriangleGeom;
}

BaseGroup::GroupType TriangleGeom::getGroupType() const
{
  return GroupType::TriangleGeom;
}

usize TriangleGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

TriangleGeom* TriangleGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

TriangleGeom* TriangleGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<TriangleGeom>(new TriangleGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string TriangleGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* TriangleGeom::shallowCopy()
{
  return new TriangleGeom(*this);
}

std::shared_ptr<DataObject> TriangleGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Construct without an identifier because insertion creates it.
  auto copy = std::shared_ptr<TriangleGeom>(new TriangleGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    INodeGeometry2D::copyMembersInto(*copy, copyPath);
    copy->m_FaceListId = deepCopyOwnedChild(copyPath, getFaces());
    copy->m_CellContainingVertDataArrayId = adoptCopiedChild<ElementDynamicListType>(copyPath, k_EltsContainingVert);
    copy->m_CellNeighborsDataArrayId = adoptCopiedChild<ElementDynamicListType>(copyPath, k_EltNeighbors);
    copy->m_CellCentroidsDataArrayId = adoptCopiedChild<Float32ArrayType>(copyPath, k_EltCentroids);

    return copy;
  }
  return nullptr;
}

usize TriangleGeom::getNumberOfCells() const
{
  return getNumberOfFaces();
}

usize TriangleGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

Result<> TriangleGeom::findElementSizes(bool recalculate)
{
  auto* triangleSizesPtr = getDataStructureRef().getDataAsUnsafe<Float32ArrayType>(m_ElementSizesId);
  if(triangleSizesPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(triangleSizesPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfFaces()}, std::vector<usize>{1}, 0.0f);
    triangleSizesPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(triangleSizesPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2230, "TriangleGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  geometry_helpers::topology::Find2DElementAreas(getFaces(), getVertices(), triangleSizesPtr);
  m_ElementSizesId = triangleSizesPtr->getId();

  return {};
}

Result<> TriangleGeom::findElementsContainingVert(bool recalculate)
{
  auto* trianglesContainingVertPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicListType>(m_CellContainingVertDataArrayId);
  if(trianglesContainingVertPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(trianglesContainingVertPtr == nullptr)
  {
    trianglesContainingVertPtr = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
    if(trianglesContainingVertPtr == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2231, "TriangleGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  auto findResult = geometry_helpers::connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getFaces(), trianglesContainingVertPtr, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }
  m_CellContainingVertDataArrayId = trianglesContainingVertPtr->getId();

  return {};
}

Result<> TriangleGeom::findElementNeighbors(bool recalculate)
{
  auto* triangleNeighborsPtr = getDataStructureRef().getDataAsUnsafe<ElementDynamicListType>(m_CellNeighborsDataArrayId);
  if(triangleNeighborsPtr != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }
  if(triangleNeighborsPtr == nullptr)
  {
    triangleNeighborsPtr = ElementDynamicListType::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(triangleNeighborsPtr == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2232, "TriangleGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = triangleNeighborsPtr->getId();

  auto findResult = geometry_helpers::connectivity::FindElementNeighbors<uint16, MeshIndexType>(getFaces(), getElementsContainingVert(), triangleNeighborsPtr, Type::Triangle);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> TriangleGeom::findElementCentroids(bool recalculate)
{
  auto* triangleCentroidsPtr = getDataStructureRef().getDataAsUnsafe<Float32ArrayType>(m_CellCentroidsDataArrayId);
  if(triangleCentroidsPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(triangleCentroidsPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector{getNumberOfFaces()}, std::vector<usize>{3}, 0.0f);
    triangleCentroidsPtr = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
  }
  if(triangleCentroidsPtr == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return MakeErrorResult(-2233, "TriangleGeom Error: Unable to find or create a valid element centroids array or data store.");
  }

  geometry_helpers::topology::FindElementCentroids(getFaces(), getVertices(), triangleCentroidsPtr);
  m_CellCentroidsDataArrayId = triangleCentroidsPtr->getId();

  return {};
}

Vec3<float64> TriangleGeom::getParametricCenter() const
{
  return {1.0 / 3.0, 1.0 / 3.0, 0.0};
}

void TriangleGeom::getShapeFunctions([[maybe_unused]] const Vec3<float64>& pCoords, float64* shape) const
{
  // r derivatives
  shape[0] = -1.0;
  shape[1] = 1.0;
  shape[2] = 0.0;

  // s derivatives
  shape[3] = -1.0;
  shape[4] = 0.0;
  shape[5] = 1.0;
}

Result<> TriangleGeom::findEdges(bool recalculate)
{
  auto* edgeListPtr = getDataStructureRef().getDataAsUnsafe<UInt64ArrayType>(m_EdgeDataArrayId);
  if(edgeListPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(edgeListPtr == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<uint64>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
    edgeListPtr = DataArray<uint64>::Create(*getDataStructure(), k_SharedEdgeListName, std::move(dataStore), getId());
    if(edgeListPtr == nullptr)
    {
      m_EdgeDataArrayId.reset();
      return MakeErrorResult(-2234, "TriangleGeom Error: Unable to find or create a valid shared edges array or data store.");
    }
  }

  auto findResult = geometry_helpers::connectivity::Find2DElementEdges(getFaces(), edgeListPtr);
  if(findResult.invalid())
  {
    m_EdgeDataArrayId.reset();
    return findResult;
  }
  m_EdgeDataArrayId = edgeListPtr->getId();

  return {};
}

Result<> TriangleGeom::findUnsharedEdges(bool recalculate)
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
  }
  if(unsharedEdgeListPtr == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return MakeErrorResult(-2235, "TriangleGeom Error: Unable to find or create a valid unshared edges array or data store.");
  }

  auto findResult = geometry_helpers::connectivity::Find2DUnsharedEdges(getFaces(), unsharedEdgeListPtr);
  if(findResult.invalid())
  {
    m_UnsharedEdgeListId.reset();
    return findResult;
  }
  m_UnsharedEdgeListId = unsharedEdgeListPtr->getId();

  return {};
}
