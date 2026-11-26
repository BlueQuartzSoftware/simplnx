#include "GridTileIndex.hpp"

#include "simplnx/DataStructure/Montage/GridMontage.hpp"

#include <stdexcept>
#include <utility>

using namespace nx::core;

GridTileIndex::GridTileIndex()

{
}

GridTileIndex::GridTileIndex(const GridMontage* montage, SizeVec3 pos)
: AbstractTileIndex(montage)
, m_Pos(std::move(pos))
{
}

GridTileIndex::GridTileIndex(const GridTileIndex& other)
: AbstractTileIndex(other)
, m_Pos(other.m_Pos)
{
}

GridTileIndex::GridTileIndex(GridTileIndex&& other) noexcept
: AbstractTileIndex(std::move(other))
, m_Pos(std::move(other.m_Pos))
{
}

GridTileIndex::~GridTileIndex() = default;

usize GridTileIndex::getRow() const
{
  return m_Pos[1];
}

usize GridTileIndex::getCol() const
{
  return m_Pos[0];
}

usize GridTileIndex::getDepth() const
{
  return m_Pos[2];
}

SizeVec3 GridTileIndex::getTilePos() const
{
  return m_Pos;
}

const IGeometry* GridTileIndex::getGeometry() const
{
  const auto *montage = dynamic_cast<const GridMontage*>(getMontage());
  if(montage == nullptr)
  {
    return nullptr;
  }
  return montage->getGeometry(this);
}

TooltipGenerator GridTileIndex::getToolTipGenerator() const
{
  throw std::runtime_error("GridTileIndex::getToolTipGenerator() is not implemented");
}

bool GridTileIndex::isValid() const
{
  if(dynamic_cast<const GridMontage*>(getMontage()) == nullptr)
  {
    return false;
  }
  return AbstractTileIndex::isValid();
}
