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

HexahedralGeom::HexahedralGeom(const HexahedralGeom&) = default;

HexahedralGeom::HexahedralGeom(HexahedralGeom&& other) noexcept = default;

HexahedralGeom::~HexahedralGeom() noexcept = default;

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

std::string HexahedralGeom::getTypeName() const
{
  return "HexahedralGeom";
}

DataObject* HexahedralGeom::shallowCopy()
{
  return new HexahedralGeom(*this);
}

DataObject* HexahedralGeom::deepCopy()
{
  return new HexahedralGeom(*this);
}

void HexahedralGeom::setVertsAtHex(usize hexId, usize verts[8])
{
  auto& hex = getPolyhedraRef();
  usize index = hexId * 8;
  for(usize i = 0; i < 8; i++)
  {
    hex[index + i] = verts[i];
  }
}

void HexahedralGeom::getVertsAtHex(usize hexId, usize verts[8]) const
{
  auto& hex = getPolyhedraRef();
  usize index = hexId * 8;
  for(usize i = 0; i < 8; i++)
  {
    verts[i] = hex[index + i];
  }
}

void HexahedralGeom::getVertCoordsAtHex(usize hexId, Point3D<float32>& vert1, Point3D<float32>& vert2, Point3D<float32>& vert3, Point3D<float32>& vert4, Point3D<float32>& vert5,
                                        Point3D<float32>& vert6, Point3D<float32>& vert7, Point3D<float32>& vert8) const
{
  std::array<usize, 8> vertIds = {0};
  getVertsAtHex(hexId, vertIds.data());

  vert1 = getCoords(vertIds[0]);
  vert2 = getCoords(vertIds[1]);
  vert3 = getCoords(vertIds[2]);
  vert4 = getCoords(vertIds[3]);
  vert5 = getCoords(vertIds[4]);
  vert6 = getCoords(vertIds[5]);
  vert7 = getCoords(vertIds[6]);
  vert8 = getCoords(vertIds[7]);
}

usize HexahedralGeom::getNumberOfHexas() const
{
  auto& hexList = getPolyhedraRef();
  return hexList.getNumberOfTuples();
}

usize HexahedralGeom::getNumberOfElements() const
{
  auto& elements = getPolyhedraRef();
  return elements.getNumberOfTuples();
}

IGeometry::StatusCode HexahedralGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfHexas()}, std::vector<usize>{1}, 0.0f);
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
  m_ElementContainingVertId = hexasControllingVert->getId();
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getPolyhedra(), hexasControllingVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    m_ElementContainingVertId.reset();
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
  m_ElementNeighborsId = hexNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getPolyhedra(), getElementsContainingVert(), hexNeighbors, IGeometry::Type::Hexahedral);
  if(getElementNeighbors() == nullptr)
  {
    m_ElementNeighborsId.reset();
    return -1;
  }
  return err;
}

IGeometry::StatusCode HexahedralGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfHexas()}, std::vector<usize>{3}, 0.0f);
  auto* hexCentroids = DataArray<float32>::Create(*getDataStructure(), "Hex Centroids", std::move(dataStore), getId());
  m_ElementCentroidsId = hexCentroids->getId();
  GeometryHelpers::Topology::FindElementCentroids<uint64_t>(getPolyhedra(), getVertices(), hexCentroids);
  if(getElementCentroids() == nullptr)
  {
    m_ElementCentroidsId.reset();
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
    m_EdgeListId.reset();
    return -1;
  }
  m_EdgeListId = edgeList->getId();
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

void HexahedralGeom::setVertsAtQuad(usize quadId, usize verts[4])
{
  auto& quads = getFacesRef();
  auto numQuads = quads.getNumberOfTuples();
  if(quadId >= numQuads)
  {
    return;
  }

  for(usize i = 0; i < 4; i++)
  {
    verts[i] = quads[quadId + i];
  }
}

void HexahedralGeom::getVertsAtQuad(usize quadId, usize verts[4]) const
{
  auto& quads = getFacesRef();
  usize index = quadId * 4;
  for(usize i = 0; i < 4; i++)
  {
    verts[i] = quads.at(index + i);
  }
}

void HexahedralGeom::getVertCoordsAtQuad(usize quadId, Point3D<float32>& vert1, Point3D<float32>& vert2, Point3D<float32>& vert3, Point3D<float32>& vert4) const
{
  const auto& vertices = getVerticesRef();

  usize verts[4];
  getVertsAtQuad(quadId, verts);
  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = vertices.at(verts[0] * 3 + i);
    vert2[i] = vertices.at(verts[1] * 3 + i);
    vert3[i] = vertices.at(verts[2] * 3 + i);
    vert4[i] = vertices.at(verts[3] * 3 + i);
  }
}

usize HexahedralGeom::getNumberOfQuads() const
{
  return getFacesRef().getNumberOfTuples();
}
