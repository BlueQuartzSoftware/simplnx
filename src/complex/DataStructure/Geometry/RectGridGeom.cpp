#include "RectGridGeom.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

#include <iterator>
#include <stdexcept>

using namespace complex;

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
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<RectGridGeom>(new RectGridGeom(dataStruct, copyPath.getTargetName()));
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

    if(m_xBoundsId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getXBounds()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getXBounds()))
      {
        const auto dataObjCopy = getXBounds()->deepCopy(copiedDataPath);
      }
      copy->m_xBoundsId = dataStruct.getId(copiedDataPath);
    }
    if(m_yBoundsId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getYBounds()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getYBounds()))
      {
        const auto dataObjCopy = getYBounds()->deepCopy(copiedDataPath);
      }
      copy->m_yBoundsId = dataStruct.getId(copiedDataPath);
    }
    if(m_zBoundsId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getZBounds()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getZBounds()))
      {
        const auto dataObjCopy = getZBounds()->deepCopy(copiedDataPath);
      }
      copy->m_zBoundsId = dataStruct.getId(copiedDataPath);
    }

    if(const auto voxelSizesCopy = dataStruct.getDataAs<Float32Array>(copyPath.createChildPath(k_VoxelSizes)); voxelSizesCopy != nullptr)
    {
      copy->m_ElementSizesId = voxelSizesCopy->getId();
    }
    return copy;
  }
  return nullptr;
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

Float32Array* RectGridGeom::getXBounds()
{
  return dynamic_cast<Float32Array*>(getDataStructure()->getData(m_xBoundsId));
}

Float32Array* RectGridGeom::getYBounds()
{
  return dynamic_cast<Float32Array*>(getDataStructure()->getData(m_yBoundsId));
}

Float32Array* RectGridGeom::getZBounds()
{
  return dynamic_cast<Float32Array*>(getDataStructure()->getData(m_zBoundsId));
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

Float32Array& RectGridGeom::getXBoundsRef()
{
  Float32Array* xBounds = getXBounds();
  if(xBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getXBoundsRef: X bounds do not exist");
  }
  return *xBounds;
}

Float32Array& RectGridGeom::getYBoundsRef()
{
  Float32Array* yBounds = getYBounds();
  if(yBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getYBoundsRef: Y bounds do not exist");
  }
  return *yBounds;
}

Float32Array& RectGridGeom::getZBoundsRef()
{
  Float32Array* zBounds = getZBounds();
  if(zBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getZBoundsRef: Z bounds do not exist");
  }
  return *zBounds;
}

const Float32Array& RectGridGeom::getXBoundsRef() const
{
  const Float32Array* xBounds = getXBounds();
  if(xBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getXBoundsRef: X bounds do not exist");
  }
  return *xBounds;
}

const Float32Array& RectGridGeom::getYBoundsRef() const
{
  const Float32Array* yBounds = getYBounds();
  if(yBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getYBoundsRef: Y bounds do not exist");
  }
  return *yBounds;
}

const Float32Array& RectGridGeom::getZBoundsRef() const
{
  const Float32Array* zBounds = getZBounds();
  if(zBounds == nullptr)
  {
    throw std::runtime_error("RectGridGeom::getZBoundsRef: Z bounds do not exist");
  }
  return *zBounds;
}

std::shared_ptr<Float32Array> RectGridGeom::getSharedXBounds()
{
  if(!m_xBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32Array>(m_xBoundsId.value());
}

std::shared_ptr<Float32Array> RectGridGeom::getSharedYBounds()
{
  if(!m_yBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32Array>(m_yBoundsId.value());
}

std::shared_ptr<Float32Array> RectGridGeom::getSharedZBounds()
{
  if(!m_zBoundsId.has_value())
  {
    return nullptr;
  }
  return getDataStructure()->getSharedDataAs<Float32Array>(m_zBoundsId.value());
}

DataObject::OptionalId RectGridGeom::getXBoundsId() const
{
  return m_xBoundsId;
}
DataObject::OptionalId RectGridGeom::getYBoundsId() const
{
  return m_yBoundsId;
}
DataObject::OptionalId RectGridGeom::getZBoundsId() const
{
  return m_zBoundsId;
}

void RectGridGeom::setXBoundsId(const OptionalId& xBoundsId)
{
  m_xBoundsId = xBoundsId;
}
void RectGridGeom::setYBoundsId(const OptionalId& yBoundsId)
{
  m_yBoundsId = yBoundsId;
}
void RectGridGeom::setZBoundsId(const OptionalId& zBoundsId)
{
  m_zBoundsId = zBoundsId;
}

usize RectGridGeom::getNumberOfCells() const
{
  return m_Dimensions.getX() * m_Dimensions.getY() * m_Dimensions.getZ();
}

IGeometry::StatusCode RectGridGeom::findElementSizes()
{
  auto sizes = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
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
          m_ElementSizesId.reset();
          return -1;
        }
        (*sizes)[(m_Dimensions[0] * m_Dimensions[1] * z) + (m_Dimensions[0] * y) + x] = zRes * yRes * xRes;
      }
    }
  }

  Float32Array* sizeArray = DataArray<float32>::Create(*getDataStructure(), k_VoxelSizes, std::move(sizes), getId());
  if(!sizeArray)
  {
    m_ElementSizesId.reset();
    return -1;
  }

  m_ElementSizesId = sizeArray->getId();
  return 1;
}

Point3D<float64> RectGridGeom::getParametricCenter() const
{
  return {0.5, 0.5, 0.5};
}

void RectGridGeom::getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const
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

void RectGridGeom::setDimensions(const SizeVec3& dims)
{
  m_Dimensions = dims;
}

SizeVec3 RectGridGeom::getDimensions() const
{
  return m_Dimensions;
}

Result<FloatVec3> RectGridGeom::getOrigin() const
{
  const Float32Array* xBounds = getXBounds();
  const Float32Array* yBounds = getYBounds();
  const Float32Array* zBounds = getZBounds();

  if(xBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4000, "Unable to calculate the RectGridGeom origin - X Bounds array is not available.");
  }

  if(yBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4001, "Unable to calculate the RectGridGeom origin - Y Bounds array is not available.");
  }

  if(zBounds == nullptr)
  {
    return MakeErrorResult<FloatVec3>(-4002, "Unable to calculate the RectGridGeom origin - Z Bounds array is not available.");
  }

  if(xBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4003, "Unable to calculate the RectGridGeom origin - X Bounds array is empty.");
  }

  if(yBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4004, "Unable to calculate the RectGridGeom origin - Y Bounds array is empty.");
  }

  if(zBounds->getSize() == 0)
  {
    return MakeErrorResult<FloatVec3>(-4005, "Unable to calculate the RectGridGeom origin - Z Bounds array is empty.");
  }

  FloatVec3 origin = {0.0f, 0.0f, 0.0f};
  origin.setX(xBounds->at(0));
  origin.setY(yBounds->at(0));
  origin.setZ(zBounds->at(0));
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

Point3D<float32> RectGridGeom::getPlaneCoordsf(usize idx[3]) const
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

Point3D<float32> RectGridGeom::getPlaneCoordsf(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = (*xBnds)[x];
  coords[1] = (*yBnds)[y];
  coords[2] = (*zBnds)[z];
  return coords;
}

Point3D<float32> RectGridGeom::getPlaneCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = (*xBnds)[column];
  coords[1] = (*yBnds)[row];
  coords[2] = (*zBnds)[plane];
  return coords;
}

Point3D<float64> RectGridGeom::getPlaneCoords(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[idx[0]]);
  coords[1] = static_cast<float64>((*yBnds)[idx[1]]);
  coords[2] = static_cast<float64>((*zBnds)[idx[2]]);
  return coords;
}

Point3D<float64> RectGridGeom::getPlaneCoords(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[x]);
  coords[1] = static_cast<float64>((*yBnds)[y]);
  coords[2] = static_cast<float64>((*zBnds)[z]);
  return coords;
}

Point3D<float64> RectGridGeom::getPlaneCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = static_cast<float64>((*xBnds)[column]);
  coords[1] = static_cast<float64>((*yBnds)[row]);
  coords[2] = static_cast<float64>((*zBnds)[plane]);
  return coords;
}

Point3D<float32> RectGridGeom::getCoordsf(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[idx[0]] + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5f * ((*yBnds)[idx[1]] + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5f * ((*zBnds)[idx[2]] + (*zBnds)[idx[2] + 1]);
  return coords;
}

Point3D<float32> RectGridGeom::getCoordsf(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[x] + (*xBnds)[x + 1]);
  coords[1] = 0.5f * ((*yBnds)[y] + (*yBnds)[y + 1]);
  coords[2] = 0.5f * ((*zBnds)[z] + (*zBnds)[z + 1]);
  return coords;
}

Point3D<float32> RectGridGeom::getCoordsf(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float32> coords;
  coords[0] = 0.5f * ((*xBnds)[column] + (*xBnds)[column + 1]);
  coords[1] = 0.5f * ((*yBnds)[row] + (*yBnds)[row + 1]);
  coords[2] = 0.5f * ((*zBnds)[plane] + (*zBnds)[plane + 1]);
  return coords;
}

Point3D<float64> RectGridGeom::getCoords(usize idx[3]) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[idx[0]]) + (*xBnds)[idx[0] + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[idx[1]]) + (*yBnds)[idx[1] + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[idx[2]]) + (*zBnds)[idx[2] + 1]);
  return coords;
}

Point3D<float64> RectGridGeom::getCoords(usize x, usize y, usize z) const
{
  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[x]) + (*xBnds)[x + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[y]) + (*yBnds)[y + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[z]) + (*zBnds)[z + 1]);
  return coords;
}

Point3D<float64> RectGridGeom::getCoords(usize idx) const
{
  usize column = idx % m_Dimensions[0];
  usize row = (idx / m_Dimensions[0]) % m_Dimensions[1];
  usize plane = idx / (m_Dimensions[0] * m_Dimensions[1]);

  auto xBnds = getXBounds();
  auto yBnds = getYBounds();
  auto zBnds = getZBounds();

  Point3D<float64> coords;
  coords[0] = 0.5 * (static_cast<float64>((*xBnds)[column]) + (*xBnds)[column + 1]);
  coords[1] = 0.5 * (static_cast<float64>((*yBnds)[row]) + (*yBnds)[row + 1]);
  coords[2] = 0.5 * (static_cast<float64>((*zBnds)[plane]) + (*zBnds)[plane + 1]);
  return coords;
}

std::optional<usize> RectGridGeom::getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const
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

std::optional<usize> RectGridGeom::getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const
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

void RectGridGeom::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  IGridGeometry::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_xBoundsId == updatedId.first)
    {
      m_xBoundsId = updatedId.second;
    }

    if(m_yBoundsId == updatedId.first)
    {
      m_yBoundsId = updatedId.second;
    }

    if(m_zBoundsId == updatedId.first)
    {
      m_zBoundsId = updatedId.second;
    }
  }
}
