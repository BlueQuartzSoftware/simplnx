#include "GridMontage.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataStructure.hpp"

using namespace complex;

GridMontage::GridMontage(DataStructure* ds, const std::string& name)
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

DataObject* GridMontage::shallowCopy()
{
  return new GridMontage(*this);
}

DataObject* GridMontage::deepCopy()
{
  throw std::runtime_error("");
}

size_t GridMontage::getRowCount() const
{
  return m_RowCount;
}

size_t GridMontage::getColumnCount() const
{
  return m_ColumnCount;
}

size_t GridMontage::getDepth() const
{
  return m_DepthCount;
}

void GridMontage::resizeTileDims(size_t row, size_t col, size_t depth)
{
  const CollectionType& oldCollection = getCollection();
  DimensionsType newDims = {row, col, depth};

  // Update collection
  CollectionType newCollection(row * col * depth);
  for(size_t x = 0; x < m_ColumnCount && col; x++)
  {
    for(size_t y = 0; y < m_RowCount && row; y++)
    {
      for(size_t z = 0; z < m_DepthCount && depth; z++)
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

GridTileIndex GridMontage::getTileIndex(size_t col, size_t row, size_t depth) const
{
  return getTileIndex({col, row, depth});
}

GridTileIndex GridMontage::getTileIndex(const SizeVec3& pos) const
{
  return GridTileIndex(this, pos);
}

std::optional<SizeVec3> GridMontage::getTilePosOfGeometry(const AbstractGeometry* geom) const
{
  for(size_t i = 0; i < getTileCount(); i++)
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
  size_t index = getOffsetFromTilePos(position);
  return getCollection()[index];
}

const AbstractGeometry* GridMontage::getGeometry(const SizeVec3& position) const
{
  size_t index = getOffsetFromTilePos(position);
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
  size_t index = getOffsetFromTilePos(position);
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

H5::ErrorType GridMontage::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::runtime_error("");
}

H5::ErrorType GridMontage::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::runtime_error("");
}

SizeVec3 GridMontage::getTilePosFromOffset(size_t offset) const
{
  const size_t numRows = getRowCount();
  const size_t numCols = getColumnCount();
  const size_t numDeep = getDepth();

  const size_t depth = offset / (numRows * numCols);
  const size_t xyOffset = offset % depth;
  const size_t row = xyOffset / numCols;
  const size_t col = xyOffset % numRows;
  return {col, row, depth};
}

size_t GridMontage::getOffsetFromTileId(const TileIdType& tileId) const
{
  return getOffsetFromTilePos(tileId.getTilePos());
}

size_t GridMontage::getOffsetFromTilePos(const SizeVec3& tilePos) const
{
  return getOffsetFromTilePos(tilePos, {getDimensions()});
}

size_t GridMontage::getOffsetFromTilePos(const SizeVec3& tilePos, const DimensionsType& dims)
{
  const size_t numCols = dims[0];
  const size_t numRows = dims[1];
  const size_t numDeep = dims[2];

  return tilePos[0] + tilePos[1] * numCols + tilePos[2] * numCols * numRows;
}
