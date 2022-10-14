#include "HexahedralGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name)
: INodeGeometry3D(ds, std::move(name))
{
}

HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry3D(ds, std::move(name), importId)
{
}

DataObject::Type HexahedralGeom::getDataObjectType() const
{
  return DataObject::Type::HexahedralGeom;
}

HexahedralGeom* HexahedralGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

HexahedralGeom* HexahedralGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
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
  return "HexahedralGeom";
}

DataObject* HexahedralGeom::shallowCopy()
{
  return new HexahedralGeom(*this);
}

std::shared_ptr<DataObject> HexahedralGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = *getDataStructure();
  auto copy = std::shared_ptr<HexahedralGeom>(new HexahedralGeom(dataStruct, copyPath.getTargetName(), getId()));
  if(dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    if(m_VertexAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getVertexAttributeMatrix()))
      {
        const auto dataObjCopy = getVertexAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_VertexDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertices()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(isParentOf(getVertices()))
      {
        const auto dataObjCopy = getVertices()->deepCopy(copiedDataPath);
      }
      copy->m_VertexDataArrayId = dataStruct.getId(copiedDataPath);
    }

    if(m_EdgeAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getEdgeAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getEdgeAttributeMatrix()))
      {
        const auto dataObjCopy = getEdgeAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_EdgeAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_FaceDataId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getFaceAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getFaceAttributeMatrix()))
      {
        const auto dataObjCopy = getFaceAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_FaceDataId = dataStruct.getId(copiedDataPath);
    }

    if(m_PolyhedronDataId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getPolyhedraAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(isParentOf(getPolyhedraAttributeMatrix()))
      {
        const auto dataObjCopy = getPolyhedraAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_PolyhedronDataId = dataStruct.getId(copiedDataPath);
    }

    if(m_PolyhedronListId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getPolyhedra()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(isParentOf(getPolyhedra()))
      {
        const auto dataObjCopy = getPolyhedra()->deepCopy(copiedDataPath);
      }
      copy->m_PolyhedronListId = dataStruct.getId(copiedDataPath);
    }

    if(getElementSizes() != nullptr)
    {
      copy->findElementSizes();
    }
    if(getElementsContainingVert() != nullptr)
    {
      copy->findElementsContainingVert();
    }
    if(getElementNeighbors() != nullptr)
    {
      copy->findElementNeighbors();
    }
    if(getElementCentroids() != nullptr)
    {
      copy->findElementCentroids();
    }
    if(getUnsharedEdges() != nullptr)
    {
      copy->findUnsharedEdges();
    }
    if(getEdges() != nullptr)
    {
      copy->findEdges();
    }
    if(getUnsharedFaces() != nullptr)
    {
      copy->findUnsharedFaces();
    }
    if(getFaces() != nullptr)
    {
      copy->findFaces();
    }
  }

  return copy;
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

IGeometry::StatusCode HexahedralGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
  Float32Array* hexSizes = DataArray<float32>::Create(*getDataStructure(), "Hex Volumes", std::move(dataStore), getId());
  m_ElementSizesId = hexSizes->getId();
  GeometryHelpers::Topology::FindHexVolumes<uint64_t>(getPolyhedra(), getVertices(), hexSizes);
  if(getElementSizes() == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  return 1;
}

IGeometry::StatusCode HexahedralGeom::findElementsContainingVert()
{
  auto* hexasControllingVert = DynamicListArray<uint16_t, MeshIndexType>::Create(*getDataStructure(), "Hex Containing Vertices", getId());
  m_CellContainingVertDataArrayId = hexasControllingVert->getId();
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getPolyhedra(), hexasControllingVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    m_CellContainingVertDataArrayId.reset();
    return -1;
  }
  return 1;
}

IGeometry::StatusCode HexahedralGeom::findElementNeighbors()
{
  StatusCode err = 0;
  if(getElementsContainingVert() == nullptr)
  {
    err = findElementsContainingVert();
    if(err < 0)
    {
      return err;
    }
  }
  auto* hexNeighbors = DynamicListArray<uint16_t, MeshIndexType>::Create(*getDataStructure(), "Hex Neighbors", getId());
  m_CellNeighborsDataArrayId = hexNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getPolyhedra(), getElementsContainingVert(), hexNeighbors, IGeometry::Type::Hexahedral);
  if(getElementNeighbors() == nullptr)
  {
    m_CellNeighborsDataArrayId.reset();
    return -1;
  }
  return err;
}

IGeometry::StatusCode HexahedralGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{3}, 0.0f);
  auto* hexCentroids = DataArray<float32>::Create(*getDataStructure(), "Hex Centroids", std::move(dataStore), getId());
  m_CellCentroidsDataArrayId = hexCentroids->getId();
  GeometryHelpers::Topology::FindElementCentroids<uint64_t>(getPolyhedra(), getVertices(), hexCentroids);
  if(getElementCentroids() == nullptr)
  {
    m_CellCentroidsDataArrayId.reset();
    return -1;
  }
  return 1;
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

IGeometry::StatusCode HexahedralGeom::findEdges()
{
  auto* edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::FindHexEdges<uint64_t>(getPolyhedra(), edgeList);
  if(getEdges() == nullptr)
  {
    m_EdgeDataArrayId.reset();
    return -1;
  }
  m_EdgeDataArrayId = edgeList->getId();
  return 1;
}

IGeometry::StatusCode HexahedralGeom::findFaces()
{
  auto* quadList = createSharedQuadList(0);
  GeometryHelpers::Connectivity::FindHexFaces<uint64_t>(getPolyhedra(), quadList);
  if(quadList == nullptr)
  {
    m_FaceListId.reset();
    return -1;
  }
  m_FaceListId = quadList->getId();
  return 1;
}

IGeometry::StatusCode HexahedralGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  DataArray<MeshIndexType>* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedHexEdges<uint64_t>(getPolyhedra(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    m_UnsharedEdgeListId.reset();
    return -1;
  }
  m_UnsharedEdgeListId = unsharedEdgeList->getId();
  return 1;
}

IGeometry::StatusCode HexahedralGeom::findUnsharedFaces()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto* unsharedQuadList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedHexFaces<uint64_t>(getPolyhedra(), unsharedQuadList);
  if(unsharedQuadList == nullptr)
  {
    m_UnsharedFaceListId.reset();
    return -1;
  }
  m_UnsharedFaceListId = unsharedQuadList->getId();
  return 1;
}
