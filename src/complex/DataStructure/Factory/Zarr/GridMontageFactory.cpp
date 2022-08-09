#include "GridMontageFactory.hpp"

#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/Group.hpp"

using namespace complex;
using namespace complex::Zarr;

GridMontageFactory::GridMontageFactory()
: IDataFactory()
{
}

GridMontageFactory::~GridMontageFactory() = default;

std::string GridMontageFactory::getDataTypeName() const
{
  return "GridMontage";
}

IDataFactory::error_type GridMontageFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::Group& parentReader, const FileVec::BaseCollection& baseReader,
                                                  const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const auto& groupReader = reinterpret_cast<const FileVec::Group&>(baseReader);
  std::string name = groupReader.name();
  auto importId = ReadObjectId(groupReader);
  auto montage = GridMontage::Import(dataStructureReader.getDataStructure(), name, importId, parentId);
  return montage->readZarr(dataStructureReader, groupReader, preflight);
}
