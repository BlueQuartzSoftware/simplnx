#include "ScalarDataFactory.hpp"

#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

ScalarDataFactory::ScalarDataFactory()
: IDataFactory()
{
}

ScalarDataFactory::~ScalarDataFactory() = default;

std::string ScalarDataFactory::getDataTypeName() const
{
  return "ScalarData";
}

template <typename T>
H5::ErrorType readZarrScalar(DataStructure& dataStructure, const nlohmann::json& attributeReader, const std::string& name, DataObject::IdType importId,
                             const std::optional<DataObject::IdType>& parentId)
{
  H5::ErrorType err = 0;
  T buffer = attributeReader.get<T>();
  ScalarData<T>* scalar = ScalarData<T>::Import(dataStructure, name, importId, buffer, parentId);
  return err;
}

IDataFactory::error_type ScalarDataFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                 const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& iArrayReader = reinterpret_cast<const FileVec::BaseGenericArray&>(baseReader);
  std::string name = iArrayReader.name();
  auto importId = ReadObjectId(iArrayReader);
  auto valueAttribute = iArrayReader.attributes()["Value"];
  auto valueType = iArrayReader.header()->dataType();
  switch(valueType)
  {
  case FileVec::DataType::float32:
    return readZarrScalar<float>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::float64:
    return readZarrScalar<double>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::int8:
    return readZarrScalar<int8_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::int16:
    return readZarrScalar<int16_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::int32:
    return readZarrScalar<int32_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::int64:
    return readZarrScalar<int64_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::uint8:
    return readZarrScalar<uint8_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::uint16:
    return readZarrScalar<uint16_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::uint32:
    return readZarrScalar<uint32_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  case FileVec::DataType::uint64:
    return readZarrScalar<uint64_t>(dataStructureReader.getDataStructure(), valueAttribute, name, importId, parentId);
  default:
    return 0;
  }
}
