#include "ImageGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

using namespace complex;

ImageGeom::ImageGeom(DataStructure& ds, const std::string& name)
: AbstractGeometryGrid(ds, name)
{
}

ImageGeom::ImageGeom(const ImageGeom& other)
: AbstractGeometryGrid(other)
, m_VoxelSizesId(other.m_VoxelSizesId)
, m_Spacing(other.m_Spacing)
, m_Origin(other.m_Origin)
, m_Dimensions(other.m_Dimensions)
{
}

ImageGeom::ImageGeom(ImageGeom&& other) noexcept
: AbstractGeometryGrid(std::move(other))
, m_VoxelSizesId(std::move(other.m_VoxelSizesId))
, m_Spacing(std::move(other.m_Spacing))
, m_Origin(std::move(other.m_Origin))
, m_Dimensions(std::move(other.m_Dimensions))
{
}

ImageGeom::~ImageGeom() = default;

ImageGeom* ImageGeom::Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<ImageGeom>(new ImageGeom(ds, name));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string ImageGeom::getTypeName() const
{
  return "ImageGeom";
}

DataObject* ImageGeom::shallowCopy()
{
  return new ImageGeom(*this);
}

DataObject* ImageGeom::deepCopy()
{
  throw std::runtime_error("");
}

std::string ImageGeom::getGeometryTypeAsString() const
{
  return "ImageGeom";
}

complex::FloatVec3 ImageGeom::getSpacing() const
{
  return m_Spacing;
}

void ImageGeom::setSpacing(const complex::FloatVec3& spacing)
{
  m_Spacing = spacing;
}

void ImageGeom::setSpacing(float32 x, float32 y, float32 z)
{
  m_Spacing = {x, y, z};
}

complex::FloatVec3 ImageGeom::getOrigin() const
{
  return m_Origin;
}

void ImageGeom::setOrigin(const complex::FloatVec3& origin)
{
  m_Origin = origin;
}

void ImageGeom::setOrigin(float32 x, float32 y, float32 z)
{
  m_Origin = {x, y, z};
}

BoundingBox<float32> ImageGeom::getBoundingBoxf() const
{
  std::array<float32, 6> arr{m_Origin[0], m_Origin[0] + (m_Dimensions[0] * m_Spacing[0]), m_Origin[1], m_Origin[1] + (m_Dimensions[1] * m_Spacing[1]),
                             m_Origin[2], m_Origin[2] + (m_Dimensions[2] * m_Spacing[2])};
  return BoundingBox<float32>(arr);
}

BoundingBox<float64> ImageGeom::getBoundingBox() const
{
  std::array<float64, 6> arr{m_Origin[0], m_Origin[0] + (m_Dimensions[0] * m_Spacing[0]), m_Origin[1], m_Origin[1] + (m_Dimensions[1] * m_Spacing[1]),
                             m_Origin[2], m_Origin[2] + (m_Dimensions[2] * m_Spacing[2])};
  return BoundingBox<float64>(arr);
}

void ImageGeom::initializeWithZeros()
{
  for(size_t i = 0; i < 3; i++)
  {
    m_Dimensions[i] = 0;
    m_Spacing[i] = 1.0f;
    m_Origin[i] = 0.0f;
  }
}

size_t ImageGeom::getNumberOfElements() const
{
  return (m_Dimensions[0] * m_Dimensions[1] * m_Dimensions[2]);
}

AbstractGeometry::StatusCode ImageGeom::findElementSizes()
{
  FloatVec3 res = getSpacing();

  if(res[0] <= 0.0f || res[1] <= 0.0f || res[2] <= 0.0f)
  {
    return -1;
  }
  auto dataStore = new DataStore<float32>(1, getNumberOfElements());
  auto voxelSizes = DataArray<float32>::Create(*getDataStructure(), "Voxel Sizes", dataStore, getId());
  voxelSizes->getDataStore()->fill(res[0] * res[1] * res[2]);
  m_VoxelSizesId = voxelSizes->getId();
  return 1;
}

const Float32Array* ImageGeom::getElementSizes() const
{
  return dynamic_cast<const Float32Array*>(getDataStructure()->getData(m_VoxelSizesId));
}

void ImageGeom::deleteElementSizes()
{
  getDataStructure()->removeData(m_VoxelSizesId);
  m_VoxelSizesId.reset();
}

AbstractGeometry::StatusCode ImageGeom::findElementsContainingVert()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* ImageGeom::getElementsContainingVert() const
{
  return nullptr;
}

void ImageGeom::deleteElementsContainingVert()
{
}

AbstractGeometry::StatusCode ImageGeom::findElementNeighbors()
{
  return -1;
}

const AbstractGeometry::ElementDynamicList* ImageGeom::getElementNeighbors() const
{
  return nullptr;
}

void ImageGeom::deleteElementNeighbors()
{
}

AbstractGeometry::StatusCode ImageGeom::findElementCentroids()
{
  return -1;
}

const Float32Array* ImageGeom::getElementCentroids() const
{
  return nullptr;
}

void ImageGeom::deleteElementCentroids()
{
}

complex::Point3D<float64> ImageGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void ImageGeom::getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const
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

void ImageGeom::findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const
{
  throw std::runtime_error("");
}

complex::TooltipGenerator ImageGeom::getTooltipGenerator() const
{
  TooltipGenerator toolTipGen;

  std::string lengthUnit = LengthUnitToString(getUnits());
  int64 volDims[3] = {static_cast<int64>(getNumXPoints()), static_cast<int64>(getNumYPoints()), static_cast<int64>(getNumZPoints())};
  FloatVec3 spacing = getSpacing();
  FloatVec3 origin = getOrigin();

  float32 halfRes[3] = {spacing[0] / 2.0f, spacing[1] / 2.0f, spacing[2] / 2.0f};
  float32 vol = (volDims[0] * spacing[0]) * (volDims[1] * spacing[1]) * (volDims[2] * spacing[2]);

  toolTipGen.addTitle("Geometry Info");
  toolTipGen.addValue("Type", "ImageGeom");
  toolTipGen.addValue("Units", LengthUnitToString(getUnits()));
  toolTipGen.addTitle("Extents");
  toolTipGen.addValue("X", "0 to " + std::to_string(volDims[0] - 1) + " (dimension: " + std::to_string(volDims[0]) + ")");
  toolTipGen.addValue("Y", "0 to " + std::to_string(volDims[1] - 1) + " (dimension: " + std::to_string(volDims[1]) + ")");
  toolTipGen.addValue("Z", "0 to " + std::to_string(volDims[2] - 1) + " (dimension: " + std::to_string(volDims[2]) + ")");
  toolTipGen.addValue("Origin", std::to_string(origin[0]) + ", " + std::to_string(origin[1]) + ", " + std::to_string(origin[2]));
  toolTipGen.addValue("Spacing", std::to_string(spacing[0]) + ", " + std::to_string(spacing[1]) + ", " + std::to_string(spacing[2]));
  toolTipGen.addValue("Volume", std::to_string(vol) + " " + lengthUnit + "s ^3");
  toolTipGen.addTitle("Bounds (Cell Centered)");
  toolTipGen.addValue("X Range",
                      std::to_string(origin[0] - halfRes[0]) + " to " + std::to_string(origin[0] - halfRes[0] + volDims[0] * spacing[0]) + " (delta: " + std::to_string(volDims[0] * spacing[0]) + ")");
  toolTipGen.addValue("Y Range",
                      std::to_string(origin[1] - halfRes[1]) + " to " + std::to_string(origin[1] - halfRes[1] + volDims[1] * spacing[1]) + " (delta: " + std::to_string(volDims[1] * spacing[1]) + ")");
  toolTipGen.addValue("Z Range",
                      std::to_string(origin[2] - halfRes[2]) + " to " + std::to_string(origin[2] - halfRes[2] + volDims[2] * spacing[2]) + " (delta: " + std::to_string(volDims[2] * spacing[2]) + ")");

  return toolTipGen;
}

complex::SizeVec3 ImageGeom::getDimensions() const
{
  return m_Dimensions;
}

void ImageGeom::setDimensions(const complex::SizeVec3& dims)
{
  m_Dimensions = dims;
}

size_t ImageGeom::getNumXPoints() const
{
  return m_Dimensions.getX();
}

size_t ImageGeom::getNumYPoints() const
{
  return m_Dimensions.getY();
}

size_t ImageGeom::getNumZPoints() const
{
  return m_Dimensions.getZ();
}

complex::Point3D<float32> ImageGeom::getPlaneCoordsf(size_t idx[3]) const
{
  Point3D<float32> coords;
  coords[0] = idx[0] * m_Spacing[0] + m_Origin[0];
  coords[1] = idx[1] * m_Spacing[1] + m_Origin[1];
  coords[2] = idx[2] * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float32> ImageGeom::getPlaneCoordsf(size_t x, size_t y, size_t z) const
{
  Point3D<float32> coords;
  coords[0] = x * m_Spacing[0] + m_Origin[0];
  coords[1] = y * m_Spacing[1] + m_Origin[1];
  coords[2] = z * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float32> ImageGeom::getPlaneCoordsf(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float32> coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0];
  coords[1] = row * m_Spacing[1] + m_Origin[1];
  coords[2] = plane * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float64> ImageGeom::getPlaneCoords(size_t idx[3]) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(idx[0]) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(idx[1]) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(idx[2]) * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float64> ImageGeom::getPlaneCoords(size_t x, size_t y, size_t z) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(x) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(y) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(z) * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float64> ImageGeom::getPlaneCoords(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float64> coords;
  coords[0] = static_cast<float64>(column) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(row) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(plane) * m_Spacing[2] + m_Origin[2];
  return coords;
}

complex::Point3D<float32> ImageGeom::getCoordsf(size_t idx[3]) const
{
  Point3D<float32> coords;
  coords[0] = idx[0] * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = idx[1] * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = idx[2] * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

complex::Point3D<float32> ImageGeom::getCoordsf(size_t x, size_t y, size_t z) const
{
  Point3D<float32> coords;
  coords[0] = x * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = y * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = z * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

complex::Point3D<float32> ImageGeom::getCoordsf(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float32> coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = row * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = plane * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

complex::Point3D<float64> ImageGeom::getCoords(size_t idx[3]) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(idx[0]) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(idx[1]) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(idx[2]) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

complex::Point3D<float64> ImageGeom::getCoords(size_t x, size_t y, size_t z) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(x) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(y) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(z) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

complex::Point3D<float64> ImageGeom::getCoords(size_t idx) const
{
  size_t column = idx % m_Dimensions[0];
  size_t row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  size_t plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float64> coords;
  coords[0] = static_cast<float64>(column) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(row) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(plane) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

size_t ImageGeom::getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const
{
  size_t x = (xCoord - (0.5f * m_Spacing[0]) - m_Origin[0]) / m_Spacing[0];
  size_t y = (yCoord - (0.5f * m_Spacing[1]) - m_Origin[1]) / m_Spacing[1];
  size_t z = (zCoord - (0.5f * m_Spacing[2]) - m_Origin[2]) / m_Spacing[2];

  return (m_Dimensions[1] * m_Dimensions[0] * z) + (m_Dimensions[0] * y) + x;
}

size_t ImageGeom::getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const
{
  size_t x = (xCoord - (0.5 * m_Spacing[0]) - m_Origin[0]) / m_Spacing[0];
  size_t y = (yCoord - (0.5 * m_Spacing[1]) - m_Origin[1]) / m_Spacing[1];
  size_t z = (zCoord - (0.5 * m_Spacing[2]) - m_Origin[2]) / m_Spacing[2];

  return (m_Dimensions[1] * m_Dimensions[0] * z) + (m_Dimensions[0] * y) + x;
}

ImageGeom::ErrorType ImageGeom::computeCellIndex(const complex::Point3D<float32>& coords, SizeVec3& index) const
{
  ImageGeom::ErrorType err = ImageGeom::ErrorType::NoError;
  for(size_t i = 0; i < 3; i++)
  {
    if(coords[i] < m_Origin[i])
    {
      return static_cast<ImageGeom::ErrorType>(i * 2);
    }
    if(coords[i] > (m_Origin[i] + m_Dimensions[i] * m_Spacing[i]))
    {
      return static_cast<ImageGeom::ErrorType>(i * 2 + 1);
    }
    index[i] = static_cast<usize>((coords[i] - m_Origin[i]) / m_Spacing[i]);
    if(index[i] > m_Dimensions[i])
    {
      return static_cast<ImageGeom::ErrorType>(i * 2 + 1);
    }
  }
  return err;
}

uint32 ImageGeom::getXdmfGridType() const
{
  throw std::runtime_error("");
}

void ImageGeom::setElementsContainingVert(const ElementDynamicList* elementsContainingVert)
{
  (void)elementsContainingVert;
}

void ImageGeom::setElementNeighbors(const ElementDynamicList* elementsNeighbors)
{
  (void)elementsNeighbors;
}

void ImageGeom::setElementCentroids(const Float32Array* elementCentroids)
{
  (void)elementCentroids;
}

void ImageGeom::setElementSizes(const Float32Array* elementSizes)
{
  if(!elementSizes)
  {
    m_VoxelSizesId.reset();
    return;
  }
  m_VoxelSizesId = elementSizes->getId();
}

H5::ErrorType ImageGeom::readHdf5(H5::IdType targetId, H5::IdType groupId)
{
  return getDataMap().readH5Group(*getDataStructure(), targetId);
}

H5::ErrorType ImageGeom::writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const
{
  return getDataMap().writeH5Group(groupId);
}
