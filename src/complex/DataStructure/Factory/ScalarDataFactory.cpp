#include "ScalarDataFactory.hpp"

#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"

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
H5::ErrorType readH5Scalar(DataStructure& ds, H5::IdType attrId, const std::string& name, const std::optional<DataObject::IdType>& parentId)
{
  T buffer = 0;
  hid_t dataType = H5::Support::HDFTypeForPrimitive<T>();
  H5::ErrorType err = H5Aread(attrId, dataType, &buffer);
  if(err < 0)
  {
    throw std::runtime_error("Error reading Scalar data from HDF5");
  }

  ScalarData<T>* scalar = ScalarData<T>::Create(ds, name, buffer, parentId);
  return err;
}

H5::ErrorType ScalarDataFactory::readDataStructureGroup(DataStructure& ds, H5::IdType targetId, H5::IdType groupId, const std::optional<DataObject::IdType>& parentId)
{
  std::string name = H5::Reader::Generic::getName(targetId);
  auto valueId = H5Aopen(targetId, "Value", H5P_DEFAULT);
  auto typeId = H5Aget_type(valueId);
  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    return readH5Scalar<float32>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    return readH5Scalar<float64>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    return readH5Scalar<int8>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    return readH5Scalar<int16>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    return readH5Scalar<int32>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    return readH5Scalar<int64>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    return readH5Scalar<uint8>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    return readH5Scalar<uint16>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    return readH5Scalar<uint32>(ds, valueId, name, parentId);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    return readH5Scalar<uint64>(ds, valueId, name, parentId);
  }
  H5Tclose(typeId);
  H5Aclose(valueId);
  return 0;
}

//------------------------------------------------------------------------------
H5::ErrorType ScalarDataFactory::readDataStructureDataset(DataStructure& ds, H5::IdType h5LocationId, const std::string& h5DatasetName, const std::optional<DataObject::IdType>& parentId)
{
  return -1;
}
