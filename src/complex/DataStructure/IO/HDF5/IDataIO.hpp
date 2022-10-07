#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IO/Generic/IDataFactory.hpp"

#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

namespace complex
{
namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

class COMPLEX_EXPORT IDataIO : public complex::IDataFactory
{
public:
  using group_reader_type = H5::GroupReader;
  using group_writer_type = H5::GroupWriter;
  using object_writer_type = H5::ObjectWriter;
  using object_reader_type = H5::ObjectReader;

  virtual ~IDataIO() noexcept;

  virtual Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                            const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const = 0;

  virtual Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const = 0;

  virtual std::string getTypeName() const = 0;

  IDataIO(const IDataIO& other) = delete;
  IDataIO(IDataIO&& other) = delete;
  IDataIO& operator=(const IDataIO& rhs) = delete;
  IDataIO& operator=(IDataIO&& rhs) = delete;

protected:
  static DataObject::OptionalId ReadDataId(const object_reader_type& groupReader, const std::string& tag);
  static Result<> WriteDataId(object_writer_type& groupWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag);
  static Result<> WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, object_writer_type& objectWriter, bool importable);
  IDataIO();
};
} // namespace HDF5
} // namespace complex