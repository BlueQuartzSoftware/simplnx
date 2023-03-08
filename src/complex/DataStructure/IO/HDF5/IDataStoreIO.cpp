#include "IDataStoreIO.hpp"

#include "complex/DataStructure/IO/Generic/IOConstants.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include "complex/Common/Types.hpp"

#include <vector>

using namespace complex;

typename complex::IDataStore::ShapeType complex::HDF5::IDataStoreIO::ReadTupleShape(const complex::HDF5::DatasetReader& datasetReader)
{
  complex::HDF5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(IOConstants::k_TupleShapeTag);
  if(!tupleShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Tuple Shape from HDF5 at {}/{}", complex::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return tupleShapeAttribute.readAsVector<usize>();
}

typename complex::IDataStore::ShapeType complex::HDF5::IDataStoreIO::ReadComponentShape(const complex::HDF5::DatasetReader& datasetReader)
{
  complex::HDF5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(IOConstants::k_ComponentShapeTag);
  if(!componentShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Component Shape from HDF5 at {}/{}", complex::HDF5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return componentShapeAttribute.readAsVector<usize>();
}
