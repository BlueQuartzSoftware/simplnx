#include "NeighborListFactory.hpp"

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Utilities/Parsing/Zarr/ZarrStructureReader.hpp"

#include "FileVec/collection/IArray.hpp"

#include <optional>

using namespace complex;
using namespace complex::Zarr;

NeighborListFactory::NeighborListFactory()
: IDataFactory()
{
}

NeighborListFactory::~NeighborListFactory() = default;

std::string NeighborListFactory::getDataTypeName() const
{
  return "NeighborList<T>";
}

template <typename T>
inline void importNeighborList(DataStructure& dataStructure, const FileVec::IGroup& parentReader, const FileVec::BaseGenericArray& iArray, DataObject::IdType importId,
                               const std::optional<DataObject::IdType>& parentId, bool preflight)
{
  using NeighborListType = NeighborList<T>;

  const auto& fileArray = dynamic_cast<const FileVec::IArray<T>&>(iArray);
  auto dataVector = NeighborListType::ReadZarrData(parentReader, fileArray);
  NeighborListType::Import(dataStructure, iArray.name(), importId, dataVector, parentId);
}

IDataFactory::error_type NeighborListFactory::read(Zarr::DataStructureReader& dataStructureReader, const FileVec::IGroup& parentReader, const FileVec::BaseCollection& baseReader,
                                                   const std::optional<complex::DataObject::IdType>& parentId, bool preflight)
{
  const FileVec::BaseGenericArray& iArray = reinterpret_cast<const FileVec::BaseGenericArray&>(baseReader);
  const FileVec::DataType type = iArray.header()->dataType();

  H5::ErrorType err = 0;

  std::string dataArrayName = iArray.name();
  DataObject::IdType importId = ReadObjectId(iArray);

  switch(type)
  {
  case FileVec::DataType::float32: {
    importNeighborList<float32>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::float64: {
    importNeighborList<float64>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::int8: {
    importNeighborList<int8>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::int16: {
    importNeighborList<int16>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::int32: {
    importNeighborList<int32>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::int64: {
    importNeighborList<int64>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::uint8: {
    importNeighborList<uint8>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::uint16: {
    importNeighborList<uint16>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::uint32: {
    importNeighborList<uint32>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  case FileVec::DataType::uint64: {
    importNeighborList<uint64>(dataStructureReader.getDataStructure(), parentReader, iArray, importId, parentId, preflight);
    break;
  }
  default:
    err = -777;
    break;
  }

  return err;
}
