#include "AbstractGridGeometry.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
AbstractGridGeometry::AbstractGridGeometry(DataStructure& dataStructure, std::string name)
: AbstractGeometry(dataStructure, std::move(name))
{
}

AbstractGridGeometry::AbstractGridGeometry(DataStructure& dataStructure, std::string name, IdType importId)
: AbstractGeometry(dataStructure, std::move(name), importId)
{
}

const std::optional<AbstractGridGeometry::IdType>& AbstractGridGeometry::getCellDataId() const
{
  return m_CellDataId;
}

AttributeMatrix* AbstractGridGeometry::getCellData()
{
  if(!m_CellDataId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

const AttributeMatrix* AbstractGridGeometry::getCellData() const
{
  if(!m_CellDataId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

AttributeMatrix& AbstractGridGeometry::getCellDataRef()
{
  if(!m_CellDataId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AttributeMatrix& AbstractGridGeometry::getCellDataRef()::{} threw runtime exception. The geometry with name '{}' does not have a cell attribute matrix assigned.\n  {}:{}",
                    __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
}

const AttributeMatrix& AbstractGridGeometry::getCellDataRef() const
{
  if(!m_CellDataId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AttributeMatrix& AbstractGridGeometry::getCellDataRef()::{} threw runtime exception. The geometry with name '{}' does not have a cell attribute matrix assigned.\n  {}:{}",
                    __func__, getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
}

DataPath AbstractGridGeometry::getCellDataPath() const
{
  return getCellDataRef().getDataPaths().at(0);
}

void AbstractGridGeometry::setCellData(const AttributeMatrix& attributeMatrix)
{
  m_CellDataId = attributeMatrix.getId();
}

void AbstractGridGeometry::setCellData(OptionalId id)
{
  m_CellDataId = id;
}

void AbstractGridGeometry::checkUpdatedIdsImpl(const std::unordered_map<AbstractDataObject::IdType, AbstractDataObject::IdType>& updatedIdsMap)
{
  AbstractGeometry::checkUpdatedIdsImpl(updatedIdsMap);

  std::vector<bool> visited(1, false);

  for(const auto& updatedId : updatedIdsMap)
  {
    m_CellDataId = nx::core::VisitDataStructureId(m_CellDataId, updatedId, visited, 0);
    if(visited[0])
    {
      break;
    }
  }
}

Result<> AbstractGridGeometry::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result;

  usize numTuples = getNumberOfCells();
  const AttributeMatrix* amPtr = getCellData();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumberOfTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Grid Geometry '{}' has {} cells but the cell Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}

} // namespace nx::core
