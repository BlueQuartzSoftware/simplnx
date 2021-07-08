#include "AbstractGeometry2D.hpp"

using namespace complex;

AbstractGeometry2D::AbstractGeometry2D(DataStructure* ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

AbstractGeometry2D::AbstractGeometry2D(const AbstractGeometry2D& other)
: AbstractGeometry(other)
{
}

AbstractGeometry2D::AbstractGeometry2D(AbstractGeometry2D&& other) noexcept
: AbstractGeometry(std::move(other))
{
}

AbstractGeometry2D::~AbstractGeometry2D() = default;
