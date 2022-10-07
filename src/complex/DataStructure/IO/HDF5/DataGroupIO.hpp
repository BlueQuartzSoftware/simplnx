#pragma once

#include "complex/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace complex
{
class DataGroup;

namespace HDF5
{
class COMPLEX_EXPORT DataGroupIO : public BaseGroupIO
{
public:
  using data_type = DataGroup;

  DataGroupIO();
  virtual ~DataGroupIO() noexcept;

  DataGroupIO(const DataGroupIO& other) = delete;
  DataGroupIO(DataGroupIO&& other) = delete;
  DataGroupIO& operator=(const DataGroupIO& rhs) = delete;
  DataGroupIO& operator=(DataGroupIO&& rhs) = delete;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const DataGroup& dataGroup, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace complex
