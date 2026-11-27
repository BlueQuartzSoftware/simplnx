#include "RectGridGeom.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/DataObjectUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <vector>

using namespace nx::core;

RectGridGeom::RectGridGeom(DataStructure& dataStructure, std::string name)
: IGridGeometry(dataStructure, std::move(name))
{
}

RectGridGeom::RectGridGeom(DataStructure& dataStructure, std::string name, IdType importId)
: IGridGeometry(dataStructure, std::move(name), importId)
{
}

IGeometry::Type RectGridGeom::getGeomType() const
{
  return IGeometry::Type::RectGrid;
}

DataObject::Type RectGridGeom::getDataObjectType() const
{
  return DataObject::Type::RectGridGeom;
}

BaseGroup::GroupType RectGridGeom::getGroupType() const
{
  return GroupType::RectGridGeom;
}

RectGridGeom* RectGridGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<RectGridGeom>(new RectGridGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

RectGridGeom* RectGridGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<RectGridGeom>(new RectGridGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string RectGridGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* RectGridGeom::shallowCopy()
{
  return new RectGridGeom(*this);
}

std::shared_ptr<DataObject> RectGridGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // The non-import constructor generates an identifier for the copied geometry.
  auto copy = std::shared_ptr<RectGridGeom>(new RectGridGeom(dataStruct, copyPath.getTargetName()));
  copy->setDimensions(m_Dimensions);
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    IGridGeometry::copyMembersInto(*copy, copyPath);

    copy->m_XBoundsId = deepCopyOwnedChild(copyPath, getXBounds());
    copy->m_YBoundsId = deepCopyOwnedChild(copyPath, getYBounds());
    copy->m_ZBoundsId = deepCopyOwnedChild(copyPath, getZBounds());
    return copy;
  }
  return nullptr;
}

namespace
{
Result<> ValidateBoundsArray(const Float32ArrayType* bounds, std::string_view axis, float32 translation)
{
  if(bounds == nullptr)
  {
    return {};
  }
  const usize size = bounds->getNumberOfTuples();
  if(size < 2)
  {
    return MakeErrorResult(-4006, fmt::format("RectGridGeom: {} bounds array must contain at least 2 values to define at least 1 cell, but {} value(s) provided.", axis, size));
  }
  if(bounds->getNumberOfComponents() != 1)
  {
    return MakeErrorResult(RectGridGeom::k_NonscalarBounds, fmt::format("RectGridGeom: {} bounds array must have one component per tuple, but has {}.", axis, bounds->getNumberOfComponents()));
  }
  const auto& store = bounds->getDataStoreRef();
  if(store.getStoreType() == IDataStore::StoreType::Empty)
  {
    // Preflight stores contain metadata only; their bounds values are not available yet.
    return {};
  }

  std::array<float32, 4096> buffer{};
  float32 previousValue = 0.0f;
  for(usize offset = 0; offset < size;)
  {
    const usize count = std::min(buffer.size(), size - offset);
    auto readResult = store.copyIntoBuffer(offset, nonstd::span<float32>(buffer.data(), count));
    if(readResult.invalid())
    {
      for(auto& error : readResult.errors())
      {
        error.message = fmt::format("Failed to read {} bounds array '{}' starting at index {}: {}", axis, bounds->getName(), offset, error.message);
      }
      return readResult;
    }
    for(usize j = 0; j < count; ++j)
    {
      const usize i = offset + j;
      const float32 currentValue = buffer[j] + translation;
      if(!std::isfinite(currentValue))
      {
        return MakeErrorResult(RectGridGeom::k_NonfiniteBounds, fmt::format("RectGridGeom: {} bounds value at index {} must be finite. Received {}.", axis, i, currentValue));
      }
      if(i > 0 && !(previousValue < currentValue))
      {
        return MakeErrorResult(
            -4007, fmt::format("RectGridGeom: {} bounds array is not strictly monotonically increasing at index {}: bounds[{}]={} >= bounds[{}]={}", axis, i, i - 1, previousValue, i, currentValue));
      }
      previousValue = currentValue;
    }
    offset += count;
  }
  return {};
}
} // namespace

Result<> RectGridGeom::ValidateBounds(const Float32ArrayType* xBounds, const Float32ArrayType* yBounds, const Float32ArrayType* zBounds, const FloatVec3Type& translation)
{
  if(auto result = ValidateBoundsArray(xBounds, "X", translation[0]); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateBoundsArray(yBounds, "Y", translation[1]); result.invalid())
  {
    return result;
  }
  return ValidateBoundsArray(zBounds, "Z", translation[2]);
}

Result<> RectGridGeom::setBounds(const Float32ArrayType* xBounds, const Float32ArrayType* yBounds, const Float32ArrayType* zBounds)
{
  if(auto result = ValidateBounds(xBounds, yBounds, zBounds); result.invalid())
  {
    return result;
  }

  if(xBounds == nullptr)
  {
    m_XBoundsId.reset();
  }
  else
  {
    m_XBoundsId = xBounds->getId();
  }

  if(yBounds == nullptr)
  {
    m_YBoundsId.reset();
  }
  else
  {
    m_YBoundsId = yBounds->getId();
  }

  if(zBounds == nullptr)
  {
    m_ZBoundsId.reset();
  }
  else
  {
    m_ZBoundsId = zBounds->getId();
  }

  return {};
}

Float32ArrayType* RectGridGeom::getXBounds()
{
  return dynamic_cast<Float32ArrayType*>(getDataStructure()->getData(m_XBoundsId));
}

Float32ArrayType* RectGridGeom::getYBounds()
{
  return dynamic_cast<Float32ArrayType*>(getDataStructure()->getData(m_YBoundsId));
}

Float32ArrayType* RectGridGeom::getZBounds()
{
  return dynamic_cast<Float32ArrayType*>(getDataStructure()->getData(m_ZBoundsId));
}

const Float32ArrayType* RectGridGeom::getXBounds() const
{
  return dynamic_cast<const Float32ArrayType*>(getDataStructure()->getData(m_XBoundsId));
}

const Float32ArrayType* RectGridGeom::getYBounds() const
{
  return dynamic_cast<const Float32ArrayType*>(getDataStructure()->getData(m_YBoundsId));
}

const Float32ArrayType* RectGridGeom::getZBounds() const
{
  return dynamic_cast<const Float32ArrayType*>(getDataStructure()->getData(m_ZBoundsId));
}

Float32ArrayType& RectGridGeom::getXBoundsRef()
{
  Float32ArrayType* xBoundsPtr = getXBounds();
  if(xBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getXBoundsRef: X bounds do not exist");
  }
  return *xBoundsPtr;
}

Float32ArrayType& RectGridGeom::getYBoundsRef()
{
  Float32ArrayType* yBoundsPtr = getYBounds();
  if(yBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getYBoundsRef: Y bounds do not exist");
  }
  return *yBoundsPtr;
}

Float32ArrayType& RectGridGeom::getZBoundsRef()
{
  Float32ArrayType* zBoundsPtr = getZBounds();
  if(zBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getZBoundsRef: Z bounds do not exist");
  }
  return *zBoundsPtr;
}

const Float32ArrayType& RectGridGeom::getXBoundsRef() const
{
  const Float32ArrayType* xBoundsPtr = getXBounds();
  if(xBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getXBoundsRef: X bounds do not exist");
  }
  return *xBoundsPtr;
}

const Float32ArrayType& RectGridGeom::getYBoundsRef() const
{
  const Float32ArrayType* yBoundsPtr = getYBounds();
  if(yBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getYBoundsRef: Y bounds do not exist");
  }
  return *yBoundsPtr;
}

const Float32ArrayType& RectGridGeom::getZBoundsRef() const
{
  const Float32ArrayType* zBoundsPtr = getZBounds();
  if(zBoundsPtr == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getZBoundsRef: Z bounds do not exist");
  }
  return *zBoundsPtr;
}

std::shared_ptr<Float32ArrayType> RectGridGeom::getSharedXBounds()
{
  if(!m_XBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32ArrayType>(m_XBoundsId.value());
}

std::shared_ptr<Float32ArrayType> RectGridGeom::getSharedYBounds()
{
  if(!m_YBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32ArrayType>(m_YBoundsId.value());
}

std::shared_ptr<Float32ArrayType> RectGridGeom::getSharedZBounds()
{
  if(!m_ZBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32ArrayType>(m_ZBoundsId.value());
}

DataObject::OptionalIdType RectGridGeom::getXBoundsId() const
{
  return m_XBoundsId;
}
DataObject::OptionalIdType RectGridGeom::getYBoundsId() const
{
  return m_YBoundsId;
}
DataObject::OptionalIdType RectGridGeom::getZBoundsId() const
{
  return m_ZBoundsId;
}

void RectGridGeom::setXBoundsId(const OptionalIdType& xBoundsId)
{
  m_XBoundsId = xBoundsId;
}
void RectGridGeom::setYBoundsId(const OptionalIdType& yBoundsId)
{
  m_YBoundsId = yBoundsId;
}
void RectGridGeom::setZBoundsId(const OptionalIdType& zBoundsId)
{
  m_ZBoundsId = zBoundsId;
}

usize RectGridGeom::getNumberOfCells() const
{
  return m_Dimensions.getX() * m_Dimensions.getY() * m_Dimensions.getZ();
}

Result<> RectGridGeom::findElementSizes(bool recalculate)
{
  auto* sizeArrayPtr = getDataStructureRef().getDataAsUnsafe<Float32ArrayType>(m_ElementSizesId);
  if(sizeArrayPtr != nullptr && !recalculate)
  {
    return {};
  }

  if(sizeArrayPtr == nullptr)
  {
    // The resolver selects a disk-backed store for a large grid when a compatible
    // OOC manager is registered.
    std::vector<DataPath> geomPaths = getDataPaths();
    DataPath sizesPath = geomPaths.empty() ? DataPath({getName(), k_VoxelSizes}) : geomPaths.front().createChildPath(k_VoxelSizes);
    auto dataStore = data_store_utilities::CreateDataStore<float32>(*getDataStructure(), sizesPath, std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1});
    sizeArrayPtr = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(dataStore), getId());
    if(sizeArrayPtr == nullptr)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-1834, "RectGridGeom Error: Unable to find or create a valid element sizes array or data store.");
    }
  }

  const auto* xBndsPtr = getXBounds();
  if(xBndsPtr == nullptr)
  {
    return MakeErrorResult(-1830, "RectGridGeom Error: No valid X Bounds Array");
  }
  const auto* yBndsPtr = getYBounds();
  if(yBndsPtr == nullptr)
  {
    return MakeErrorResult(-1831, "RectGridGeom Error: No valid Y Bounds Array");
  }
  const auto* zBndsPtr = getZBounds();
  if(zBndsPtr == nullptr)
  {
    return MakeErrorResult(-1832, "RectGridGeom Error: No valid Z Bounds Array");
  }

  // Copy each bounds array once and calculate axis spacings in memory. This
  // avoids per-voxel virtual and cache access for out-of-core stores.
  const usize dimX = m_Dimensions[0];
  const usize dimY = m_Dimensions[1];
  const usize dimZ = m_Dimensions[2];

  std::vector<float32> xBndsBuf(dimX + 1);
  std::vector<float32> yBndsBuf(dimY + 1);
  std::vector<float32> zBndsBuf(dimZ + 1);
  Result<> readResult = xBndsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(xBndsBuf.data(), xBndsBuf.size()));
  if(readResult.invalid())
  {
    m_ElementSizesId.reset();
    return readResult;
  }
  readResult = yBndsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(yBndsBuf.data(), yBndsBuf.size()));
  if(readResult.invalid())
  {
    m_ElementSizesId.reset();
    return readResult;
  }
  readResult = zBndsPtr->getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(zBndsBuf.data(), zBndsBuf.size()));
  if(readResult.invalid())
  {
    m_ElementSizesId.reset();
    return readResult;
  }

  std::vector<float32> xRes(dimX);
  std::vector<float32> yRes(dimY);
  std::vector<float32> zRes(dimZ);
  for(usize x = 0; x < dimX; x++)
  {
    xRes[x] = xBndsBuf[x + 1] - xBndsBuf[x];
    if(xRes[x] <= 0.0f)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-1833, fmt::format("RectGridGeom Error: Found voxel with a spacing of zero or less along X.\nX-Index: {} | X-Spacing: {}", x, xRes[x]));
    }
  }
  for(usize y = 0; y < dimY; y++)
  {
    yRes[y] = yBndsBuf[y + 1] - yBndsBuf[y];
    if(yRes[y] <= 0.0f)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-1833, fmt::format("RectGridGeom Error: Found voxel with a spacing of zero or less along Y.\nY-Index: {} | Y-Spacing: {}", y, yRes[y]));
    }
  }
  for(usize z = 0; z < dimZ; z++)
  {
    zRes[z] = zBndsBuf[z + 1] - zBndsBuf[z];
    if(zRes[z] <= 0.0f)
    {
      m_ElementSizesId.reset();
      return MakeErrorResult(-1833, fmt::format("RectGridGeom Error: Found voxel with a spacing of zero or less along Z.\nZ-Index: {} | Z-Spacing: {}", z, zRes[z]));
    }
  }

  // Build one Z slice and write it with bulk I/O. This bounds the temporary
  // voxel-size buffer and avoids per-voxel out-of-core writes.
  auto& sizeStoreRef = sizeArrayPtr->getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<float32> sliceBuf(sliceSize);
  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      const float32 yzRes = yRes[y] * zRes[z];
      for(usize x = 0; x < dimX; x++)
      {
        sliceBuf[y * dimX + x] = xRes[x] * yzRes;
      }
    }
    Result<> writeResult = sizeStoreRef.copyFromBuffer(z * sliceSize, nonstd::span<const float32>(sliceBuf.data(), sliceSize));
    if(writeResult.invalid())
    {
      m_ElementSizesId.reset();
      return writeResult;
    }
  }

  m_ElementSizesId = sizeArrayPtr->getId();

  return {};
}

Vec3<float64> RectGridGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void RectGridGeom::getShapeFunctions(const Vec3<float64>& pCoords, float64* shape) const
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

void RectGridGeom::setDimensions(const SizeVec3Type& dims)
{
  m_Dimensions = dims;
}

SizeVec3Type RectGridGeom::getDimensions() const
{
  return m_Dimensions;
}

Result<FloatVec3Type> RectGridGeom::getOrigin() const
{
  const Float32ArrayType* xBoundsPtr = getXBounds();
  const Float32ArrayType* yBoundsPtr = getYBounds();
  const Float32ArrayType* zBoundsPtr = getZBounds();

  if(xBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3Type>(-4000, "Unable to calculate the RectGridGeom origin - X Bounds array is not available.");
  }

  if(yBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3Type>(-4001, "Unable to calculate the RectGridGeom origin - Y Bounds array is not available.");
  }

  if(zBoundsPtr == nullptr)
  {
    return MakeErrorResult<FloatVec3Type>(-4002, "Unable to calculate the RectGridGeom origin - Z Bounds array is not available.");
  }

  if(xBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3Type>(-4003, "Unable to calculate the RectGridGeom origin - X Bounds array is empty.");
  }

  if(yBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3Type>(-4004, "Unable to calculate the RectGridGeom origin - Y Bounds array is empty.");
  }

  if(zBoundsPtr->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3Type>(-4005, "Unable to calculate the RectGridGeom origin - Z Bounds array is empty.");
  }

  FloatVec3Type origin = {0.0f, 0.0f, 0.0f};
  origin.setX(xBoundsPtr->at(0));
  origin.setY(yBoundsPtr->at(0));
  origin.setZ(zBoundsPtr->at(0));
  return {origin};
}

usize RectGridGeom::getNumXCells() const
{
  return m_Dimensions.getX();
}

usize RectGridGeom::getNumYCells() const
{
  return m_Dimensions.getY();
}

usize RectGridGeom::getNumZCells() const
{
  return m_Dimensions.getZ();
}

Vec3<float32> RectGridGeom::getPlaneCoordsf(usize idx[3]) const
{
  const Float32ArrayType* xBndsPtr = getXBounds();
  const Float32ArrayType* yBndsPtr = getYBounds();
  const Float32ArrayType* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = (*xBndsPtr)[idx[0]];
  coords[1] = (*yBndsPtr)[idx[1]];
  coords[2] = (*zBndsPtr)[idx[2]];
  return coords;
}

Vec3<float32> RectGridGeom::getPlaneCoordsf(usize x, usize y, usize z) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = (*xBndsPtr)[x];
  coords[1] = (*yBndsPtr)[y];
  coords[2] = (*zBndsPtr)[z];
  return coords;
}

Vec3<float32> RectGridGeom::getPlaneCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = (*xBndsPtr)[column];
  coords[1] = (*yBndsPtr)[row];
  coords[2] = (*zBndsPtr)[plane];
  return coords;
}

Vec3<float64> RectGridGeom::getPlaneCoords(usize idx[3]) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = static_cast<float64>((*xBndsPtr)[idx[0]]);
  coords[1] = static_cast<float64>((*yBndsPtr)[idx[1]]);
  coords[2] = static_cast<float64>((*zBndsPtr)[idx[2]]);
  return coords;
}

Vec3<float64> RectGridGeom::getPlaneCoords(usize x, usize y, usize z) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = static_cast<float64>((*xBndsPtr)[x]);
  coords[1] = static_cast<float64>((*yBndsPtr)[y]);
  coords[2] = static_cast<float64>((*zBndsPtr)[z]);
  return coords;
}

Vec3<float64> RectGridGeom::getPlaneCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = static_cast<float64>((*xBndsPtr)[column]);
  coords[1] = static_cast<float64>((*yBndsPtr)[row]);
  coords[2] = static_cast<float64>((*zBndsPtr)[plane]);
  return coords;
}

Vec3<float32> RectGridGeom::getCoordsf(usize idx[3]) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = 0.5f * ((*xBndsPtr)[idx[0]] + (*xBndsPtr)[idx[0] + 1]);
  coords[1] = 0.5f * ((*yBndsPtr)[idx[1]] + (*yBndsPtr)[idx[1] + 1]);
  coords[2] = 0.5f * ((*zBndsPtr)[idx[2]] + (*zBndsPtr)[idx[2] + 1]);
  return coords;
}

Vec3<float32> RectGridGeom::getCoordsf(usize x, usize y, usize z) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = 0.5f * ((*xBndsPtr)[x] + (*xBndsPtr)[x + 1]);
  coords[1] = 0.5f * ((*yBndsPtr)[y] + (*yBndsPtr)[y + 1]);
  coords[2] = 0.5f * ((*zBndsPtr)[z] + (*zBndsPtr)[z + 1]);
  return coords;
}

Vec3<float32> RectGridGeom::getCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float32> coords;
  coords[0] = 0.5f * ((*xBndsPtr)[column] + (*xBndsPtr)[column + 1]);
  coords[1] = 0.5f * ((*yBndsPtr)[row] + (*yBndsPtr)[row + 1]);
  coords[2] = 0.5f * ((*zBndsPtr)[plane] + (*zBndsPtr)[plane + 1]);
  return coords;
}

Vec3<float64> RectGridGeom::getCoords(usize idx[3]) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBndsPtr)[idx[0]]) + (*xBndsPtr)[idx[0] + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBndsPtr)[idx[1]]) + (*yBndsPtr)[idx[1] + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBndsPtr)[idx[2]]) + (*zBndsPtr)[idx[2] + 1]);
  return coords;
}

Vec3<float64> RectGridGeom::getCoords(usize x, usize y, usize z) const
{
  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBndsPtr)[x]) + (*xBndsPtr)[x + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBndsPtr)[y]) + (*yBndsPtr)[y + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBndsPtr)[z]) + (*zBndsPtr)[z + 1]);
  return coords;
}

Vec3<float64> RectGridGeom::getCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  const auto* xBndsPtr = getXBounds();
  const auto* yBndsPtr = getYBounds();
  const auto* zBndsPtr = getZBounds();

  Vec3<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBndsPtr)[column]) + (*xBndsPtr)[column + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBndsPtr)[row]) + (*yBndsPtr)[row + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBndsPtr)[plane]) + (*zBndsPtr)[plane + 1]);
  return coords;
}

std::optional<usize> RectGridGeom::getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const
{
  const auto& xBnds = *getXBounds();
  const auto& yBnds = *getYBounds();
  const auto& zBnds = *getZBounds();

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

std::optional<usize> RectGridGeom::getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const
{
  const auto& xBnds = *getXBounds();
  const auto& yBnds = *getYBounds();
  const auto& zBnds = *getZBounds();

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

  usize x = std::distance(xBnds.begin(), std::adjacent_find(xBnds.begin(), xBnds.end(), [xCoord](const float32 a, const float32 b) { return (xCoord >= a && xCoord < b); }));
  usize y = std::distance(yBnds.begin(), std::adjacent_find(yBnds.begin(), yBnds.end(), [yCoord](const float32 a, const float32 b) { return (yCoord >= a && yCoord < b); }));
  usize z = std::distance(zBnds.begin(), std::adjacent_find(zBnds.begin(), zBnds.end(), [zCoord](const float32 a, const float32 b) { return (zCoord >= a && zCoord < b); }));

  usize xSize = xBnds.getSize() - 1;
  usize ySize = yBnds.getSize() - 1;
  return (ySize * xSize * z) + (xSize * y) + x;
}

void RectGridGeom::checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap)
{
  IGridGeometry::checkUpdatedIdsImpl(updatedIdsMap);
  std::vector<bool> visited(3, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_XBoundsId = nx::core::VisitDataStructureId(m_XBoundsId, updatedId, visited, 0);
    m_YBoundsId = nx::core::VisitDataStructureId(m_YBoundsId, updatedId, visited, 1);
    m_ZBoundsId = nx::core::VisitDataStructureId(m_ZBoundsId, updatedId, visited, 2);
  }
}
