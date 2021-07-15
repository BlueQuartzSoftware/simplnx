#include "GridTileIndex.hpp"

#include "complex/DataStructure/Montage/GridMontage.hpp"

using namespace complex;

// Constructors/Destructors
//

GridTileIndex::GridTileIndex(GridMontage* montage, const SizeVec3& pos)
: AbstractTileIndex(montage)
, m_Pos(pos)
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
