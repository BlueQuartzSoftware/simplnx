#include "ImageGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <array>
#include <cmath>
#include <stdexcept>

using namespace nx::core;

namespace
{
constexpr std::array<char, 3> k_Axes = {'X', 'Y', 'Z'};
} // namespace

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
  // The non-import constructor generates an identifier for the copied geometry.
  auto copy = std::shared_ptr<ImageGeom>(new ImageGeom(dataStruct, copyPath.getTargetName()));
  // The source geometry already enforces the metadata invariant.
  copy->m_Origin = m_Origin;
  copy->m_Spacing = m_Spacing;
  copy->setDimensions(m_Dimensions);
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    IGridGeometry::copyMembersInto(*copy, copyPath);
    return copy;
  }
  return nullptr;
}

FloatVec3 ImageGeom::getSpacing() const
{
  return m_Spacing;
}

Result<> ImageGeom::ValidateSpacing(const FloatVec3& spacing)
{
  for(usize axis = 0; axis < k_Axes.size(); ++axis)
  {
    const float32 value = spacing[axis];
    if(!std::isfinite(value) || !(value > 0.0f))
    {
      return MakeErrorResult(k_InvalidSpacing, fmt::format("Spacing on the {} axis must be finite and greater than zero. Received {}.", k_Axes[axis], value));
    }
  }
  return {};
}

Result<> ImageGeom::setSpacing(const FloatVec3& spacing)
{
  auto result = ValidateSpacing(spacing);
  if(result.valid())
  {
    m_Spacing = spacing;
  }
  else
  {
    result.errors().front().message = fmt::format("Image geometry '{}': {}", getName(), result.errors().front().message);
  }
  return result;
}

Result<> ImageGeom::setSpacing(float32 x, float32 y, float32 z)
{
  return setSpacing(FloatVec3{x, y, z});
}

FloatVec3 ImageGeom::getOrigin() const
{
  return m_Origin;
}

Result<> ImageGeom::ValidateOrigin(const FloatVec3& origin)
{
  for(usize axis = 0; axis < k_Axes.size(); ++axis)
  {
    const float32 value = origin[axis];
    if(!std::isfinite(value))
    {
      return MakeErrorResult(k_InvalidOrigin, fmt::format("Origin on the {} axis must be finite. Received {}.", k_Axes[axis], value));
    }
  }
  return {};
}

Result<> ImageGeom::setOrigin(const FloatVec3& origin)
{
  auto result = ValidateOrigin(origin);
  if(result.valid())
  {
    m_Origin = origin;
  }
  else
  {
    result.errors().front().message = fmt::format("Image geometry '{}': {}", getName(), result.errors().front().message);
  }
  return result;
}

Result<> ImageGeom::setOrigin(float32 x, float32 y, float32 z)
{
  return setOrigin(FloatVec3{x, y, z});
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
  return m_Dimensions[0] * m_Dimensions[1] * m_Dimensions[2];
}

Result<> ImageGeom::findElementSizes(bool recalculate)
{
  auto* voxelSizesPtr = getDataStructureRef().getDataAsUnsafe<Float32Array>(m_ElementSizesId);
  if(voxelSizesPtr != nullptr && !recalculate)
  {
    return {};
  }

  // The product retains supplied empty-axis thickness for one- and two-dimensional grids.
  const float32 singleVoxelSize = m_Spacing[0] * m_Spacing[1] * m_Spacing[2];

  if(voxelSizesPtr == nullptr)
  {
    // The resolver selects a disk-backed store for a large grid when a compatible
    // OOC manager is registered.
    std::vector<DataPath> geomPaths = getDataPaths();
    DataPath voxelSizesPath = geomPaths.empty() ? DataPath({getName(), k_VoxelSizes}) : geomPaths.front().createChildPath(k_VoxelSizes);
    auto dataStore = DataStoreUtilities::CreateDataStore<float32>(*getDataStructure(), voxelSizesPath, std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1});
    voxelSizesPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(voxelSizesPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-1532, "ImageGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
    m_ElementSizesId = voxelSizesPtr->getId();
  }
  voxelSizesPtr->fill(singleVoxelSize);

  return {};
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

  // Derivatives with respect to r.
  shape[0] = -sm * tm;
  shape[1] = sm * tm;
  shape[2] = -pCoords[1] * tm;
  shape[3] = pCoords[1] * tm;
  shape[4] = -sm * pCoords[2];
  shape[5] = sm * pCoords[2];
  shape[6] = -pCoords[1] * pCoords[2];
  shape[7] = pCoords[1] * pCoords[2];

  // Derivatives with respect to s.
  shape[8] = -rm * tm;
  shape[9] = -pCoords[0] * tm;
  shape[10] = rm * tm;
  shape[11] = pCoords[0] * tm;
  shape[12] = -rm * pCoords[2];
  shape[13] = -pCoords[0] * pCoords[2];
  shape[14] = rm * pCoords[2];
  shape[15] = pCoords[0] * pCoords[2];

  // Derivatives with respect to t.
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
