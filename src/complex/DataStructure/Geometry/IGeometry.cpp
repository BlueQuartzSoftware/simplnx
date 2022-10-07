#include "IGeometry.hpp"

namespace complex
{
IGeometry::IGeometry(DataStructure& ds, std::string name)
: BaseGroup(ds, std::move(name))
{
}

IGeometry::IGeometry(DataStructure& ds, std::string name, IdType importId)
: BaseGroup(ds, std::move(name), importId)
{
}

const Float32Array* IGeometry::getElementSizes() const
{
  return getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
}

DataObject::OptionalId IGeometry::getElementSizesId() const
{
  return m_ElementSizesId;
}

void IGeometry::setElementSizesId(const OptionalId& sizesId)
{
  m_ElementSizesId = sizesId;
}

void IGeometry::deleteElementSizes()
{
  getDataStructureRef().removeData(m_ElementSizesId);
  m_ElementSizesId.reset();
}

uint32 IGeometry::getUnitDimensionality() const
{
  return m_UnitDimensionality;
}

void IGeometry::setUnitDimensionality(uint32 value)
{
  m_UnitDimensionality = value;
}

uint32 IGeometry::getSpatialDimensionality() const
{
  return m_SpacialDimensionality;
}

void IGeometry::setSpatialDimensionality(uint32 value)
{
  m_SpacialDimensionality = value;
}

std::set<std::string> IGeometry::StringListFromGeometryType(const std::set<Type>& geomTypes)
{
  static const std::map<Type, std::string> k_TypeToStringMap = {{Type::Image, "ImageGeom"},   {Type::RectGrid, "RectGrid"}, {Type::Vertex, "Vertex"},           {Type::Edge, "Edge"},
                                                                {Type::Triangle, "Triangle"}, {Type::Quad, "Quad"},         {Type::Tetrahedral, "Tetrahedral"}, {Type::Hexahedral, "Hexahedral"}};

  std::set<std::string> stringValues;
  for(auto geomType : geomTypes)
  {
    stringValues.insert(k_TypeToStringMap.at(geomType));
  }
  return stringValues;
}

const std::set<IGeometry::Type>& IGeometry::GetAllGeomTypes()
{
  static const std::set<Type> types = {Type::Image, Type::RectGrid, Type::Vertex, Type::Edge, Type::Triangle, Type::Quad, Type::Tetrahedral, Type::Hexahedral};
  return types;
}

const LinkedGeometryData& IGeometry::getLinkedGeometryData() const
{
  return m_LinkedGeometryData;
}

LinkedGeometryData& IGeometry::getLinkedGeometryData()
{
  return m_LinkedGeometryData;
}

IGeometry::LengthUnit IGeometry::getUnits() const
{
  return m_Units;
}

void IGeometry::setUnits(LengthUnit units)
{
  m_Units = units;
}

std::string IGeometry::LengthUnitToString(LengthUnit unit)
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

void IGeometry::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  BaseGroup::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_ElementSizesId == updatedId.first)
    {
      m_ElementSizesId = updatedId.second;
    }
  }
}
} // namespace complex
