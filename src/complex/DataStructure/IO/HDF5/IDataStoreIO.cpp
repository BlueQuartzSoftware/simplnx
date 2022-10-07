#include "IDataStoreIO.hpp"

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <vector>

typename complex::IDataStore::ShapeType complex::HDF5::IDataStoreIO::ReadTupleShape(const H5::DatasetReader& datasetReader)
{
  H5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(complex::H5::k_TupleShapeTag);
  if(!tupleShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Tuple Shape from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return tupleShapeAttribute.readAsVector<usize>();
}

typename complex::IDataStore::ShapeType complex::HDF5::IDataStoreIO::ReadComponentShape(const H5::DatasetReader& datasetReader)
{
  H5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(complex::H5::k_ComponentShapeTag);
  if(!componentShapeAttribute.isValid())
  {
    throw std::runtime_error(fmt::format("Error reading Component Shape from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
  }
  return componentShapeAttribute.readAsVector<usize>();
}