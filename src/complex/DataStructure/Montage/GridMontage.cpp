#include "GridMontage.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

#include <stdexcept>

using namespace complex;

GridMontage::GridMontage(DataStructure& ds, const std::string& name)
: AbstractMontage(ds, name)
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

GridMontage* GridMontage::Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<GridMontage>(new GridMontage(ds, name));
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
  throw std::runtime_error("");
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
  for(usize x = 0; x < m_ColumnCount && col; x++)
  {
    for(usize y = 0; y < m_RowCount && row; y++)
    {
      for(usize z = 0; z < m_DepthCount && depth; z++)
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

std::optional<SizeVec3> GridMontage::getTilePosOfGeometry(const AbstractGeometry* geom) const
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

std::shared_ptr<AbstractTileIndex> GridMontage::getTileIndex(AbstractGeometry* geom) const
{
  auto tilePos = getTilePosOfGeometry(geom);
  if(!tilePos)
  {
    return nullptr;
  }
  return std::shared_ptr<AbstractTileIndex>(new GridTileIndex(this, tilePos.value()));
}

AbstractGeometry* GridMontage::getGeometry(const AbstractTileIndex* index)
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return nullptr;
  }
  return getGeometry(pos->getTilePos());
}

const AbstractGeometry* GridMontage::getGeometry(const AbstractTileIndex* index) const
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return nullptr;
  }
  return getGeometry(pos->getTilePos());
}

AbstractGeometry* GridMontage::getGeometry(const SizeVec3& position)
{
  usize index = getOffsetFromTilePos(position);
  return getCollection()[index];
}

const AbstractGeometry* GridMontage::getGeometry(const SizeVec3& position) const
{
  usize index = getOffsetFromTilePos(position);
  return getCollection()[index];
}

void GridMontage::setGeometry(const AbstractTileIndex* index, AbstractGeometry* geom)
{
  auto pos = dynamic_cast<const GridTileIndex*>(index);
  if(!pos)
  {
    return;
  }
  setGeometry(pos->getTilePos(), geom);
}

void GridMontage::setGeometry(const SizeVec3& position, AbstractGeometry* geom)
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
  const usize numDeep = getDepth();

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

H5::ErrorType GridMontage::readHdf5(const H5::GroupReader& groupReader)
{
  return getDataMap().readH5Group(*getDataStructure(), groupReader, getId());
}

H5::ErrorType GridMontage::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const
{
  auto groupWriter = parentGroupWriter.createGroupWriter(getName());
  auto err = writeH5ObjectAttributes(groupWriter);
  if(err < 0)
  {
    return err;
  }

  return getDataMap().writeH5Group(dataStructureWriter, groupWriter);
}
