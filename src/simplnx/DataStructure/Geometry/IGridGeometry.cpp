#include "IGridGeometry.hpp"

#include "simplnx/Utilities/DataObjectUtilities.hpp"

namespace nx::core
{
IGridGeometry::IGridGeometry(DataStructure& dataStructure, std::string name)
: IGeometry(dataStructure, std::move(name))
{
}

IGridGeometry::IGridGeometry(DataStructure& dataStructure, std::string name, IdType importId)
: IGeometry(dataStructure, std::move(name), importId)
{
}

const std::optional<IGridGeometry::IdType>& IGridGeometry::getCellDataId() const
{
  return m_CellDataId;
}

AttributeMatrix* IGridGeometry::getCellData()
{
  if(!m_CellDataId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

const AttributeMatrix* IGridGeometry::getCellData() const
{
  if(!m_CellDataId.has_value())
  {
    return nullptr;
  }
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

AttributeMatrix& IGridGeometry::getCellDataRef()
{
  if(!m_CellDataId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AttributeMatrix& IGridGeometry::getCellDataRef()::{} threw runtime exception. The geometry with name '{}' does not have a cell attribute matrix assigned.\n  {}:{}", __func__,
                    getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
}

const AttributeMatrix& IGridGeometry::getCellDataRef() const
{
  if(!m_CellDataId.has_value())
  {
    throw std::runtime_error(
        fmt::format("AttributeMatrix& IGridGeometry::getCellDataRef()::{} threw runtime exception. The geometry with name '{}' does not have a cell attribute matrix assigned.\n  {}:{}", __func__,
                    getName(), __FILE__, __LINE__));
  }
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
}

DataPath IGridGeometry::getCellDataPath() const
{
  return getCellDataRef().getDataPaths().at(0);
}

void IGridGeometry::setCellData(const AttributeMatrix& attributeMatrix)
{
  m_CellDataId = attributeMatrix.getId();
}

void IGridGeometry::setCellData(OptionalId id)
{
  m_CellDataId = id;
}

void IGridGeometry::checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap)
{
  IGeometry::checkUpdatedIdsImpl(updatedIdsMap);

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

Result<> IGridGeometry::validate() const
{
  // Validate the next lower dimension geometry
  Result<> result;

  usize numTuples = getNumberOfCells();
  const AttributeMatrix* amPtr = getCellData();
  if(nullptr == amPtr)
  {
    return result;
  }
  usize amNumTuples = amPtr->getNumTuples();

  if(numTuples != amNumTuples)
  {
    return MergeResults(
        result, MakeErrorResult(-4501, fmt::format("Grid Geometry '{}' has {} cells but the cell Attribute Matrix '{}' has {} total tuples.", getName(), numTuples, amPtr->getName(), amNumTuples)));
  }
  return result;
}

} // namespace nx::core
