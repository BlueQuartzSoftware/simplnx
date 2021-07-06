#include "AbstractMontage.h"

using namespace SIMPL;

AbstractMontage::AbstractMontage(DataStructure* ds, const std::string& name)
: BaseGroup(ds, name)
{
}

AbstractMontage::AbstractMontage(const AbstractMontage& other)
: BaseGroup(other)
{
}

AbstractMontage::AbstractMontage(AbstractMontage&& other) noexcept
: BaseGroup(std::move(other))
{
}

AbstractMontage::~AbstractMontage() = default;
