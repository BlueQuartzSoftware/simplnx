#include "IDataStoreIO.hpp"

#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"

#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"

#include "simplnx/Common/Types.hpp"

#include <vector>

using namespace nx::core;

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadTupleShape(const nx::core::HDF5::DatasetReader& datasetReader)
{
  nx::core::HDF5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(IOConstants::k_TupleShapeTag);
  if(!tupleShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Tuple Shape from HDF5 at {}/{}", nx::core::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return tupleShapeAttribute.readAsVector<usize>();
}

typename nx::core::IDataStore::ShapeType nx::core::HDF5::IDataStoreIO::ReadComponentShape(const nx::core::HDF5::DatasetReader& datasetReader)
{
  nx::core::HDF5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(IOConstants::k_ComponentShapeTag);
  if(!componentShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Component Shape from HDF5 at {}/{}", nx::core::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return componentShapeAttribute.readAsVector<usize>();
}
