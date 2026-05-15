#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataFactory.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core
{
class DataStructure;

namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

using ErrorType = int32;

class SIMPLNX_EXPORT IDataIO : public nx::core::IDataFactory
{
public:
  using group_reader_type = nx::core::HDF5::GroupIO;
  using group_writer_type = nx::core::HDF5::GroupIO;
  using object_writer_type = nx::core::HDF5::ObjectIO;
  using object_reader_type = nx::core::HDF5::ObjectIO;

  ~IDataIO() noexcept override;

  static Result<> ReadMetaData(DataStructureReader& dataStructureReader, const group_reader_type& parentReader, const std::string& objectName, const DataObject::IdType& objectId);

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
   * @brief Finishes importing data after the DataObject has been created and added to the DataStructure.
   * @param dataStructure The DataStructure containing the imported data
   * @param dataPath The path to the imported DataObject
   * @param dataStructureGroup The HDF5 group containing the data
   * @return Result<> Result with any errors or warnings
   */
  virtual Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const;

  /**
   * @brief Attempts to write a DataObject to HDF5.
   * @param dataStructureWriter
   * @param dataObject
   * @param parentGroupIO
   * @return Result<>
   */
  virtual Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentGroupIO) const = 0;

  virtual std::string getTypeName() const = 0;

  IDataIO(const IDataIO& other) = delete;
  IDataIO(IDataIO&& other) = delete;
  IDataIO& operator=(const IDataIO& rhs) = delete;
  IDataIO& operator=(IDataIO&& rhs) = delete;

protected:
  /**
   * @brief Reads a DataObject ID from an HDF5 group attribute.
   * @param groupReader The HDF5 group reader to read from
   * @param tag The attribute tag name to read
   * @return DataObject::OptionalId The read ID or std::nullopt if not present
   */
  static DataObject::OptionalId ReadDataId(const object_reader_type& groupReader, const std::string& tag);

  /**
   * @brief Writes a DataObject ID to an HDF5 group attribute.
   * @param groupWriter The HDF5 group writer to write to
   * @param objectId The ID to write
   * @param tag The attribute tag name to write
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteDataId(object_writer_type& groupWriter, const std::optional<DataObject::IdType>& objectId, const std::string& tag);

  /**
   * @brief Writes common DataObject attributes (name, ID, etc.) to HDF5.
   * @param dataStructureWriter The DataStructure writer
   * @param dataObject The DataObject whose attributes to write
   * @param objectWriter The HDF5 object writer to write to
   * @param importable Whether the object is importable
   * @return Result<> Result with any errors or warnings
   */
  static Result<> WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const DataObject& dataObject, object_writer_type& objectWriter, bool importable);

  static Result<> ReadMetaData(DataStructureReader& dataStructureReader, const DataObject::IdType& objectId, const object_reader_type& objectReader);

  /**
   * @brief Protected constructor.
   */
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
