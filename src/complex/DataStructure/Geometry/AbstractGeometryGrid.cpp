#include "AbstractGeometryGrid.hpp"

using namespace complex;

AbstractGeometryGrid::AbstractGeometryGrid(DataStructure& ds, std::string name)
: AbstractGeometry(ds, std::move(name))
{
}

AbstractGeometryGrid::AbstractGeometryGrid(DataStructure& ds, std::string name, IdType importId)
: AbstractGeometry(ds, std::move(name), importId)
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
DataObject::DataObjectType AbstractGeometryGrid::getDataObjectType() const
{
  return DataObjectType::AbstractGeometryGrid;
}
