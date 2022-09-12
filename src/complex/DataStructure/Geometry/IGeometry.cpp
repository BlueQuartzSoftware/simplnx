#include "IGeometry.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

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

H5::ErrorType IGeometry::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = BaseGroup::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_ElementSizesId = ReadH5DataId(groupReader, H5Constants::k_ElementSizesTag);

  return error;
}

H5::ErrorType IGeometry::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = BaseGroup::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_ElementSizesId, H5Constants::k_ElementSizesTag);
  if(error < 0)
  {
    return error;
  }

  return error;
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

std::optional<IGeometry::IdType> IGeometry::ReadH5DataId(const H5::ObjectReader& objectReader, const std::string& attributeName)
{
  if(!objectReader.isValid())
  {
    return {};
  }

  auto attribute = objectReader.getAttribute(attributeName);
  auto id = attribute.readAsValue<IdType>();
  if(id == 0)
  {
    return {};
  }
  return id;
}

H5::ErrorType IGeometry::WriteH5DataId(H5::ObjectWriter& objectWriter, const std::optional<IdType>& dataId, const std::string& attributeName)
{
  if(!objectWriter.isValid())
  {
    return -1;
  }

  auto attribute = objectWriter.createAttribute(attributeName);
  if(dataId.has_value())
  {
    return attribute.writeValue<IdType>(dataId.value());
  }
  return attribute.writeValue<IdType>(0);
}
} // namespace complex
