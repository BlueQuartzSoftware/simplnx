#include "ScalarDataFactory.hpp"

#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataStructureReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5ObjectReader.hpp"

using namespace complex;
using namespace complex::H5;

ScalarDataFactory::ScalarDataFactory()
: H5::IDataFactory()
{
}

ScalarDataFactory::~ScalarDataFactory() = default;

std::string ScalarDataFactory::getDataTypeName() const
{
  return "ScalarData";
}

template <typename T>
H5::ErrorType readH5Scalar(DataStructure& dataStructure, const H5::AttributeReader& attributeReader, const std::string& name, DataObject::IdType importId,
                           const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  T buffer = attributeReader.readAsValue<T>();
  ScalarData<T>* scalar = ScalarData<T>::Import(dataStructure, name, importId, buffer, parentId);
  return err;
}

H5::ErrorType ScalarDataFactory::readH5Group(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::GroupReader& groupReader,
                                             const std::optional<DataObject::IdType>& parentId)
{
  std::string name = groupReader.getName();
  auto importId = ReadObjectId(groupReader);
  auto valueAttribute = groupReader.getAttribute("Value");
  auto valueType = valueAttribute.getType();
  switch(valueType)
  {
  case H5::Type::float32:
    return readH5Scalar<float>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::float64:
    return readH5Scalar<double>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::int8:
    return readH5Scalar<int8_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::int16:
    return readH5Scalar<int16_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::int32:
    return readH5Scalar<int32_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::int64:
    return readH5Scalar<int64_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::uint8:
    return readH5Scalar<uint8_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::uint16:
    return readH5Scalar<uint16_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::uint32:
    return readH5Scalar<uint32_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case H5::Type::uint64:
    return readH5Scalar<uint64_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  default:
    return 0;
  }
}

//------------------------------------------------------------------------------
H5::ErrorType ScalarDataFactory::readH5Dataset(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& parentReader, const H5::DatasetReader& datasetReader,
                                               const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
