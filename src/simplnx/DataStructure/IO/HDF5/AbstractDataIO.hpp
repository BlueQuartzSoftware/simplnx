#pragma once

#include "simplnx/DataStructure/AbstractDataObject.hpp"
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

class SIMPLNX_EXPORT AbstractDataIO : public nx::core::IDataFactory
{
public:
  using group_reader_type = nx::core::HDF5::GroupIO;
  using group_writer_type = nx::core::HDF5::GroupIO;
  using object_writer_type = nx::core::HDF5::ObjectIO;
  using object_reader_type = nx::core::HDF5::ObjectIO;

  ~AbstractDataIO() noexcept override;

  /**
   * @brief Attempts to read the AbstractDataObject from HDF5.
   * Returns a Result<> with any errors or warnings encountered during the process.
   * @param dataStructureReader
   * @param parentGroup
   * @param objectName
   * @param importId
   * @param parentId
   * @param useEmptyDataStore = false
   * @return Result<>
   */
  virtual Result<> readData(DataStructureReader& dataStructureReader, const group_reader_type& parentGroup, const std::string& objectName, AbstractDataObject::IdType importId,
                            const std::optional<AbstractDataObject::IdType>& parentId, bool useEmptyDataStore = false) const = 0;

  virtual Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& dataStructureGroup) const;

  /**
   * @brief Attempts to write a AbstractDataObject to HDF5.
   * @param dataStructureWriter
   * @param dataObject
   * @param parentGroupIO
   * @return Result<>
   */
  virtual Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentGroupIO) const = 0;

  virtual std::string getTypeName() const = 0;

  AbstractDataIO(const AbstractDataIO& other) = delete;
  AbstractDataIO(AbstractDataIO&& other) = delete;
  AbstractDataIO& operator=(const AbstractDataIO& rhs) = delete;
  AbstractDataIO& operator=(AbstractDataIO&& rhs) = delete;

protected:
  static AbstractDataObject::OptionalId ReadDataId(const object_reader_type& groupReader, const std::string& tag);
  static Result<> WriteDataId(object_writer_type& groupWriter, const std::optional<AbstractDataObject::IdType>& objectId, const std::string& tag);
  static Result<> WriteObjectAttributes(DataStructureWriter& dataStructureWriter, const AbstractDataObject& dataObject, object_writer_type& objectWriter, bool importable);
  AbstractDataIO();

  template <class IOClassT>
  static Result<> WriteDataObjectImpl(IOClassT* instance, DataStructureWriter& dataStructureWriter, const AbstractDataObject* dataObject, group_writer_type& parentWriter)
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
