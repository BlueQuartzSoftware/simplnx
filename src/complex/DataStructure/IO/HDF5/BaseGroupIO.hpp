#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
class BaseGroup;

namespace HDF5
{
class COMPLEX_EXPORT BaseGroupIO : public IDataIO
{
public:
  BaseGroupIO();
  virtual ~BaseGroupIO() noexcept;

  BaseGroupIO(const BaseGroupIO& other) = delete;
  BaseGroupIO(BaseGroupIO&& other) = delete;
  BaseGroupIO& operator=(const BaseGroupIO& rhs) = delete;
  BaseGroupIO& operator=(BaseGroupIO&& rhs) = delete;

  static Result<> WriteDataMap(DataStructureWriter& dataStructureWriter, const DataMap& dataMap, group_writer_type& parentGroup, bool importable);

protected:
  static Result<> ReadBaseGroupData(DataStructureReader& dataStructureReader, BaseGroup& baseGroup, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false);

  static Result<> WriteBaseGroupData(DataStructureWriter& dataStructureWriter, const BaseGroup& baseGroup, group_writer_type& parentGroup, bool importable);
};
} // namespace HDF5
} // namespace complex
