#include "AbstractGeometry.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
AbstractGeometry::AbstractGeometry(DataStructure& dataStructure, std::string name)
: BaseGroup(dataStructure, std::move(name))
{
}

AbstractGeometry::AbstractGeometry(DataStructure& dataStructure, std::string name, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
{
}

const Float32Array* AbstractGeometry::getElementSizes() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
}

AbstractDataObject::OptionalId AbstractGeometry::getElementSizesId() const
{
  return m_ElementSizesId;
}

void AbstractGeometry::setElementSizesId(const OptionalId& sizesId)
{
  m_ElementSizesId = sizesId;
}

void AbstractGeometry::deleteElementSizes()
{
  getDataStructureRef().removeData(m_ElementSizesId);
  m_ElementSizesId.reset();
}

uint32 AbstractGeometry::getUnitDimensionality() const
{
  return m_UnitDimensionality;
}

void AbstractGeometry::setUnitDimensionality(uint32 value)
{
  m_UnitDimensionality = value;
}

uint32 AbstractGeometry::getSpatialDimensionality() const
{
  return m_SpacialDimensionality;
}

void AbstractGeometry::setSpatialDimensionality(uint32 value)
{
  m_SpacialDimensionality = value;
}

std::set<std::string> AbstractGeometry::StringListFromGeometryType(const std::set<AbstractGeometry::Type>& geomTypes)
{
  std::set<std::string> stringValues;
  for(auto geomType : geomTypes)
  {
    stringValues.insert(AbstractGeometry::k_GeomTypeStrings[to_underlying(geomType)]);
  }
  return stringValues;
}

const std::set<AbstractGeometry::Type>& AbstractGeometry::GetAllGeomTypes()
{
  static const std::set<Type> types = {Type::Image, Type::RectGrid, Type::Vertex, Type::Edge, Type::Triangle, Type::Quad, Type::Tetrahedral, Type::Hexahedral};
  return types;
}

const std::vector<std::string>& AbstractGeometry::GetAllLengthUnitStrings()
{
  static const std::vector<std::string> lengthUnitStrs = {
      LengthUnitToString(LengthUnit::Yoctometer), LengthUnitToString(LengthUnit::Zeptometer), LengthUnitToString(LengthUnit::Attometer),  LengthUnitToString(LengthUnit::Femtometer),
      LengthUnitToString(LengthUnit::Picometer),  LengthUnitToString(LengthUnit::Nanometer),  LengthUnitToString(LengthUnit::Micrometer), LengthUnitToString(LengthUnit::Millimeter),
      LengthUnitToString(LengthUnit::Centimeter), LengthUnitToString(LengthUnit::Decimeter),  LengthUnitToString(LengthUnit::Meter),      LengthUnitToString(LengthUnit::Decameter),
      LengthUnitToString(LengthUnit::Hectometer), LengthUnitToString(LengthUnit::Kilometer),  LengthUnitToString(LengthUnit::Megameter),  LengthUnitToString(LengthUnit::Gigameter),
      LengthUnitToString(LengthUnit::Terameter),  LengthUnitToString(LengthUnit::Petameter),  LengthUnitToString(LengthUnit::Exameter),   LengthUnitToString(LengthUnit::Zettameter),
      LengthUnitToString(LengthUnit::Yottameter), LengthUnitToString(LengthUnit::Angstrom),   LengthUnitToString(LengthUnit::Mil),        LengthUnitToString(LengthUnit::Inch),
      LengthUnitToString(LengthUnit::Foot),       LengthUnitToString(LengthUnit::Mile),       LengthUnitToString(LengthUnit::Fathom),     LengthUnitToString(LengthUnit::Unspecified),
      LengthUnitToString(LengthUnit::Unknown)};
  return lengthUnitStrs;
}

AbstractGeometry::LengthUnit AbstractGeometry::getUnits() const
{
  return m_Units;
}

void AbstractGeometry::setUnits(LengthUnit units)
{
  m_Units = units;
}

std::string AbstractGeometry::GeomTypeToString(AbstractGeometry::Type geomType)
{
  return AbstractGeometry::k_GeomTypeStrings[to_underlying(geomType)];
}

std::string AbstractGeometry::LengthUnitToString(LengthUnit unit)
{
  switch(unit)
  {
  case LengthUnit::Yoctometer: {
    return "Yoctometer";
  }
  case LengthUnit::Zeptometer: {
    return "Zeptometer";
  }
  case LengthUnit::Attometer: {
    return "Attometer";
  }
  case LengthUnit::Femtometer: {
    return "Femtometer";
  }
  case LengthUnit::Picometer: {
    return "Picometer";
  }
  case LengthUnit::Nanometer: {
    return "Nanometer";
  }
  case LengthUnit::Micrometer: {
    return "Micrometer";
  }
  case LengthUnit::Millimeter: {
    return "Millimeter";
  }
  case LengthUnit::Centimeter: {
    return "Centimeter";
  }
  case LengthUnit::Decimeter: {
    return "Decimeter";
  }
  case LengthUnit::Meter: {
    return "Meter";
  }
  case LengthUnit::Decameter: {
    return "Decameter";
  }
  case LengthUnit::Hectometer: {
    return "Hectometer";
  }
  case LengthUnit::Kilometer: {
    return "Kilometer";
  }
  case LengthUnit::Megameter: {
    return "Megameter";
  }
  case LengthUnit::Gigameter: {
    return "Gigameter";
  }
  case LengthUnit::Terameter: {
    return "Terameter";
  }
  case LengthUnit::Petameter: {
    return "Petameter";
  }
  case LengthUnit::Exameter: {
    return "Exameter";
  }
  case LengthUnit::Zettameter: {
    return "Zettameter";
  }
  case LengthUnit::Yottameter: {
    return "Yottameter";
  }
  case LengthUnit::Angstrom: {
    return "Angstrom";
  }
  case LengthUnit::Mil: {
    return "Mil";
  }
  case LengthUnit::Inch: {
    return "Inch";
  }
  case LengthUnit::Foot: {
    return "Foot";
  }
  case LengthUnit::Mile: {
    return "Mile";
  }
  case LengthUnit::Fathom: {
    return "Fathom";
  }
  case LengthUnit::Unspecified: {
    return "Unspecified";
  }
  case LengthUnit::Unknown: {
    return "Unknown";
  }
  }
  return "Unknown";
}

void AbstractGeometry::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  BaseGroup::checkUpdatedIdsImpl(updatedIdsMap);

  std::vector<bool> visited(1, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_ElementSizesId = nx::core::VisitDataStructureId(m_ElementSizesId, updatedId, visited, 0);
  }
}
} // namespace nx::core
