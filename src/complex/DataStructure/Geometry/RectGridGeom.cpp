#include "RectGridGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

RectGridGeom::RectGridGeom(DataStructure* ds, const std::string& name)
: AbstractGeometryGrid(ds, name)
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

DataObject* RectGridGeom::shallowCopy()
{
  return new RectGridGeom(*this);
}

DataObject* RectGridGeom::deepCopy()
{
  throw std::runtime_error("");
}

std::string RectGridGeom::getGeometryTypeAsString() const
{
  return "RectGridGeom";
}

void RectGridGeom::setBounds(const FloatArray* xBounds, const FloatArray* yBounds, const FloatArray* zBounds)
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

const FloatArray* RectGridGeom::getXBounds() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_xBoundsId));
}

const FloatArray* RectGridGeom::getYBounds() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_yBoundsId));
}

const FloatArray* RectGridGeom::getZBounds() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_zBoundsId));
}

void RectGridGeom::initializeWithZeros()
{
  for(size_t i = 0; i < 3; i++)
  {
    m_Dimensions[i] = 0;
  }
  m_xBoundsId.reset();
  m_yBoundsId.reset();
  m_zBoundsId.reset();
}

size_t RectGridGeom::getNumberOfElements() const
{
  return m_Dimensions.getX() * m_Dimensions.getY() * m_Dimensions.getZ();
}

AbstractGeometry::StatusCode RectGridGeom::findElementSizes()
{
  auto sizes = new DataStore<float>(1, getNumberOfElements());
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();
  float xRes = 0.0f;
  float yRes = 0.0f;
  float zRes = 0.0f;

  for(size_t z = 0; z < m_Dimensions[2]; z++)
  {
    for(size_t y = 0; y < m_Dimensions[1]; y++)
    {
      for(size_t x = 0; x < m_Dimensions[0]; x++)
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

  FloatArray* sizeArray = getDataStructure()->createDataArray<float>("Voxel Sizes", sizes, getId());
  if(!sizeArray)
  {
    delete sizes;
    m_VoxelSizesId.reset();
    return -1;
  }

  m_VoxelSizesId = sizeArray->getId();
  return 1;
}

const FloatArray* RectGridGeom::getElementSizes() const
{
  return dynamic_cast<const FloatArray*>(getDataStructure()->getData(m_VoxelSizesId));
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

const FloatArray* RectGridGeom::getElementCentroids() const
{
  return nullptr;
}

void RectGridGeom::deleteElementCentroids()
{
}

complex::Point3D<double> RectGridGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void RectGridGeom::getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const
{
  double rm = 0.0;
  double sm = 0.0;
  double tm = 0.0;

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

void RectGridGeom::findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const
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

  int64_t volDims[3] = {static_cast<int64_t>(getNumXPoints()), static_cast<int64_t>(getNumYPoints()), static_cast<int64_t>(getNumZPoints())};

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

size_t RectGridGeom::getNumXPoints() const
{
  return m_Dimensions.getX();
}

size_t RectGridGeom::getNumYPoints() const
{
  return m_Dimensions.getY();
}

size_t RectGridGeom::getNumZPoints() const
{
  return m_Dimensions.getZ();
}

complex::Point3D<float> RectGridGeom::getPlaneCoordsf(size_t idx[3]) const
{
  const FloatArray* xBnds = getXBounds();
  const FloatArray* yBnds = getYBounds();
  const FloatArray* zBnds = getZBounds();

  Point3D<float> coords;
  coords[0] = (*xBnds)[idx[0]];
  coords[1] = (*yBnds)[idx[1]];
  coords[2] = (*zBnds)[idx[2]];
  return coords;
}

complex::Point3D<float> RectGridGeom::getPlaneCoordsf(size_t x, size_t y, size_t z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float> coords;
  coords[0] = (*xBnds)[x];
  coords[1] = (*yBnds)[y];
  coords[2] = (*zBnds)[z];
  return coords;
}

complex::Point3D<float> RectGridGeom::getPlaneCoordsf(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float> coords;
  coords[0] = (*xBnds)[column];
  coords[1] = (*yBnds)[row];
  coords[2] = (*zBnds)[plane];
  return coords;
}

complex::Point3D<double> RectGridGeom::getPlaneCoords(size_t idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = static_cast<double>((*xBnds)[idx[0]]);
  coords[1] = static_cast<double>((*yBnds)[idx[1]]);
  coords[2] = static_cast<double>((*zBnds)[idx[2]]);
  return coords;
}

complex::Point3D<double> RectGridGeom::getPlaneCoords(size_t x, size_t y, size_t z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = static_cast<double>((*xBnds)[x]);
  coords[1] = static_cast<double>((*yBnds)[y]);
  coords[2] = static_cast<double>((*zBnds)[z]);
  return coords;
}

complex::Point3D<double> RectGridGeom::getPlaneCoords(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = static_cast<double>((*xBnds)[column]);
  coords[1] = static_cast<double>((*yBnds)[row]);
  coords[2] = static_cast<double>((*zBnds)[plane]);
  return coords;
}

complex::Point3D<float> RectGridGeom::getCoordsf(size_t idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float> coords;
  coords[0] = 0.5f * ((*xBnds)[idx[0]] + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5f * ((*yBnds)[idx[1]] + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5f * ((*zBnds)[idx[2]] + (*zBnds)[idx[2] + 1]);
  return coords;
}

complex::Point3D<float> RectGridGeom::getCoordsf(size_t x, size_t y, size_t z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float> coords;
  coords[0] = 0.5f * ((*xBnds)[x] + (*xBnds)[x + 1]);
  coords[1] = 0.5f * ((*yBnds)[y] + (*yBnds)[y + 1]);
  coords[2] = 0.5f * ((*zBnds)[z] + (*zBnds)[z + 1]);
  return coords;
}

complex::Point3D<float> RectGridGeom::getCoordsf(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<float> coords;
  coords[0] = 0.5f * ((*xBnds)[column] + (*xBnds)[column + 1]);
  coords[1] = 0.5f * ((*yBnds)[row] + (*yBnds)[row + 1]);
  coords[2] = 0.5f * ((*zBnds)[plane] + (*zBnds)[plane + 1]);
  return coords;
}

complex::Point3D<double> RectGridGeom::getCoords(size_t idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = 0.5 * (static_cast<double>((*xBnds)[idx[0]]) + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5 * (static_cast<double>((*yBnds)[idx[1]]) + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5 * (static_cast<double>((*zBnds)[idx[2]]) + (*zBnds)[idx[2] + 1]);
  return coords;
}

complex::Point3D<double> RectGridGeom::getCoords(size_t x, size_t y, size_t z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = 0.5 * (static_cast<double>((*xBnds)[x]) + (*xBnds)[x + 1]);
  coords[1] = 0.5 * (static_cast<double>((*yBnds)[y]) + (*yBnds)[y + 1]);
  coords[2] = 0.5 * (static_cast<double>((*zBnds)[z]) + (*zBnds)[z + 1]);
  return coords;
}

complex::Point3D<double> RectGridGeom::getCoords(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  complex::Point3D<double> coords;
  coords[0] = 0.5 * (static_cast<double>((*xBnds)[column]) + (*xBnds)[column + 1]);
  coords[1] = 0.5 * (static_cast<double>((*yBnds)[row]) + (*yBnds)[row + 1]);
  coords[2] = 0.5 * (static_cast<double>((*zBnds)[plane]) + (*zBnds)[plane + 1]);
  return coords;
}

size_t RectGridGeom::getIndex(float xCoord, float yCoord, float zCoord) const
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

  int64_t x = std::distance(xBnds.begin(), std::adjacent_find(xBnds.begin(), xBnds.end(), [xCoord](const float a, const float b) { return (xCoord >= a && xCoord < b); }));
  int64_t y = std::distance(yBnds.begin(), std::adjacent_find(yBnds.begin(), yBnds.end(), [yCoord](const float a, const float b) { return (yCoord >= a && yCoord < b); }));
  int64_t z = std::distance(zBnds.begin(), std::adjacent_find(zBnds.begin(), zBnds.end(), [zCoord](const float a, const float b) { return (zCoord >= a && zCoord < b); }));

  size_t xSize = xBnds.getSize() - 1;
  size_t ySize = yBnds.getSize() - 1;
  return (ySize * xSize * z) + (xSize * y) + x;
}

size_t RectGridGeom::getIndex(double xCoord, double yCoord, double zCoord) const
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
  size_t x = std::distance(xBnds.begin(), std::adjacent_find(xBnds.begin(), xBnds.end(), [xCoord](const float a, const float b) { return (xCoord >= a && xCoord < b); }));
  size_t y = std::distance(yBnds.begin(), std::adjacent_find(yBnds.begin(), yBnds.end(), [yCoord](const float a, const float b) { return (yCoord >= a && yCoord < b); }));
  size_t z = std::distance(zBnds.begin(), std::adjacent_find(zBnds.begin(), zBnds.end(), [zCoord](const float a, const float b) { return (zCoord >= a && zCoord < b); }));

  size_t xSize = xBnds.getSize() - 1;
  size_t ySize = yBnds.getSize() - 1;
  return (ySize * xSize * z) + (xSize * y) + x;
}

uint32_t RectGridGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

H5::ErrorType RectGridGeom::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::runtime_error("");
}

H5::ErrorType RectGridGeom::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::runtime_error("");
}

void RectGridGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
}

void RectGridGeom::setElementNeighbors(const ElementDynamicList* elementsNeighbors)
{
}

void RectGridGeom::setElementCentroids(const FloatArray* elementCentroids)
{
}

void RectGridGeom::setElementSizes(const FloatArray* elementSizes)
{
  if(!elementSizes)
  {
    m_VoxelSizesId.reset();
    return;
  }
  m_VoxelSizesId = elementSizes->getId();
}
