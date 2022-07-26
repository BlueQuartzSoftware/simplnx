#include "IGridGeometry.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex
{
IGridGeometry::IGridGeometry(DataStructure& ds, std::string name)
: IGeometry(ds, std::move(name))
{
}

IGridGeometry::IGridGeometry(DataStructure& ds, std::string name, IdType importId)
: IGeometry(ds, std::move(name), importId)
{
}

const std::optional<IGridGeometry::IdType>& IGridGeometry::getCellDataId() const
{
  return m_CellDataId;
}

AttributeMatrix* IGridGeometry::getCellData()
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

const AttributeMatrix* IGridGeometry::getCellData() const
{
  return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
}

AttributeMatrix& IGridGeometry::getCellDataRef()
{
  return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
}

const AttributeMatrix& IGridGeometry::getCellDataRef() const
{
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

H5::ErrorType IGridGeometry::readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight)
{
  H5::ErrorType error = IGeometry::readHdf5(dataStructureReader, groupReader, preflight);
  if(error < 0)
  {
    return error;
  }

  m_CellDataId = ReadH5DataId(groupReader, H5Constants::k_CellDataTag);

  return error;
}

H5::ErrorType IGridGeometry::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  H5::ErrorType error = IGeometry::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
  if(error < 0)
  {
    return error;
  }

  H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
  error = WriteH5DataId(groupWriter, m_CellDataId, H5Constants::k_CellDataTag);
  if(error < 0)
  {
    return error;
  }

  return error;
}

void IGridGeometry::checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds)
{
  IGeometry::checkUpdatedIdsImpl(updatedIds);

  for(const auto& updatedId : updatedIds)
  {
    if(m_CellDataId == updatedId.first)
    {
      m_CellDataId = updatedId.second;
    }
  }
}
} // namespace complex
