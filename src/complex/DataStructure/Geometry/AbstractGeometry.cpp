#include "AbstractGeometry.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectWriter.hpp"

using namespace complex;

std::string AbstractGeometry::LengthUnitToString(LengthUnit unit)
{
  switch(unit)
  {
  case LengthUnit::Yoctometer:
    return "Yoctometer";
  case LengthUnit::Zeptometer:
    return "Zeptometer";
  case LengthUnit::Attometer:
    return "Attometer";
  case LengthUnit::Femtometer:
    return "Femtometer";
  case LengthUnit::Picometer:
    return "Picometer";
  case LengthUnit::Nanometer:
    return "Nanometer";
  case LengthUnit::Micrometer:
    return "Micrometer";
  case LengthUnit::Millimeter:
    return "Millimeter";
  case LengthUnit::Centimeter:
    return "Centimeter";
  case LengthUnit::Decimeter:
    return "Decimeter";
  case LengthUnit::Meter:
    return "Meter";
  case LengthUnit::Decameter:
    return "Decameter";
  case LengthUnit::Hectometer:
    return "Hectometer";
  case LengthUnit::Kilometer:
    return "Kilometer";
  case LengthUnit::Megameter:
    return "Megameter";
  case LengthUnit::Gigameter:
    return "Gigameter";
  case LengthUnit::Terameter:
    return "Terameter";
  case LengthUnit::Petameter:
    return "Petameter";
  case LengthUnit::Exameter:
    return "Exameter";
  case LengthUnit::Zettameter:
    return "Zettameter";
  case LengthUnit::Yottameter:
    return "Yottameter";
  case LengthUnit::Angstrom:
    return "Angstrom";
  case LengthUnit::Mil:
    return "Mil";
  case LengthUnit::Inch:
    return "Inch";
  case LengthUnit::Foot:
    return "Foot";
  case LengthUnit::Mile:
    return "Mile";
  case LengthUnit::Fathom:
    return "Fathom";
  case LengthUnit::Unspecified:
    return "Unspecified";
  case LengthUnit::Unknown:
    return "Unknown";
  }
  return "Unknown";
}

AbstractGeometry::AbstractGeometry(DataStructure& ds, std::string name)
: BaseGroup(ds, std::move(name))
{
}

AbstractGeometry::AbstractGeometry(DataStructure& ds, std::string name, IdType importId)
: BaseGroup(ds, std::move(name), importId)
{
}

AbstractGeometry::AbstractGeometry(const AbstractGeometry& other)
: BaseGroup(other)
, m_LinkedGeometryData(other.m_LinkedGeometryData)
{
}

AbstractGeometry::AbstractGeometry(AbstractGeometry&& other) noexcept
: BaseGroup(std::move(other))
, m_LinkedGeometryData(std::move(other.m_LinkedGeometryData))
{
}

AbstractGeometry::~AbstractGeometry() = default;
DataObject::DataObjectType AbstractGeometry::getDataObjectType() const
{
  return DataObjectType::AbstractGeometry;
}

bool AbstractGeometry::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}

AbstractGeometry::LengthUnit AbstractGeometry::getUnits() const
{
  return m_Units;
}

void AbstractGeometry::setUnits(LengthUnit units)
{
  m_Units = units;
}

bool AbstractGeometry::getTimeSeriesEnabled() const
{
  return m_IsTimeSeriesEnabled;
}

void AbstractGeometry::setTimeSeriesEnabled(bool value)
{
  m_IsTimeSeriesEnabled = value;
}

float32 AbstractGeometry::getTimeValue() const
{
  return m_TimeValue;
}

void AbstractGeometry::setTimeValue(float32 value)
{
  m_TimeValue = value;
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

std::string AbstractGeometry::getInfoString(complex::InfoStringFormat format) const
{
  if(format == InfoStringFormat::HtmlFormat)
  {
    return getTooltipGenerator().generateHTML();
  }

  return "";
}

AbstractGeometry::SharedEdgeList* AbstractGeometry::createSharedEdgeList(usize numEdges)
{
  auto dataStore = std::make_unique<DataStore<MeshIndexType>>(std::vector<usize>{numEdges}, std::vector<usize>{2});
  SharedEdgeList* edges = DataArray<MeshIndexType>::Create(*getDataStructure(), "Shared Edge List", std::move(dataStore), getId());
  edges->getDataStore()->fill(0.0);
  return edges;
}

std::optional<DataObject::IdType> AbstractGeometry::ReadH5DataId(const H5::ObjectReader& objectReader, const std::string& attributeName)
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

H5::ErrorType AbstractGeometry::WriteH5DataId(H5::ObjectWriter& objectWriter, const std::optional<DataObject::IdType>& dataId, const std::string& attributeName)
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

const LinkedGeometryData& AbstractGeometry::getLinkedGeometryData() const
{
  return m_LinkedGeometryData;
}
LinkedGeometryData& AbstractGeometry::getLinkedGeometryData()
{
  return m_LinkedGeometryData;
}
