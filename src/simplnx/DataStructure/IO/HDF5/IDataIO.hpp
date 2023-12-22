#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataFactory.hpp"

#include "simplnx/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp"

namespace nx::core
{
namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

class SIMPLNX_EXPORT IDataIO : public nx::core::IDataFactory
{
public:
  using group_reader_type = nx::core::HDF5::GroupReader;
  using group_writer_type = nx::core::HDF5::GroupWriter;
  using object_writer_type = nx::core::HDF5::ObjectWriter;
  using object_reader_type = nx::core::HDF5::ObjectReader;

  virtual ~IDataIO() noexcept;

  /**
   * @brief Attempts to read the DataObject from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param objectName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  virtual Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, DataObject::IdType importId,
                            const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const = 0;

  /**
   * @brief Attempts to write a DataObject to HDF5.
   * @param dataStructureWriter
   * @param dataObject
   * @param parentGroupWriter
   * @return Result<>
   */
  virtual Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentGroupWriter) const = 0;

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

  template <class IOClassT>
  static Result<> WriteDataObjectImpl(IOClassT* instance, DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter)
  {
    using T = typename IOClassT::data_type;
    const auto* targetData = dynamic_cast<const T*>(dataObject);
    if(targetData == nullptr)
    {
      return MakeErrorResult(-800, "Provided DataObject could not be cast to the target type");
    }

    return instance->writeData(dataStructureWriter, *targetData, parentWriter, true);
  }
};
} // namespace HDF5
} // namespace nx::core
