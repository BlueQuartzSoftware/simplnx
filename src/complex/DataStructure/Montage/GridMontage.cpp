#include "GridMontage.hpp"

using namespace complex;

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
