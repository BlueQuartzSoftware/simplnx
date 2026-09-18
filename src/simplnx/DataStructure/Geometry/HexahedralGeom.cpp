#include "HexahedralGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

HexahedralGeom::HexahedralGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry3D(dataStructure, std::move(name))
{
}

HexahedralGeom::HexahedralGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry3D(dataStructure, std::move(name), importId)
{
}

DataObject::Type HexahedralGeom::getDataObjectType() const
{
  return DataObject::Type::HexahedralGeom;
}

usize HexahedralGeom::getNumberOfVerticesPerEdge() const
{
  return k_NumEdgeVerts;
}

HexahedralGeom* HexahedralGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

HexahedralGeom* HexahedralGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type HexahedralGeom::getGeomType() const
{
  return IGeometry::Type::Hexahedral;
}

BaseGroup::GroupType HexahedralGeom::getGroupType() const
{
  return GroupType::HexahedralGeom;
}

std::string HexahedralGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* HexahedralGeom::shallowCopy()
{
  return new HexahedralGeom(*this);
}

std::shared_ptr<DataObject> HexahedralGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Construct without an identifier because insertion creates it.
  auto copy = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(dataStruct, copyPath.getTargetName()));
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

usize HexahedralGeom::getNumberOfVerticesPerFace() const
{
  return k_NumFaceVerts;
}

usize HexahedralGeom::getNumberOfVerticesPerCell() const
{
  return k_NumVerts;
}

usize HexahedralGeom::getNumberOfCells() const
{
  auto& elements = getPolyhedraRef();
  return elements.getNumberOfTuples();
}

Result<> HexahedralGeom::findElementSizes(bool recalculate)
{
  auto* hexSizes = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(hexSizes != nullptr && !recalculate)
  {
    return {};
  }

  if(hexSizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
    hexSizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(hexSizes == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-2530, "HexahedralGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  m_ElementSizesId = hexSizes->getId();
  GeometryHelpers::Topology::FindHexVolumes<uint64>(getPolyhedra(), getVertices(), hexSizes);

  return {};
}

Result<> HexahedralGeom::findElementsContainingVert(bool recalculate)
{
  auto* hexasControllingVert = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellContainingVertDataArrayId);
  if(hexasControllingVert != nullptr && !recalculate)
  {
    return {};
  }

  if(hexasControllingVert == nullptr)
  {
    hexasControllingVert = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltsContainingVert, getId());
    if(hexasControllingVert == nullptr)
    {
      m_CellContainingVertDataArrayId.reset();
      return MakeErrorResult(-2531, "HexahedralGeom Error: Unable to find or create a valid dynamic list array.");
    }
  }

  m_CellContainingVertDataArrayId = hexasControllingVert->getId();
  auto findResult = GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getPolyhedra(), hexasControllingVert, getNumberOfVertices());
  if(findResult.invalid())
  {
    m_CellContainingVertDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> HexahedralGeom::findElementNeighbors(bool recalculate)
{
  auto* hexNeighbors = getDataStructureRef().getDataAsUnsafe<ElementDynamicList>(m_CellNeighborsDataArrayId);
  if(hexNeighbors != nullptr && !recalculate)
  {
    return {};
  }

  Result<> result = findElementsContainingVert(recalculate);
  if(result.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return result;
  }
  if(hexNeighbors == nullptr)
  {
    hexNeighbors = DynamicListArray<uint16, MeshIndexType>::Create(*getDataStructure(), k_EltNeighbors, getId());
    if(hexNeighbors == nullptr)
    {
      m_CellNeighborsDataArrayId.reset();
      return MakeErrorResult(-2532, "HexahedralGeom Error: Unable to find or create a dynamic list array.");
    }
  }

  m_CellNeighborsDataArrayId = hexNeighbors->getId();

  auto findResult = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getPolyhedra(), getElementsContainingVert(), hexNeighbors, Type::Hexahedral);
  if(findResult.invalid())
  {
    m_CellNeighborsDataArrayId.reset();
    return findResult;
  }

  return {};
}

Result<> HexahedralGeom::findElementCentroids(bool recalculate)
{
  auto* hexCentroids = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_CellCentroidsDataArrayId);
  if(hexCentroids != nullptr && !recalculate)
  {
    return {};
  }

  if(hexCentroids == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
    hexCentroids = DataArray<float32>::Create(*getDataStructure(), k_EltCentroids, std::move(dataStore), getId());
    if(hexCentroids == nullptr)
    {
      m_CellCentroidsDataArrayId.reset();
      return MakeErrorResult(-2533, "HexahedralGeom Error: Unable to find or create a valid element centroids array or data store.");
    }
  }

  m_CellCentroidsDataArrayId = hexCentroids->getId();
  GeometryHelpers::Topology::FindElementCentroids<uint64>(getPolyhedra(), getVertices(), hexCentroids);

  return {};
}

Point3D<float64> HexahedralGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void HexahedralGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
{
  float64 rm = 1.0 - pCoords[0];
  float64 sm = 1.0 - pCoords[1];
  float64 tm = 1.0 - pCoords[2];

  // r-derivatives
  shape[0] = -sm * tm;
  shape[1] = sm * tm;
  shape[2] = pCoords[1] * tm;
  shape[3] = -pCoords[1] * tm;
  shape[4] = -sm * pCoords[2];
  shape[5] = sm * pCoords[2];
  shape[6] = pCoords[1] * pCoords[2];
  shape[7] = -pCoords[1] * pCoords[2];

  // s-derivatives
  shape[8] = -rm * tm;
  shape[9] = -pCoords[0] * tm;
  shape[10] = pCoords[0] * tm;
  shape[11] = rm * tm;
  shape[12] = -rm * pCoords[2];
  shape[13] = -pCoords[0] * pCoords[2];
  shape[14] = pCoords[0] * pCoords[2];
  shape[15] = rm * pCoords[2];

  // t-derivatives
  shape[16] = -rm * sm;
  shape[17] = -pCoords[0] * sm;
  shape[18] = -pCoords[0] * pCoords[1];
  shape[19] = -rm * pCoords[1];
  shape[20] = rm * sm;
  shape[21] = pCoords[0] * sm;
  shape[22] = pCoords[0] * pCoords[1];
  shape[23] = rm * pCoords[1];
}

Result<> HexahedralGeom::findEdges(bool recalculate)
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
      return MakeErrorResult(-2534, "HexahedralGeom Error: Unable to find or create a valid shared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindHexEdges<uint64>(getPolyhedra(), edgeList);
  if(findResult.invalid())
  {
    m_EdgeDataArrayId.reset();
    return findResult;
  }
  m_EdgeDataArrayId = edgeList->getId();

  return {};
}

Result<> HexahedralGeom::findFaces(bool recalculate)
{
  auto* quadList = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_FaceListId);
  if(quadList != nullptr && !recalculate)
  {
    return {};
  }

  if(quadList == nullptr)
  {
    quadList = createSharedQuadList(0);
    if(quadList == nullptr)
    {
      m_FaceListId.reset();
      return MakeErrorResult(-2535, "HexahedralGeom Error: Unable to find or create a valid shared faces array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindHexFaces<uint64>(getPolyhedra(), quadList);
  if(findResult.invalid())
  {
    m_FaceListId.reset();
    return findResult;
  }
  m_FaceListId = quadList->getId();

  return {};
}

Result<> HexahedralGeom::findUnsharedEdges(bool recalculate)
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
      return MakeErrorResult(-2536, "HexahedralGeom Error: Unable to find or create a valid unshared edges array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindUnsharedHexEdges<uint64>(getPolyhedra(), unsharedEdgeList);
  if(findResult.invalid())
  {
    m_UnsharedEdgeListId.reset();
    return findResult;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();

  return {};
}

Result<> HexahedralGeom::findUnsharedFaces(bool recalculate)
{
  auto* unsharedQuadList = getDataStructureRef().getDataAsUnsafe<DataArray<MeshIndexType>>(m_UnsharedFaceListId);
  if(unsharedQuadList != nullptr && !recalculate)
  {
    return {};
  }

  if(unsharedQuadList == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
    unsharedQuadList = DataArray<MeshIndexType>::Create(*getDataStructure(), k_UnsharedFacesListName, std::move(dataStore), getId());
    if(unsharedQuadList == nullptr)
    {
      m_UnsharedFaceListId.reset();
      return MakeErrorResult(-2537, "HexahedralGeom Error: Unable to find or create a valid unshared faces array or data store.");
    }
  }

  auto findResult = GeometryHelpers::Connectivity::FindUnsharedHexFaces<uint64>(getPolyhedra(), unsharedQuadList);
  if(findResult.invalid())
  {
    m_UnsharedFaceListId.reset();
    return findResult;
  }
  m_UnsharedFaceListId = unsharedQuadList->getId();

  return {};
}
