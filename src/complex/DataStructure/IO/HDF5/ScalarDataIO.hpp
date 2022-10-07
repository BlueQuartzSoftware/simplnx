#pragma once

#include "complex/DataStructure/IO/HDF5/IDataIO.hpp"
#include "complex/DataStructure/ScalarData.hpp"

namespace complex
{
namespace HDF5
{
template <typename T>
class ScalarDataIO : public IDataIO
{
public:
  using data_type = ScalarData<T>;

  ScalarDataIO();
  virtual ~ScalarDataIO() noexcept;

  Result<> readData(DataStructureReader& structureReader, const group_reader_type& parentGroup, const std::string_view& objectName, DataObject::IdType importId,
                    const std::optional<DataObject::IdType>& parentId, bool useEmptyDataStore = false) const override;
  Result<> writeData(DataStructureWriter& structureReader, const ScalarData<T>& scalarData, group_writer_type& parentGroup, bool importable) const;

  Result<> writeDataObject(DataStructureWriter& dataStructureWriter, const DataObject* dataObject, group_writer_type& parentWriter) const override
  {
    auto* targetData = dynamic_cast<const data_type*>(dataObject);
    if(targetData == nullptr)
    {
      std::string ss = "Provided DataObject could not be cast to the target type";
      return MakeErrorResult(-800, ss);
    }

    return writeData(dataStructureWriter, *targetData, parentWriter, true);
  }

  DataObject::Type getDataType() const override
  {
    return DataObject::Type::ScalarData;
  }

  std::string getTypeName() const override
  {
    return data_type::GetTypeName();
  }

  ScalarDataIO(const ScalarDataIO& other) = delete;
  ScalarDataIO(ScalarDataIO&& other) = delete;
  ScalarDataIO& operator=(const ScalarDataIO& rhs) = delete;
  ScalarDataIO& operator=(ScalarDataIO&& rhs) = delete;
};
} // namespace HDF5
} // namespace complex
