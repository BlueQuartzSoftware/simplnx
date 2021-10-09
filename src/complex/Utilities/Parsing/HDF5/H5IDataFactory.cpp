#include "H5IDataFactory.hpp"

#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"

using namespace complex;

H5::IDataFactory::IDataFactory() = default;

H5::IDataFactory::~IDataFactory() = default;

DataObject::IdType H5::IDataFactory::ReadObjectId(const H5::ObjectReader& dataReader)
{
  auto idAttribute = dataReader.getAttribute(Constants::k_ObjectIdTag);
  return idAttribute.readAsValue<DataObject::IdType>();
}
