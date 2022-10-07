#pragma once

#include "complex/DataStructure/IO/HDF5/BaseGroupIO.hpp"

namespace complex
{
class AttributeMatrix;

namespace HDF5
{
class COMPLEX_EXPORT AttributeMatrixIO : public BaseGroupIO
{
public:
  using data_type = AttributeMatrix;

  AttributeMatrixIO();
  virtual ~AttributeMatrixIO() noexcept;

  AttributeMatrixIO(const AttributeMatrixIO& other) = delete;
  AttributeMatrixIO(AttributeMatrixIO&& other) = delete;
  AttributeMatrixIO& operator=(const AttributeMatrixIO& rhs) = delete;
  AttributeMatrixIO& operator=(AttributeMatrixIO&& rhs) = delete;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const data_type& dataGroup, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override;

  DataObject::Type getDataType() const override;

  std::string getTypeName() const override;
};
} // namespace HDF5
} // namespace complex
