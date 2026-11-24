#include "IGeometry.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

#include <array>

namespace
{
using LengthUnit = nx::core::IGeometry::LengthUnit;
// Saved pipelines store the index in this ordered list.
constexpr std::array k_LengthUnitChoices = {LengthUnit::Yoctometer, LengthUnit::Zeptometer, LengthUnit::Attometer,  LengthUnit::Femtometer,  LengthUnit::Picometer, LengthUnit::Nanometer,
                                            LengthUnit::Micrometer, LengthUnit::Millimeter, LengthUnit::Centimeter, LengthUnit::Decimeter,   LengthUnit::Meter,     LengthUnit::Decameter,
                                            LengthUnit::Hectometer, LengthUnit::Kilometer,  LengthUnit::Megameter,  LengthUnit::Gigameter,   LengthUnit::Terameter, LengthUnit::Petameter,
                                            LengthUnit::Exameter,   LengthUnit::Zettameter, LengthUnit::Yottameter, LengthUnit::Angstrom,    LengthUnit::Mil,       LengthUnit::Inch,
                                            LengthUnit::Foot,       LengthUnit::Mile,       LengthUnit::Fathom,     LengthUnit::Unspecified, LengthUnit::Unknown};
} // namespace

namespace nx::core
{
IGeometry::IGeometry(DataStructure& dataStructure, std::string name)
: BaseGroup(dataStructure, std::move(name))
{
}

IGeometry::IGeometry(DataStructure& dataStructure, std::string name, IdType importId)
: BaseGroup(dataStructure, std::move(name), importId)
{
}

const Float32Array* IGeometry::getElementSizes() const
{
  const DataStructure* dataStructurePtr = getDataStructure();
  if(nullptr == dataStructurePtr)
  {
    return nullptr;
  }
  return dataStructurePtr->getDataAs<Float32Array>(m_ElementSizesId);
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

std::set<std::string> IGeometry::StringListFromGeometryType(const std::set<IGeometry::Type>& geomTypes)
{
  std::set<std::string> stringValues;
  for(auto geomType : geomTypes)
  {
    stringValues.insert(IGeometry::k_GeomTypeStrings[to_underlying(geomType)]);
  }
  return stringValues;
}

const std::set<IGeometry::Type>& IGeometry::GetAllGeomTypes()
{
  static const std::set<Type> types = {Type::Image, Type::RectGrid, Type::Vertex, Type::Edge, Type::Triangle, Type::Quad, Type::Tetrahedral, Type::Hexahedral};
  return types;
}

const std::vector<std::string>& IGeometry::GetAllLengthUnitStrings()
{
  static const std::vector<std::string> lengthUnitStrs = [] {
    std::vector<std::string> names;
    names.reserve(k_LengthUnitChoices.size());
    for(const auto unit : k_LengthUnitChoices)
    {
      names.push_back(LengthUnitToString(unit));
    }
    return names;
  }();
  return lengthUnitStrs;
}

IGeometry::LengthUnit IGeometry::GetLengthUnitFromChoiceIndex(uint64 index)
{
  if(index >= k_LengthUnitChoices.size())
  {
    return LengthUnit::Unknown;
  }
  return k_LengthUnitChoices[index];
}

IGeometry::LengthUnit IGeometry::getUnits() const
{
  return m_Units;
}

void IGeometry::setUnits(LengthUnit units)
{
  m_Units = units;
}

std::string IGeometry::GeomTypeToString(IGeometry::Type geomType)
{
  return IGeometry::k_GeomTypeStrings[to_underlying(geomType)];
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

std::optional<IGeometry::IdType> IGeometry::deepCopyOwnedChild(const DataPath& copyPath, DataObject* child)
{
  if(child == nullptr)
  {
    return {};
  }

  const DataPath copiedDataPath = copyPath.createChildPath(child->getName());
  // The data map copy reproduces children this geometry parents; anything else is copied here.
  if(!isParentOf(child))
  {
    const auto dataObjCopy = child->deepCopy(copiedDataPath);
  }
  return getDataStructureRef().getId(copiedDataPath);
}

void IGeometry::copyMembersInto(IGeometry& copy, const DataPath& copyPath)
{
  copy.m_ElementSizesId = adoptCopiedChild<Float32Array>(copyPath, k_VoxelSizes);
}

void IGeometry::checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap)
{
  BaseGroup::checkUpdatedIdsImpl(updatedIdsMap);

  std::vector<bool> visited(1, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_ElementSizesId = nx::core::VisitDataStructureId(m_ElementSizesId, updatedId, visited, 0);
  }
}
} // namespace nx::core
