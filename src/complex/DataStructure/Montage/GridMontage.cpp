#include "GridMontage.h"

using namespace SIMPL;

// Constructors/Destructors
//

GridMontage::GridMontage(const GridMontage& other)
: AbstractMontage(other)
{
}
GridMontage::GridMontage(GridMontage&& other) noexcept
: AbstractMontage(std::move(other))
{
}

GridMontage::~GridMontage() = default;
