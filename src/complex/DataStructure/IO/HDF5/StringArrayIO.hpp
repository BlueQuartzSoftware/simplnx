#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"

namespace complex
{
class StringArray;

namespace HDF5
{
class COMPLEX_EXPORT StringArrayIO : public IDataIO
{
public:
  using data_type = StringArray;

  StringArrayIO();
  virtual ~StringArrayIO() noexcept;

  StringArrayIO(const StringArrayIO& other) = delete;
  StringArrayIO(StringArrayIO&& other) = delete;
  StringArrayIO& operator=(const StringArrayIO& rhs) = delete;
  StringArrayIO& operator=(StringArrayIO&& rhs) = delete;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const data_type& dataGroup, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace complex
