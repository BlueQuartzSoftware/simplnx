#include "IH5DataFactory.hpp"

#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"

using namespace complex;

IH5DataFactory::IH5DataFactory() = default;

IH5DataFactory::~IH5DataFactory() = default;

DataObject::IdType IH5DataFactory::ReadObjectId(const H5::ObjectReader& dataReader)
{
  auto idAttribute = dataReader.getAttribute(Constants::k_ObjectIdTag);
  return idAttribute.readAsValue<DataObject::IdType>();
}
