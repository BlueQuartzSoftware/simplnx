#include "ScalarDataFactory.hpp"

#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"

using namespace complex;

ScalarDataFactory::ScalarDataFactory()
: IH5DataFactory()
{
}

ScalarDataFactory::~ScalarDataFactory() = default;

std::string ScalarDataFactory::getDataTypeName() const
{
  return "ScalarData";
}

template <typename T>
H5::ErrorType readH5Scalar(DataStructure& ds, const H5::AttributeReader& attributeReader, const std::string& name, const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  T buffer = attributeReader.readAsValue<T>();
  ScalarData<T>* scalar = ScalarData<T>::Create(ds, name, buffer, parentId);
  return err;
}

H5::ErrorType ScalarDataFactory::readDataStructureGroup(DataStructure& ds, const H5::GroupReader& groupReader, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto valueAttribute = groupReader.getAttribute("Value");
  auto valueType = valueAttribute.getType();
  switch(valueType)
  {
  case H5::Type::float32:
    return readH5Scalar<float>(ds, valueAttribute, name, parentId);
  case H5::Type::float64:
    return readH5Scalar<double>(ds, valueAttribute, name, parentId);
  case H5::Type::int8:
    return readH5Scalar<int8_t>(ds, valueAttribute, name, parentId);
  case H5::Type::int16:
    return readH5Scalar<int16_t>(ds, valueAttribute, name, parentId);
  case H5::Type::int32:
    return readH5Scalar<int32_t>(ds, valueAttribute, name, parentId);
  case H5::Type::int64:
    return readH5Scalar<int64_t>(ds, valueAttribute, name, parentId);
  case H5::Type::uint8:
    return readH5Scalar<uint8_t>(ds, valueAttribute, name, parentId);
  case H5::Type::uint16:
    return readH5Scalar<uint16_t>(ds, valueAttribute, name, parentId);
  case H5::Type::uint32:
    return readH5Scalar<uint32_t>(ds, valueAttribute, name, parentId);
  case H5::Type::uint64:
    return readH5Scalar<uint64_t>(ds, valueAttribute, name, parentId);
  default:
    return 0;
  }
}

//------------------------------------------------------------------------------
H5::ErrorType ScalarDataFactory::readDataStructureDataset(DataStructure& ds, const H5::DatasetReader& datasetReader, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
