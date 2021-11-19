#include "RectGridGeom.hpp"

#include <iterator>
#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

using namespace complex;

RectGridGeom::RectGridGeom(DataStructure& ds, std::string name)
: AbstractGeometryGrid(ds, std::move(name))
{
}

RectGridGeom::RectGridGeom(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometryGrid(ds, std::move(name), importId)
{
}

RectGridGeom::RectGridGeom(const RectGridGeom& other)
: AbstractGeometryGrid(other)
, m_xBoundsId(other.m_xBoundsId)
, m_yBoundsId(other.m_yBoundsId)
, m_zBoundsId(other.m_zBoundsId)
, m_VoxelSizesId(other.m_VoxelSizesId)
, m_Dimensions(other.m_Dimensions)
{
}

RectGridGeom::RectGridGeom(RectGridGeom&& other) noexcept
: AbstractGeometryGrid(std::move(other))
, m_xBoundsId(std::move(other.m_xBoundsId))
, m_yBoundsId(std::move(other.m_yBoundsId))
, m_zBoundsId(std::move(other.m_zBoundsId))
, m_VoxelSizesId(std::move(other.m_VoxelSizesId))
, m_Dimensions(std::move(other.m_Dimensions))
{
}

RectGridGeom::~RectGridGeom() = default;

RectGridGeom* RectGridGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<RectGridGeom>(new RectGridGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

RectGridGeom* RectGridGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<RectGridGeom>(new RectGridGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string RectGridGeom::getTypeName() const
{
  return getGeometryTypeAsString();
}

DataObject* RectGridGeom::shallowCopy()
{
  return new RectGridGeom(*this);
}

DataObject* RectGridGeom::deepCopy()
{
  auto copy = new RectGridGeom(*getDataStructure(), getName(), getId());

  copy->m_xBoundsId = m_xBoundsId;
  copy->m_yBoundsId = m_yBoundsId;
  copy->m_zBoundsId = m_zBoundsId;
  copy->m_VoxelSizesId = m_VoxelSizesId;
  copy->m_Dimensions = m_Dimensions;

  for(auto& [id, childPtr] : getDataMap())
  {
    copy->insert(childPtr);
  }
  return copy;
}

std::string RectGridGeom::getGeometryTypeAsString() const
{
  return "RectGridGeom";
}

void RectGridGeom::setBounds(const Float32Array* xBounds, const Float32Array* yBounds, const Float32Array* zBounds)
{
  if(!xBounds)
  {
    m_xBoundsId.reset();
  }
  else
  {
    m_xBoundsId = xBounds->getId();
  }

  if(!yBounds)
  {
    m_yBoundsId.reset();
  }
  else
  {
    m_yBoundsId = yBounds->getId();
  }

  if(!zBounds)
  {
    m_zBoundsId.reset();
  }
  else
  {
    m_zBoundsId = zBounds->getId();
  }
}

const Float32Array* RectGridGeom::getXBounds() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_xBoundsId));
}

const Float32Array* RectGridGeom::getYBounds() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_yBoundsId));
}

const Float32Array* RectGridGeom::getZBounds() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_zBoundsId));
}

void RectGridGeom::initializeWithZeros()
{
  for(usize i = 0; i < 3; i++)
  {
    m_Dimensions[i] = 0;
  }
  m_xBoundsId.reset();
  m_yBoundsId.reset();
  m_zBoundsId.reset();
}

usize RectGridGeom::getNumberOfElements() const
{
  return m_Dimensions.getX() * m_Dimensions.getY() * m_Dimensions.getZ();
}

AbstractGeometry::StatusCode RectGridGeom::findElementSizes()
{
  auto sizes = new DataStore<float32>({getNumberOfElements()}, {1});
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();
  float32 xRes = 0.0f;
  float32 yRes = 0.0f;
  float32 zRes = 0.0f;

  for(usize z = 0; z < m_Dimensions[2]; z++)
  {
    for(usize y = 0; y < m_Dimensions[1]; y++)
    {
      for(usize x = 0; x < m_Dimensions[0]; x++)
      {
        xRes = xBnds->at(x + 1) - xBnds->at(x);
        yRes = yBnds->at(y + 1) - yBnds->at(y);
        zRes = zBnds->at(z + 1) - zBnds->at(z);
        if(xRes <= 0.0f || yRes <= 0.0f || zRes <= 0.0f)
        {
          delete sizes;
          m_VoxelSizesId.reset();
          return -1;
        }
        (*sizes)[(m_Dimensions[0] * m_Dimensions[1] * z) + (m_Dimensions[0] * y) + x] = zRes * yRes * xRes;
      }
    }
  }

  Float32Array* sizeArray = DataArray<float32>::Create(*getDataStructure(), "Voxel Sizes", sizes, getId());
  if(!sizeArray)
  {
    delete sizes;
    m_VoxelSizesId.reset();
    return -1;
  }

  m_VoxelSizesId = sizeArray->getId();
  return 1;
}

const Float32Array* RectGridGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_VoxelSizesId));
}

void RectGridGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_VoxelSizesId);
  m_VoxelSizesId.reset();
}

AbstractGeometry::StatusCode RectGridGeom::findElementsContainingVert()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* RectGridGeom::getElementsContainingVert() const
{
  return nullptr;
}

void RectGridGeom::deleteElementsContainingVert()
{
}

AbstractGeometry::StatusCode RectGridGeom::findElementNeighbors()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* RectGridGeom::getElementNeighbors() const
{
  return nullptr;
}

void RectGridGeom::deleteElementNeighbors()
{
}

AbstractGeometry::StatusCode RectGridGeom::findElementCentroids()
{
  return -1;
}

const Float32Array* RectGridGeom::getElementCentroids() const
{
  return nullptr;
}

void RectGridGeom::deleteElementCentroids()
{
}

complex::Point3D<float64> RectGridGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void RectGridGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
{
  float64 rm = 0.0;
  float64 sm = 0.0;
  float64 tm = 0.0;

  rm = 1.0 - pCoords[0];
  sm = 1.0 - pCoords[1];
  tm = 1.0 - pCoords[2];

  // r derivatives
  shape[0] = -sm * tm;
  shape[1] = sm * tm;
  shape[2] = -pCoords[1] * tm;
  shape[3] = pCoords[1] * tm;
  shape[4] = -sm * pCoords[2];
  shape[5] = sm * pCoords[2];
  shape[6] = -pCoords[1] * pCoords[2];
  shape[7] = pCoords[1] * pCoords[2];

  // s derivatives
  shape[8] = -rm * tm;
  shape[9] = -pCoords[0] * tm;
  shape[10] = rm * tm;
  shape[11] = pCoords[0] * tm;
  shape[12] = -rm * pCoords[2];
  shape[13] = -pCoords[0] * pCoords[2];
  shape[14] = rm * pCoords[2];
  shape[15] = pCoords[0] * pCoords[2];

  // t derivatives
  shape[16] = -rm * sm;
  shape[17] = -pCoords[0] * sm;
  shape[18] = -rm * pCoords[1];
  shape[19] = -pCoords[0] * pCoords[1];
  shape[20] = rm * sm;
  shape[21] = pCoords[0] * sm;
  shape[22] = rm * pCoords[1];
  shape[23] = pCoords[0] * pCoords[1];
}

void RectGridGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

std::string RectGridGeom::getInfoString(complex::InfoStringFormat format) const
{
  if(format == InfoStringFormat::HtmlFormat)
  {
    return getTooltipGenerator().generateHTML();
  }

  return "";
}

complex::TooltipGenerator RectGridGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;

  int64 volDims[3] = {static_cast<int64>(getNumXPoints()), static_cast<int64>(getNumYPoints()), static_cast<int64>(getNumZPoints())};

  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "RectGridGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addValue("Dimmensions", std::to_string(volDims[0]) + " x " + std::to_string(volDims[1]) + " x " + std::to_string(volDims[2]));
  toolTipGen.addValue("Spacing", "Variable");

  return toolTipGen;
}

void RectGridGeom::setDimensions(const complex::SizeVec3& dims)
{
  m_Dimensions = dims;
}

complex::SizeVec3 RectGridGeom::getDimensions() const
{
  return m_Dimensions;
}

usize RectGridGeom::getNumXPoints() const
{
  return m_Dimensions.getX();
}

usize RectGridGeom::getNumYPoints() const
{
  return m_Dimensions.getY();
}

usize RectGridGeom::getNumZPoints() const
{
  return m_Dimensions.getZ();
}

complex::Point3D<float32> RectGridGeom::getPlaneCoordsf(usize idx[3]) const
{
  const Float32Array* xBnds = getXBounds();
  const Float32Array* yBnds = getYBounds();
  const Float32Array* zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = (*xBnds)[idx[0]];
  coords[1] = (*yBnds)[idx[1]];
  coords[2] = (*zBnds)[idx[2]];
  return coords;
}

complex::Point3D<float32> RectGridGeom::getPlaneCoordsf(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float32> coords;
  coords[0] = (*xBnds)[x];
  coords[1] = (*yBnds)[y];
  coords[2] = (*zBnds)[z];
  return coords;
}

complex::Point3D<float32> RectGridGeom::getPlaneCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float32> coords;
  coords[0] = (*xBnds)[column];
  coords[1] = (*yBnds)[row];
  coords[2] = (*zBnds)[plane];
  return coords;
}

complex::Point3D<float64> RectGridGeom::getPlaneCoords(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[idx[0]]);
  coords[1] = static_cast<float64>((*yBnds)[idx[1]]);
  coords[2] = static_cast<float64>((*zBnds)[idx[2]]);
  return coords;
}

complex::Point3D<float64> RectGridGeom::getPlaneCoords(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[x]);
  coords[1] = static_cast<float64>((*yBnds)[y]);
  coords[2] = static_cast<float64>((*zBnds)[z]);
  return coords;
}

complex::Point3D<float64> RectGridGeom::getPlaneCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[column]);
  coords[1] = static_cast<float64>((*yBnds)[row]);
  coords[2] = static_cast<float64>((*zBnds)[plane]);
  return coords;
}

complex::Point3D<float32> RectGridGeom::getCoordsf(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[idx[0]] + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5f * ((*yBnds)[idx[1]] + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5f * ((*zBnds)[idx[2]] + (*zBnds)[idx[2] + 1]);
  return coords;
}

complex::Point3D<float32> RectGridGeom::getCoordsf(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[x] + (*xBnds)[x + 1]);
  coords[1] = 0.5f * ((*yBnds)[y] + (*yBnds)[y + 1]);
  coords[2] = 0.5f * ((*zBnds)[z] + (*zBnds)[z + 1]);
  return coords;
}

complex::Point3D<float32> RectGridGeom::getCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[column] + (*xBnds)[column + 1]);
  coords[1] = 0.5f * ((*yBnds)[row] + (*yBnds)[row + 1]);
  coords[2] = 0.5f * ((*zBnds)[plane] + (*zBnds)[plane + 1]);
  return coords;
}

complex::Point3D<float64> RectGridGeom::getCoords(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[idx[0]]) + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[idx[1]]) + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[idx[2]]) + (*zBnds)[idx[2] + 1]);
  return coords;
}

complex::Point3D<float64> RectGridGeom::getCoords(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[x]) + (*xBnds)[x + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[y]) + (*yBnds)[y + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[z]) + (*zBnds)[z + 1]);
  return coords;
}

complex::Point3D<float64> RectGridGeom::getCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[column]) + (*xBnds)[column + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[row]) + (*yBnds)[row + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[plane]) + (*zBnds)[plane + 1]);
  return coords;
}

usize RectGridGeom::getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const
{
  auto& xBnds = *getXBounds();
  auto& yBnds = *getYBounds();
  auto& zBnds = *getZBounds();

  if(xCoord < xBnds.front() || xCoord >= xBnds.back())
  {
    return {};
  }

  if(yCoord < yBnds.front() || yCoord >= yBnds.back())
  {
    return {};
  }

  if(zCoord < zBnds.front() || zCoord >= zBnds.back())
  {
    return {};
  }

  int64 x = std::distance(xBnds.begin(), std::adjacent_find(xBnds.begin(), xBnds.end(), [xCoord](const float32 a, const float32 b) { return (xCoord >= a && xCoord < b); }));
  int64 y = std::distance(yBnds.begin(), std::adjacent_find(yBnds.begin(), yBnds.end(), [yCoord](const float32 a, const float32 b) { return (yCoord >= a && yCoord < b); }));
  int64 z = std::distance(zBnds.begin(), std::adjacent_find(zBnds.begin(), zBnds.end(), [zCoord](const float32 a, const float32 b) { return (zCoord >= a && zCoord < b); }));

  usize xSize = xBnds.getSize() - 1;
  usize ySize = yBnds.getSize() - 1;
  return (ySize * xSize * z) + (xSize * y) + x;
}

usize RectGridGeom::getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const
{
  auto& xBnds = *getXBounds();
  auto& yBnds = *getYBounds();
  auto& zBnds = *getZBounds();

  if(xCoord < xBnds.front() || xCoord >= xBnds.back())
  {
    return {};
  }

  if(yCoord < yBnds.front() || yCoord >= yBnds.back())
  {
    return {};
  }

  if(zCoord < zBnds.front() || zCoord >= zBnds.back())
  {
    return {};
  }

  // Use standard distance to get the index of the returned iterator from adjacent_find
  usize x = std::distance(xBnds.begin(), std::adjacent_find(xBnds.begin(), xBnds.end(), [xCoord](const float32 a, const float32 b) { return (xCoord >= a && xCoord < b); }));
  usize y = std::distance(yBnds.begin(), std::adjacent_find(yBnds.begin(), yBnds.end(), [yCoord](const float32 a, const float32 b) { return (yCoord >= a && yCoord < b); }));
  usize z = std::distance(zBnds.begin(), std::adjacent_find(zBnds.begin(), zBnds.end(), [zCoord](const float32 a, const float32 b) { return (zCoord >= a && zCoord < b); }));

  usize xSize = xBnds.getSize() - 1;
  usize ySize = yBnds.getSize() - 1;
  return (ySize * xSize * z) + (xSize * y) + x;
}

uint32 RectGridGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

void RectGridGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
}

void RectGridGeom::setElementNeighbors(const ElementDynamicList* elementsNeighbors)
{
}

void RectGridGeom::setElementCentroids(const Float32Array* elementCentroids)
{
}

void RectGridGeom::setElementSizes(const Float32Array* elementSizes)
{
  if(!elementSizes)
  {
    m_VoxelSizesId.reset();
    return;
  }
  m_VoxelSizesId = elementSizes->getId();
}

H5::ErrorType RectGridGeom::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader)
{
  // Read Dimensions
  auto volumeAttribute = groupReader.getAttribute("Dimensions");
  if(!volumeAttribute.isValid())
  {
    return -1;
  }
  std::vector<size_t> volumeDimensions = volumeAttribute.readAsVector<size_t>();
  setDimensions(volumeDimensions);

  // Read DataObject IDs
  m_xBoundsId = ReadH5DataId(groupReader, H5Constants::k_XBoundsTag);
  m_yBoundsId = ReadH5DataId(groupReader, H5Constants::k_YBoundsTag);
  m_zBoundsId = ReadH5DataId(groupReader, H5Constants::k_ZBoundsTag);
  m_VoxelSizesId = ReadH5DataId(groupReader, H5Constants::k_VoxelSizesTag);

  return getDataMap().readH5Group(dataStructureReader, groupReader, getId());
}

H5::ErrorType RectGridGeom::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write dimensions
  H5::AttributeWriter::DimsVector dims = {3};
  std::vector<size_t> dimsVector(3);
  for(size_t i = 0; i < 3; i++)
  {
    dimsVector[i] = m_Dimensions[i];
  }

  auto dimensionAttr = groupWriter.createAttribute(H5Constants::k_DimensionsTag);
  errorCode = dimensionAttr.writeVector(dims, dimsVector);
  if(errorCode < 0)
  {
    return errorCode;
  }

  // Write DataObject IDs
  errorCode = WriteH5DataId(groupWriter, m_xBoundsId, H5Constants::k_XBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_yBoundsId, H5Constants::k_YBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_zBoundsId, H5Constants::k_ZBoundsTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WriteH5DataId(groupWriter, m_VoxelSizesId, H5Constants::k_VoxelSizesTag);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
