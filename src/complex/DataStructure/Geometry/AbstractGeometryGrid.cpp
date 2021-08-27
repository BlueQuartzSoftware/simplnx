#include "AbstractGeometryGrid.hpp"

using namespace complex;

AbstractGeometryGrid::AbstractGeometryGrid(DataStructure& ds, const std::string& name)
: AbstractGeometry(ds, name)
{
}

AbstractGeometryGrid::AbstractGeometryGrid(const AbstractGeometryGrid& other)
: AbstractGeometry(other)
{
}

AbstractGeometryGrid::AbstractGeometryGrid(AbstractGeometryGrid&& other) noexcept
: AbstractGeometry(std::move(other))
{
}

AbstractGeometryGrid::~AbstractGeometryGrid() = default;
