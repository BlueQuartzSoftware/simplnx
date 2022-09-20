#pragma once

#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"

namespace complex {
namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

class COMPLEX_EXPORT IDataIO : public IDataFactory
{
public:
  using parent_group_type = H5::GroupReader;
  using object_reader_type = H5::ObjectReader;

  IDataIO();
  virtual ~IDataIO() noexcept;

  virtual Result<> readData(DataStructureReader& structureReader, const parent_group_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId, const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const = 0;
  virtual Result<> writeData(DataStructureWriter& structureReader, const parent_group_type& parentGroup, bool importable) const = 0;

  IDataIO(const IDataIO& other) = delete;
  IDataIO(IDataIO&& other) = delete;
  IDataIO& operator=(const IDataIO& rhs) = delete;
  IDataIO& operator=(IDataIO&& rhs) = delete;
};
}
}