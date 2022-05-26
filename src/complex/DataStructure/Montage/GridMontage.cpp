#include "GridMontage.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

#include <stdexcept>

using namespace complex;

GridMontage::GridMontage(DataStructure& ds, std::string name)
: AbstractMontage(ds, std::move(name))
{
}

GridMontage::GridMontage(DataStructure& ds, std::string name, IdType importId)
: AbstractMontage(ds, std::move(name), importId)
{
}

GridMontage::GridMontage(const GridMontage& other)
: AbstractMontage(other)
{
}

GridMontage::GridMontage(GridMontage&& other) noexcept
: AbstractMontage(std::move(other))
{
}

GridMontage::~GridMontage() = default;

GridMontage* GridMontage::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<GridMontage>(new GridMontage(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

GridMontage* GridMontage::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<GridMontage>(new GridMontage(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

std::string GridMontage::getTypeName() const
{
  return "GridMontage";
}

DataObject* GridMontage::shallowCopy()
{
  return new GridMontage(*this);
}

DataObject* GridMontage::deepCopy()
{
  return new GridMontage(*this);
}

usize GridMontage::getRowCount() const
{
  return m_RowCount;
}

usize GridMontage::getColumnCount() const
{
  return m_ColumnCount;
}

usize GridMontage::getDepth() const
{
  return m_DepthCount;
}

SizeVec3 GridMontage::getGridSize() const
{
  return {m_ColumnCount, m_RowCount, m_DepthCount};
}

void GridMontage::resizeTileDims(usize row, usize col, usize depth)
{
  const CollectionType& oldCollection = getCollection();
  DimensionsType newDims = {row, col, depth};

  // Update collection
  CollectionType newCollection(row * col * depth);
  for(usize x = 0; x < m_ColumnCount && col != 0; x++)
  {
    for(usize y = 0; y < m_RowCount && row != 0; y++)
    {
      for(usize z = 0; z < m_DepthCount && depth != 0; z++)
      {
        const SizeVec3 pos = {x, y, z};
        auto oldOffset = getOffsetFromTilePos(pos);
        auto newOffset = getOffsetFromTilePos(pos, newDims);

        if(oldOffset >= oldCollection.size())
        {
          continue;
        }
        newCollection[newOffset] = oldCollection[oldOffset];
      }
    }
  }

  m_RowCount = row;
  m_ColumnCount = col;
  m_DepthCount = depth;
  getCollection() = newCollection;
}

GridTileIndex GridMontage::getTileIndex(usize col, usize row, usize depth) const
{
  if(col >= m_ColumnCount)
  {
    return GridTileIndex();
  }
  if(row >= m_RowCount)
  {
    return GridTileIndex();
  }
  if(depth >= m_DepthCount)
  {
    return GridTileIndex();
  }
  return GridTileIndex(this, {col, row, depth});
}

GridTileIndex GridMontage::getTileIndex(const SizeVec3& pos) const
{
  return getTileIndex(pos[0], pos[1], pos[2]);
}

std::optional<SizeVec3> GridMontage::getTilePosOfGeometry(const IGeometry* geom) const
{
  for(usize i = 0; i < getTileCount(); i++)
  {
    if(geom == getCollection()[i])
    {
      return getTilePosFromOffset(i);
    }
  }
  return {};
}

std::shared_ptr<AbstractTileIndex> GridMontage::getTileIndex(IGeometry* geom) const
{
  auto tilePos = getTilePosOfGeometry(geom);
  if(!tilePos)
  {
    return nullptr;
  }
  return std::shared_ptr<AbstractTileIndex>(new GridTileIndex(this, tilePos.value()));
}

IGeometry* GridMontage::getGeometry(const AbstractTileIndex* index)
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return nullptr;
  }
  return getGeometry(pos->getTilePos());
}

const IGeometry* GridMontage::getGeometry(const AbstractTileIndex* index) const
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return nullptr;
  }
  return getGeometry(pos->getTilePos());
}

IGeometry* GridMontage::getGeometry(const SizeVec3& position)
{
  usize index = getOffsetFromTilePos(position);
  return getCollection()[index];
}

const IGeometry* GridMontage::getGeometry(const SizeVec3& position) const
{
  usize index = getOffsetFromTilePos(position);
  return getCollection()[index];
}

void GridMontage::setGeometry(const AbstractTileIndex* index, IGeometry* geom)
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return;
  }
  setGeometry(pos->getTilePos(), geom);
}

void GridMontage::setGeometry(const SizeVec3& position, IGeometry* geom)
{
  CollectionType& collection = getCollection();
  usize index = getOffsetFromTilePos(position);
  collection[index] = geom;
  getDataStructure()->setAdditionalParent(geom->getId(), getId());
}

TooltipGenerator GridMontage::getTooltipGenerator() const
{
  throw std::runtime_error("");
}

GridMontage::DimensionsType GridMontage::getDimensions() const
{
  return {m_ColumnCount, m_RowCount, m_DepthCount};
}

GridMontage::BoundsType GridMontage::getBounds() const
{
  throw std::runtime_error("");
}

SizeVec3 GridMontage::getTilePosFromOffset(usize offset) const
{
  const usize numRows = getRowCount();
  const usize numCols = getColumnCount();
  // const usize numDeep = getDepth();

  const usize depth = offset / (numRows * numCols);
  const usize xyOffset = offset % depth;
  const usize row = xyOffset / numCols;
  const usize col = xyOffset % numRows;
  return {col, row, depth};
}

usize GridMontage::getOffsetFromTileId(const TileIdType& tileId) const
{
  return getOffsetFromTilePos(tileId.getTilePos());
}

usize GridMontage::getOffsetFromTilePos(const SizeVec3& tilePos) const
{
  return getOffsetFromTilePos(tilePos, {getDimensions()});
}

usize GridMontage::getOffsetFromTilePos(const SizeVec3& tilePos, const DimensionsType& dims)
{
  const usize numCols = dims[0];
  const usize numRows = dims[1];
  const usize numDeep = dims[2];

  return tilePos[0] + tilePos[1] * numCols + tilePos[2] * numCols * numRows;
}

H5::ErrorType GridMontage::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  auto rowCountAttribute = groupReader.getAttribute(H5Constants::k_RowCountTag);
  m_RowCount = rowCountAttribute.readAsValue<uint64_t>();

  auto colCountAttribute = groupReader.getAttribute(H5Constants::k_ColCountTag);
  m_ColumnCount = colCountAttribute.readAsValue<uint64_t>();

  auto depthCountAttribute = groupReader.getAttribute(H5Constants::k_DepthCountTag);
  m_DepthCount = depthCountAttribute.readAsValue<uint64_t>();

  return BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
}

H5::ErrorType GridMontage::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto errorCode = writeH5ObjectAttributes(dataStructureWriter, groupWriter, importable);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto rowCountAttribute = groupWriter.createAttribute(H5Constants::k_RowCountTag);
  errorCode = rowCountAttribute.writeValue<uint64_t>(m_RowCount);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto colCountAttribute = groupWriter.createAttribute(H5Constants::k_ColCountTag);
  errorCode = colCountAttribute.writeValue<uint64_t>(m_ColumnCount);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto depthCountAttribute = groupWriter.createAttribute(H5Constants::k_DepthCountTag);
  errorCode = depthCountAttribute.writeValue<uint64_t>(m_DepthCount);
  if(errorCode < 0)
  {
    return errorCode;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
