#include "ImageGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <stdexcept>

using namespace nx::core;

ImageGeom::ImageGeom(DataStructure& dataStructure, std::string name)
: IGridGeometry(dataStructure, std::move(name))
{
}

ImageGeom::ImageGeom(DataStructure& dataStructure, std::string name, IdType importId)
: IGridGeometry(dataStructure, std::move(name), importId)
{
}

IGeometry::Type ImageGeom::getGeomType() const
{
  return IGeometry::Type::Image;
}

DataObject::Type ImageGeom::getDataObjectType() const
{
  return DataObject::Type::ImageGeom;
}

BaseGroup::GroupType ImageGeom::getGroupType() const
{
  return GroupType::ImageGeom;
}

ImageGeom* ImageGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<ImageGeom>(new ImageGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

ImageGeom* ImageGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<ImageGeom>(new ImageGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string ImageGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* ImageGeom::shallowCopy()
{
  return new ImageGeom(*this);
}

std::shared_ptr<DataObject> ImageGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<ImageGeom>(new ImageGeom(dataStruct, copyPath.getTargetName()));
  copy->setOrigin(m_Origin);
  copy->setSpacing(m_Spacing);
  copy->setDimensions(m_Dimensions);
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    if(m_CellDataId.has_value())
    {
      const DataPath copiedCellDataPath = copyPath.createChildPath(getCellData()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getCellData()))
      {
        const auto cellDataCopy = getCellData()->deepCopy(copiedCellDataPath);
      }
      copy->m_CellDataId = dataStruct.getId(copiedCellDataPath);
    }

    if(const auto voxelSizesCopy = dataStruct.getDataAs<Float32Array>(copyPath.createChildPath(k_VoxelSizes)); voxelSizesCopy != nullptr)
    {
      copy->m_ElementSizesId = voxelSizesCopy->getId();
    }
    return copy;
  }
  return nullptr;
}

FloatVec3 ImageGeom::getSpacing() const
{
  return m_Spacing;
}

void ImageGeom::setSpacing(const FloatVec3& spacing)
{
  m_Spacing = spacing;
}

void ImageGeom::setSpacing(float32 x, float32 y, float32 z)
{
  m_Spacing = {x, y, z};
}

FloatVec3 ImageGeom::getOrigin() const
{
  return m_Origin;
}

void ImageGeom::setOrigin(const FloatVec3& origin)
{
  m_Origin = origin;
}

void ImageGeom::setOrigin(float32 x, float32 y, float32 z)
{
  m_Origin = {x, y, z};
}

BoundingBox<float32> ImageGeom::getBoundingBoxf() const
{
  std::array<float32, 6> arr{
      m_Origin[0], m_Origin[1], m_Origin[2], m_Origin[0] + (m_Dimensions[0] * m_Spacing[0]), m_Origin[1] + (m_Dimensions[1] * m_Spacing[1]), m_Origin[2] + (m_Dimensions[2] * m_Spacing[2])};
  return BoundingBox<float32>(arr);
}

BoundingBox<float64> ImageGeom::getBoundingBox() const
{
  std::array<float64, 6> arr{
      m_Origin[0], m_Origin[1], m_Origin[2], m_Origin[0] + (m_Dimensions[0] * m_Spacing[0]), m_Origin[1] + (m_Dimensions[1] * m_Spacing[1]), m_Origin[2] + (m_Dimensions[2] * m_Spacing[2])};
  return BoundingBox<float64>(arr);
}

usize ImageGeom::getNumberOfCells() const
{
  return (m_Dimensions[0] * m_Dimensions[1] * m_Dimensions[2]);
}

IGeometry::StatusCode ImageGeom::findElementSizes(bool recalculate)
{
  auto* voxelSizes = getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
  if(voxelSizes != nullptr && !recalculate)
  {
    return 0;
  }

  FloatVec3 res = getSpacing();

  if(res[0] <= 0.0f || res[1] <= 0.0f || res[2] <= 0.0f)
  {
    m_ElementSizesId.reset();
    return -1;
  }

  if(voxelSizes == nullptr)
  {
    float32 initValue = res[0] * res[1] * res[2];
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, initValue);
    voxelSizes = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
  }
  if(voxelSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = voxelSizes->getId();
  return 1;
}

Point3D<float64> ImageGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void ImageGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
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

usize ImageGeom::getDimensionality() const
{
  SizeVec3 dims = getDimensions();
  usize numberOfOnes = std::count(dims.begin(), dims.end(), 1);

  if(numberOfOnes == 0)
  {
    return 3;
  }

  if(numberOfOnes == 1)
  {
    return 2;
  }

  return 1;
}

SizeVec3 ImageGeom::getDimensions() const
{
  return m_Dimensions;
}

void ImageGeom::setDimensions(const SizeVec3& dims)
{
  m_Dimensions = dims;
}

usize ImageGeom::getNumXCells() const
{
  return m_Dimensions.getX();
}

usize ImageGeom::getNumYCells() const
{
  return m_Dimensions.getY();
}

usize ImageGeom::getNumZCells() const
{
  return m_Dimensions.getZ();
}

Point3D<float32> ImageGeom::getPlaneCoordsf(usize idx[3]) const
{
  Point3D<float32> coords;
  coords[0] = idx[0] * m_Spacing[0] + m_Origin[0];
  coords[1] = idx[1] * m_Spacing[1] + m_Origin[1];
  coords[2] = idx[2] * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float32> ImageGeom::getPlaneCoordsf(usize x, usize y, usize z) const
{
  Point3D<float32> coords;
  coords[0] = x * m_Spacing[0] + m_Origin[0];
  coords[1] = y * m_Spacing[1] + m_Origin[1];
  coords[2] = z * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float32> ImageGeom::getPlaneCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float32> coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0];
  coords[1] = row * m_Spacing[1] + m_Origin[1];
  coords[2] = plane * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float64> ImageGeom::getPlaneCoords(usize idx[3]) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(idx[0]) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(idx[1]) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(idx[2]) * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float64> ImageGeom::getPlaneCoords(usize x, usize y, usize z) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(x) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(y) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(z) * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float64> ImageGeom::getPlaneCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float64> coords;
  coords[0] = static_cast<float64>(column) * m_Spacing[0] + m_Origin[0];
  coords[1] = static_cast<float64>(row) * m_Spacing[1] + m_Origin[1];
  coords[2] = static_cast<float64>(plane) * m_Spacing[2] + m_Origin[2];
  return coords;
}

Point3D<float32> ImageGeom::getCoordsf(usize idx[3]) const
{
  Point3D<float32> coords;
  coords[0] = idx[0] * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = idx[1] * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = idx[2] * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

Point3D<float32> ImageGeom::getCoordsf(usize x, usize y, usize z) const
{
  Point3D<float32> coords;
  coords[0] = x * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = y * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = z * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

Point3D<float32> ImageGeom::getCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float32> coords;
  coords[0] = column * m_Spacing[0] + m_Origin[0] + (0.5f * m_Spacing[0]);
  coords[1] = row * m_Spacing[1] + m_Origin[1] + (0.5f * m_Spacing[1]);
  coords[2] = plane * m_Spacing[2] + m_Origin[2] + (0.5f * m_Spacing[2]);
  return coords;
}

Point3D<float64> ImageGeom::getCoords(usize idx[3]) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(idx[0]) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(idx[1]) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(idx[2]) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

Point3D<float64> ImageGeom::getCoords(usize x, usize y, usize z) const
{
  Point3D<float64> coords;
  coords[0] = static_cast<float64>(x) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(y) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(z) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

Point3D<float64> ImageGeom::getCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  Point3D<float64> coords;
  coords[0] = static_cast<float64>(column) * m_Spacing[0] + m_Origin[0] + (0.5 * m_Spacing[0]);
  coords[1] = static_cast<float64>(row) * m_Spacing[1] + m_Origin[1] + (0.5 * m_Spacing[1]);
  coords[2] = static_cast<float64>(plane) * m_Spacing[2] + m_Origin[2] + (0.5 * m_Spacing[2]);
  return coords;
}

std::optional<usize> ImageGeom::getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const
{
  if(xCoord < m_Origin[0] || xCoord > (static_cast<float32>(m_Dimensions[0]) * m_Spacing[0] + m_Origin[0]))
  {
    return {};
  }

  if(yCoord < m_Origin[1] || yCoord > (static_cast<float32>(m_Dimensions[1]) * m_Spacing[1] + m_Origin[1]))
  {
    return {};
  }

  if(zCoord < m_Origin[2] || zCoord > (static_cast<float32>(m_Dimensions[2]) * m_Spacing[2] + m_Origin[2]))
  {
    return {};
  }

  usize x = static_cast<usize>(std::floor((xCoord - m_Origin[0]) / m_Spacing[0]));
  if(x >= m_Dimensions[0])
  {
    return {};
  }

  usize y = static_cast<usize>(std::floor((yCoord - m_Origin[1]) / m_Spacing[1]));
  if(y >= m_Dimensions[1])
  {
    return {};
  }

  usize z = static_cast<usize>(std::floor((zCoord - m_Origin[2]) / m_Spacing[2]));
  if(z >= m_Dimensions[2])
  {
    return {};
  }

  return (m_Dimensions[1] * m_Dimensions[0] * z) + (m_Dimensions[0] * y) + x;
}

std::optional<usize> ImageGeom::getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const
{
  if(xCoord < m_Origin[0] || xCoord > (static_cast<float64>(m_Dimensions[0]) * m_Spacing[0] + m_Origin[0]))
  {
    return {};
  }

  if(yCoord < m_Origin[1] || yCoord > (static_cast<float64>(m_Dimensions[1]) * m_Spacing[1] + m_Origin[1]))
  {
    return {};
  }

  if(zCoord < m_Origin[2] || zCoord > (static_cast<float64>(m_Dimensions[2]) * m_Spacing[2] + m_Origin[2]))
  {
    return {};
  }

  usize x = static_cast<usize>(std::floor((xCoord - m_Origin[0]) / m_Spacing[0]));
  if(x >= m_Dimensions[0])
  {
    return {};
  }

  usize y = static_cast<usize>(std::floor((yCoord - m_Origin[1]) / m_Spacing[1]));
  if(y >= m_Dimensions[1])
  {
    return {};
  }

  usize z = static_cast<usize>(std::floor((zCoord - m_Origin[2]) / m_Spacing[2]));
  if(z >= m_Dimensions[2])
  {
    return {};
  }

  return (m_Dimensions[1] * m_Dimensions[0] * z) + (m_Dimensions[0] * y) + x;
}

ImageGeom::ErrorType ImageGeom::computeCellIndex(const Point3D<float32>& coords, SizeVec3& index) const
{
  ImageGeom::ErrorType err = ImageGeom::ErrorType::NoError;
  for(usize i = 0; i < 3; i++)
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
    if(index[i] >= m_Dimensions[i])
    {
      return static_cast<ImageGeom::ErrorType>(i * 2 + 1);
    }
  }
  return err;
}
