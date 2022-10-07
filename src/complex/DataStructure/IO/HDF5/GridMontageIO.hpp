#pragma once

#include "complex/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace complex
{
class GridMontage;

namespace HDF5
{
class COMPLEX_EXPORT GridMontageIO : public BaseGroupIO
{
public:
  using data_type = GridMontage;

  GridMontageIO();
  virtual ~GridMontageIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const;
  Result<> writeData(DataStructureWriter& structureWriter, const GridMontage& montage, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;

  GridMontageIO(const GridMontageIO& other) = delete;
  GridMontageIO(GridMontageIO&& other) = delete;
  GridMontageIO& operator=(const GridMontageIO& rhs) = delete;
  GridMontageIO& operator=(GridMontageIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
