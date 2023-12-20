#include "AbstractTileIndex.hpp"

using namespace nx::core;

AbstractTileIndex::AbstractTileIndex()

{
}

AbstractTileIndex::AbstractTileIndex(const AbstractMontage* montage)
: m_Montage(montage)
{
}

AbstractTileIndex::AbstractTileIndex(const AbstractTileIndex& other)
: m_Montage(other.m_Montage)
{
}

AbstractTileIndex::AbstractTileIndex(AbstractTileIndex&& other) noexcept
: m_Montage(std::move(other.m_Montage))
{
}

AbstractTileIndex::~AbstractTileIndex() = default;

const AbstractMontage* AbstractTileIndex::getMontage() const
{
  return m_Montage;
}

bool AbstractTileIndex::isValid() const
{
  return getMontage() != nullptr;
}
