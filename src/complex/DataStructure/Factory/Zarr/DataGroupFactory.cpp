#include "DataGroupFactory.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IGroup.hpp"

using namespace complex;
using namespace complex::Zarr;

DataGroupFactory::DataGroupFactory()
: IDataFactory()
{
}

DataGroupFactory::~DataGroupFactory() = default;

std::string DataGroupFactory::getDataTypeName() const
{
  return "DataGroup";
}

IDataFactory::error_type DataGroupFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& group = dynamic_cast<const FileVec::IGroup&>(baseReader);

  auto name = group.name();
  auto importId = ReadObjectId(baseReader);
  auto dataGroup = DataGroup::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return -1;
  // return dataGroup->readZarr(dataStructureReader, group, preflight);
}
