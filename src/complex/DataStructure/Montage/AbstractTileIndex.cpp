#include "AbstractTileIndex.h"

using namespace SIMPL;

// Constructors/Destructors
//

AbstractTileIndex::AbstractTileIndex()
: m_Montage(nullptr)
{
}

AbstractTileIndex::AbstractTileIndex(AbstractMontage* montage)
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
