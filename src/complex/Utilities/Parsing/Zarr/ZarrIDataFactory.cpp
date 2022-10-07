#include "ZarrIDataFactory.hpp"

#include "FileVec/collection/IGroup.hpp"

namespace complex
{
Zarr::IDataFactory::IDataFactory() = default;

Zarr::IDataFactory::~IDataFactory() = default;

DataObject::IdType Zarr::IDataFactory::ReadObjectId(const FileVec::BaseCollection& dataReader)
{
  nlohmann::json idAttribute = dataReader.attributes()[Constants::k_ObjectIdTag];
  return idAttribute.get<DataObject::IdType>();
}
} // namespace complex
