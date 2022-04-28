#include "HexahedralGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name)
: AbstractGeometry3D(ds, std::move(name))
{
}

HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry3D(ds, std::move(name), importId)
{
}

// HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name, usize numHexas, const std::shared_ptr<SharedVertexList>& vertices, bool allocate)
//: AbstractGeometry3D(ds, std::move(name))
//{
//}
//
// HexahedralGeom::HexahedralGeom(DataStructure& ds, std::string name, const std::shared_ptr<SharedHexList>& hexas, const std::shared_ptr<SharedVertexList>& vertices)
//: AbstractGeometry3D(ds, std::move(name))
//{
//}

HexahedralGeom::HexahedralGeom(const HexahedralGeom& other)
: AbstractGeometry3D(other)
, m_HexListId(other.m_HexListId)
, m_HexasContainingVertId(other.m_HexasContainingVertId)
, m_HexNeighborsId(other.m_HexNeighborsId)
, m_HexCentroidsId(other.m_HexCentroidsId)
, m_HexSizesId(other.m_HexSizesId)
{
}

HexahedralGeom::HexahedralGeom(HexahedralGeom&& other) noexcept
: AbstractGeometry3D(std::move(other))
, m_HexListId(std::move(other.m_HexListId))
, m_HexasContainingVertId(std::move(other.m_HexasContainingVertId))
, m_HexNeighborsId(std::move(other.m_HexNeighborsId))
, m_HexCentroidsId(std::move(other.m_HexCentroidsId))
, m_HexSizesId(std::move(other.m_HexSizesId))
{
}

HexahedralGeom::~HexahedralGeom() = default;
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

AbstractGeometry::Type HexahedralGeom::getGeomType() const
{
  return AbstractGeometry::Type::Hexahedral;
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

std::string HexahedralGeom::getGeometryTypeAsString() const
{
  return "HexahedralGeom";
}

void HexahedralGeom::resizeHexList(usize numHexas)
{
  getHexahedrals()->getDataStore()->reshapeTuples({numHexas});
}

void HexahedralGeom::setHexahedra(const SharedHexList* hexas)
{
  if(hexas == nullptr)
  {
    m_HexListId.reset();
    return;
  }
  m_HexListId = hexas->getId();
}

AbstractGeometry::SharedHexList* HexahedralGeom::getHexahedrals()
{
  return dynamic_cast<SharedHexList*>(getDataStructure()->getData(m_HexListId));
}

const AbstractGeometry::SharedHexList* HexahedralGeom::getHexahedrals() const
{
  return dynamic_cast<const SharedHexList*>(getDataStructure()->getData(m_HexListId));
}

void HexahedralGeom::setVertsAtHex(usize hexId, usize verts[8])
{
  auto* hex = dynamic_cast<SharedHexList*>(getDataStructure()->getData(m_HexListId));
  if(hex == nullptr)
  {
    return;
  }
  usize index = hexId * 8;
  for(usize i = 0; i < 8; i++)
  {
    (*hex)[index + i] = verts[i];
  }
}

void HexahedralGeom::getVertsAtHex(usize hexId, usize verts[8]) const
{
  const auto* hex = getHexahedrals();
  if(hex == nullptr)
  {
    return;
  }
  usize index = hexId * 8;
  for(usize i = 0; i < 8; i++)
  {
    verts[i] = (*hex)[index + i];
  }
}

void HexahedralGeom::getVertCoordsAtHex(usize hexId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4,
                                        complex::Point3D<float32>& vert5, complex::Point3D<float32>& vert6, complex::Point3D<float32>& vert7, complex::Point3D<float32>& vert8) const
{
  if(getHexahedrals() == nullptr)
  {
    return;
  }
  const auto* vertices = getVertices();
  if(vertices == nullptr)
  {
    return;
  }

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
  const auto* hexList = getHexahedrals();
  if(hexList == nullptr)
  {
    return 0;
  }
  return hexList->getNumberOfTuples();
}

void HexahedralGeom::initializeWithZeros()
{
  auto* vertices = getVertices();
  if(vertices != nullptr)
  {
    vertices->getDataStore()->fill(0.0);
  }
  auto* hexas = getHexahedrals();
  if(hexas != nullptr)
  {
    hexas->getDataStore()->fill(0.0);
  }
}

usize HexahedralGeom::getNumberOfElements() const
{
  const auto* elements = dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_HexSizesId));
  if(elements == nullptr)
  {
    return 0;
  }
  return elements->getNumberOfTuples();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementSizes()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfHexas()}, std::vector<usize>{1}, 0.0f);
  Float32Array* hexSizes = DataArray<float32>::Create(*getDataStructure(), "Hex Volumes", std::move(dataStore), getId());
  m_HexSizesId = hexSizes->getId();
  GeometryHelpers::Topology::FindHexVolumes<uint64_t>(getHexahedrals(), getVertices(), hexSizes);
  if(getElementSizes() == nullptr)
  {
    m_HexSizesId.reset();
    return -1;
  }
  return 1;
}

const Float32Array* HexahedralGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_HexSizesId));
}

void HexahedralGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_HexSizesId);
  m_HexSizesId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementsContainingVert()
{
  auto* hexasControllingVert = DynamicListArray<uint16_t, MeshIndexType>::Create(*getDataStructure(), "Hex Containing Vertices", getId());
  m_HexasContainingVertId = hexasControllingVert->getId();
  GeometryHelpers::Connectivity::FindElementsContainingVert<uint16, MeshIndexType>(getHexahedrals(), hexasControllingVert, getNumberOfVertices());
  if(getElementsContainingVert() == nullptr)
  {
    m_HexasContainingVertId.reset();
    return -1;
  }
  return 1;
}

const AbstractGeometry::ElementDynamicList* HexahedralGeom::getElementsContainingVert() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_HexasContainingVertId));
}

void HexahedralGeom::deleteElementsContainingVert()
{
  getDataStructure()->removeData(m_HexasContainingVertId);
  m_HexasContainingVertId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementNeighbors()
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
  m_HexNeighborsId = hexNeighbors->getId();
  err = GeometryHelpers::Connectivity::FindElementNeighbors<uint16, MeshIndexType>(getHexahedrals(), getElementsContainingVert(), hexNeighbors, AbstractGeometry::Type::Hexahedral);
  if(getElementNeighbors() == nullptr)
  {
    m_HexNeighborsId.reset();
    return -1;
  }
  return err;
}

const AbstractGeometry::ElementDynamicList* HexahedralGeom::getElementNeighbors() const
{
  return dynamic_cast<const ElementDynamicList*>(getDataStructure()->getData(m_HexNeighborsId));
}

void HexahedralGeom::deleteElementNeighbors()
{
  getDataStructure()->removeData(m_HexNeighborsId);
  m_HexNeighborsId.reset();
}

AbstractGeometry::StatusCode HexahedralGeom::findElementCentroids()
{
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfHexas()}, std::vector<usize>{3}, 0.0f);
  auto* hexCentroids = DataArray<float32>::Create(*getDataStructure(), "Hex Centroids", std::move(dataStore), getId());
  m_HexCentroidsId = hexCentroids->getId();
  GeometryHelpers::Topology::FindElementCentroids<uint64_t>(getHexahedrals(), getVertices(), hexCentroids);
  if(getElementCentroids() == nullptr)
  {
    m_HexCentroidsId.reset();
    return -1;
  }
  return 1;
}

const Float32Array* HexahedralGeom::getElementCentroids() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_HexCentroidsId));
}

void HexahedralGeom::deleteElementCentroids()
{
  getDataStructure()->removeData(m_HexCentroidsId);
  m_HexCentroidsId.reset();
}

complex::Point3D<float64> HexahedralGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void HexahedralGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
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

void HexahedralGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator HexahedralGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;
  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "HexahedralGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Number of Hexahedra", std::to_string(getNumberOfHexas()));
  toolTipGen.addValue("Number of Vertices", std::to_string(getNumberOfVertices()));
  return toolTipGen;
}

AbstractGeometry::StatusCode HexahedralGeom::findEdges()
{
  auto* edgeList = createSharedEdgeList(0);
  GeometryHelpers::Connectivity::FindHexEdges<uint64_t>(getHexahedrals(), edgeList);
  if(getEdges() == nullptr)
  {
    setEdges(nullptr);
    return -1;
  }
  setEdges(edgeList);
  return 1;
}

AbstractGeometry::StatusCode HexahedralGeom::findFaces()
{
  auto* quadList = createSharedQuadList(0);
  GeometryHelpers::Connectivity::FindHexFaces<uint64_t>(getHexahedrals(), quadList);
  if(quadList == nullptr)
  {
    setQuads(nullptr);
    return -1;
  }
  setQuads(quadList);
  return 1;
}

AbstractGeometry::StatusCode HexahedralGeom::findUnsharedEdges()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{2}, 0);
  DataArray<MeshIndexType>* unsharedEdgeList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedHexEdges<uint64_t>(getHexahedrals(), unsharedEdgeList);
  if(unsharedEdgeList == nullptr)
  {
    setUnsharedEdges(nullptr);
    return -1;
  }
  setUnsharedEdges(unsharedEdgeList);
  return 1;
}

AbstractGeometry::StatusCode HexahedralGeom::findUnsharedFaces()
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{0}, std::vector<usize>{4}, 0);
  auto* unsharedQuadList = DataArray<MeshIndexType>::Create(*getDataStructure(), "Unshared Edge List", std::move(dataStore), getId());
  GeometryHelpers::Connectivity::FindUnsharedHexFaces<uint64_t>(getHexahedrals(), unsharedQuadList);
  if(unsharedQuadList == nullptr)
  {
    setUnsharedFaces(nullptr);
    return -1;
  }
  setUnsharedFaces(unsharedQuadList);
  return 1;
}

void HexahedralGeom::resizeQuadList(usize numQuads)
{
  getQuads()->getDataStore()->reshapeTuples({numQuads});
}

void HexahedralGeom::setQuads(const SharedQuadList* quads)
{
  setFaces(quads);
}

AbstractGeometry::SharedQuadList* HexahedralGeom::getQuads()
{
  return getFaces();
}

const AbstractGeometry::SharedQuadList* HexahedralGeom::getQuads() const
{
  return getFaces();
}

void HexahedralGeom::setVertsAtQuad(usize quadId, usize verts[4])
{
  auto* quads = getQuads();
  if(quads == nullptr)
  {
    return;
  }

  auto numQuads = quads->getNumberOfTuples();
  if(quadId >= numQuads)
  {
    return;
  }

  for(usize i = 0; i < 4; i++)
  {
    verts[i] = (*quads)[quadId + i];
  }
}

void HexahedralGeom::getVertsAtQuad(usize quadId, usize verts[4]) const
{
  const auto* quads = getQuads();
  if(quads == nullptr)
  {
    return;
  }
  usize index = quadId * 4;
  for(usize i = 0; i < 4; i++)
  {
    verts[i] = quads->at(index + i);
  }
}

void HexahedralGeom::getVertCoordsAtQuad(usize quadId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4) const
{
  if(getQuads() == nullptr)
  {
    return;
  }
  const auto* vertices = getVertices();
  if(vertices == nullptr)
  {
    return;
  }

  usize verts[4];
  getVertsAtQuad(quadId, verts);
  for(usize i = 0; i < 3; i++)
  {
    vert1[i] = vertices->at(verts[0] * 3 + i);
    vert2[i] = vertices->at(verts[1] * 3 + i);
    vert3[i] = vertices->at(verts[2] * 3 + i);
    vert4[i] = vertices->at(verts[3] * 3 + i);
  }
}

usize HexahedralGeom::getNumberOfQuads() const
{
  return getNumberOfFaces();
}

uint32 HexahedralGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

void HexahedralGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  if(elementsContainingVert == nullptr)
  {
    m_HexasContainingVertId.reset();
    return;
  }
  m_HexasContainingVertId = elementsContainingVert->getId();
}

void HexahedralGeom::setElementNeighbors(const ElementDynamicList* elementNeighbors)
{
  if(elementNeighbors == nullptr)
  {
    m_HexNeighborsId.reset();
    return;
  }
  m_HexNeighborsId = elementNeighbors->getId();
}

void HexahedralGeom::setElementCentroids(const Float32Array* elementCentroids)
{
  if(elementCentroids == nullptr)
  {
    m_HexCentroidsId.reset();
    return;
  }
  m_HexCentroidsId = elementCentroids->getId();
}

void HexahedralGeom::setElementSizes(const Float32Array* elementSizes)
{
  if(elementSizes == nullptr)
  {
    m_HexSizesId.reset();
    return;
  }
  m_HexSizesId = elementSizes->getId();
}

H5::ErrorType HexahedralGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  m_HexListId = ReadH5DataId(groupReader, H5Constants::k_HexListTag);
  m_HexasContainingVertId = ReadH5DataId(groupReader, H5Constants::k_HexasContainingVertTag);
  m_HexNeighborsId = ReadH5DataId(groupReader, H5Constants::k_HexNeighborsTag);
  m_HexCentroidsId = ReadH5DataId(groupReader, H5Constants::k_HexCentroidsTag);
  m_HexSizesId = ReadH5DataId(groupReader, H5Constants::k_HexSizesTag);

  return AbstractGeometry3D::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType HexahedralGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto errorCode = AbstractGeometry3D::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto err = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(err < 0)
  {
    return err;
  }

  errorCode = WriteH5DataId(groupWriter, m_HexListId, H5Constants::k_HexListTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_HexasContainingVertId, H5Constants::k_HexasContainingVertTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_HexNeighborsId, H5Constants::k_HexNeighborsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_HexCentroidsId, H5Constants::k_HexCentroidsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_HexSizesId, H5Constants::k_HexSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}

void HexahedralGeom::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  AbstractGeometry3D::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_HexListId == updatedId.first)
    {
      m_HexListId = updatedId.second;
    }

    if(m_HexasContainingVertId == updatedId.first)
    {
      m_HexasContainingVertId = updatedId.second;
    }

    if(m_HexNeighborsId == updatedId.first)
    {
      m_HexNeighborsId = updatedId.second;
    }

    if(m_HexCentroidsId == updatedId.first)
    {
      m_HexCentroidsId = updatedId.second;
    }

    if(m_HexSizesId == updatedId.first)
    {
      m_HexSizesId = updatedId.second;
    }
  }
}
