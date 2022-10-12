#pragma once

#include <stdexcept>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

#include <array>

namespace complex
{

/**
 * @class ScalarData
 * @brief The ScalarData class is designed to store a single scalar value
 * within the DataStructure. Like DataArrays, the stored value can be
 * retrieved or edited after the object has been constructed.
 */
template <typename T>
class ScalarData : public DataObject
{
public:
  using value_type = T;

  /**
   * @brief Attempts to create a ScalarData with the specified default value
   * and insert it into the DataStructure. If a parent ID is provided, the
   * ScalarData will be inserted under the target parent. Otherwise, the
   * ScalarData will be nested directly under the DataStructure.
   *
   * If the ScalarData cannot be created, this method returns nullptr.
   * Otherwise, a pointer to the created ScalarData will be returned.
   * @param ds
   * @param name
   * @param defaultValue
   * @param parentId = {}
   * @return ScalarData*
   */
  static ScalarData* Create(DataStructure& ds, const std::string& name, value_type defaultValue, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<ScalarData>(new ScalarData(ds, name, defaultValue));
    if(!AttemptToAddObject(ds, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Attempts to create a ScalarData with the specified default value
   * and insert it into the DataStructure. If a parent ID is provided, the
   * ScalarData will be inserted under the target parent. Otherwise, the
   * ScalarData will be nested directly under the DataStructure.
   *
   * If the ScalarData cannot be created, this method returns nullptr.
   * Otherwise, a pointer to the created ScalarData will be returned.
   *
   * Unlike Create, Import allows setting the DataObject ID for use in
   * importing DataObjects from external sources.
   * @param ds
   * @param name
   * @param importId
   * @param defaultValue
   * @param parentId = {}
   * @return ScalarData*
   */
  static ScalarData* Import(DataStructure& ds, const std::string& name, IdType importId, value_type defaultValue, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<ScalarData>(new ScalarData(ds, name, importId, defaultValue));
    if(!AttemptToAddObject(ds, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates a copy of the ScalarData that is not added to the
   * DataStructure. It is up to the caller to delete the returned object.
   * @param other
   */
  ScalarData(const ScalarData& other)
  : DataObject(other)
  , m_Data(other.m_Data)
  {
  }

  /**
   * @brief Creates a new ScalarData and moves values data from the target.
   * The returned ScalarData is not added to the DataStructure, and it is up
   * to the caller to delete it.
   * @param other
   */
  ScalarData(ScalarData&& other) noexcept
  : DataObject(std::move(other))
  , m_Data(std::move(other.m_Data))
  {
  }

  ~ScalarData() override = default;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override
  {
    return Type::ScalarData;
  }

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return "ScalarData";
  }

  /**
   * @brief Returns a shallow copy of the ScalarData. The created ScalarData
   * is not added to the DataStructure, and it is up to the caller to delete
   * it.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    return new ScalarData(*this);
  }

  /**
   * @brief Returns a deep copy of the ScalarData. This copy is not added to
   * the DataStructure, and it is up to the caller to delete it.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override
  {
    auto& dataStruct = *getDataStructure();
    std::shared_ptr<ScalarData<T>> copy = std::shared_ptr<ScalarData<T>>(new ScalarData<T>(dataStruct, copyPath.getTargetName(), getId(), getValue()));
    dataStruct.insert(copy, copyPath.getParent());
    return copy;
  }

  /**
   * @brief Returns the current scalar value.
   * @return value_type
   */
  value_type getValue() const
  {
    return m_Data;
  }

  /**
   * @brief Sets the current scalar value.
   * @param data
   */
  void setValue(value_type data)
  {
    m_Data = data;
  }

  /**
   * @brief Assigns a new scalar value.
   * @param data
   * @return ScalarData&
   */
  ScalarData& operator=(value_type data)
  {
    m_Data = data;
    return *this;
  }

  /**
   * @brief Copies the value from another ScalarData.
   * @param rhs
   * @return ScalarData&
   */
  ScalarData& operator=(const ScalarData& rhs)
  {
    m_Data = rhs.m_Data;
    return *this;
  }

  /**
   * @brief Moves the value from another ScalarData.
   * @param rhs
   * @return ScalarData&
   */
  ScalarData& operator=(ScalarData&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
    return *this;
  }

  /**
   * @brief Checks for value equality. Returns true if the values match.
   * Returns false otherwise.
   * @param rhs
   * @return bool
   */
  bool operator==(value_type rhs) const
  {
    return m_Data == rhs;
  }

  /**
   * @brief Checks for value inequality. Returns true if the values do not
   * match. Returns false otherwise.
   * @param rhs
   * @return bool
   */
  bool operator!=(value_type rhs) const
  {
    return m_Data != rhs;
  }

  /**
   * @brief Writes the ScalarData to HDF5 using the provided parent ID.
   * @param parentId
   * @param dataId
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override
  {
    auto datasetWriter = parentGroupWriter.createDatasetWriter(getName());

    H5::DatasetWriter::DimsType dims = {1};
    std::array<T, 1> dataVector = {m_Data};
    auto error = datasetWriter.writeSpan(dims, nonstd::span<const T>{dataVector});
    if(error == 0)
    {
      error = writeH5ObjectAttributes(dataStructureWriter, datasetWriter, importable);
    }
    return error;
  }

protected:
  /**
   * @brief Constructs a ScalarData object with the target name and value.
   * @param ds
   * @param name
   * @param defaultValue
   */
  ScalarData(DataStructure& ds, const std::string& name, value_type defaultValue)
  : DataObject(ds, name)
  , m_Data(defaultValue)
  {
  }

  /**
   * @brief Constructs a ScalarData object with the target name and value.
   * @param ds
   * @param name
   * @param importId
   * @param defaultValue
   */
  ScalarData(DataStructure& ds, const std::string& name, IdType importId, value_type defaultValue)
  : DataObject(ds, name, importId)
  , m_Data(defaultValue)
  {
  }

private:
  value_type m_Data;
};
} // namespace complex
