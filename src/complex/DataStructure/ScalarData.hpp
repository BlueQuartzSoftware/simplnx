#pragma once

#include <stdexcept>

#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

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
   * @brief
   * @param ds
   * @param name
   * @param defaultValue
   * @param parentId = {}
   * @return ScalarData*
   */
  static ScalarData* ScalarData::Create(DataStructure& ds, const std::string& name, value_type defaultValue, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<ScalarData>(new ScalarData(ds, name, defaultValue));
    if(!AddObjectToDS(ds, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  ScalarData(const ScalarData& other)
  : DataObject(other)
  , m_Data(other.m_Data)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  ScalarData(ScalarData&& other) noexcept
  : DataObject(std::move(other))
  , m_Data(std::move(other.m_Data))
  {
  }

  virtual ~ScalarData() = default;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return "ScalarData";
  }

  /**
   * @brief Returns a shallow copy of the ScalarData.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    return new ScalarData(*this);
  }

  /**
   * @brief Returns a deep copy of the ScalarData.
   * @return DataObject*
   */
  DataObject* deepCopy() override
  {
    return Create(*getDataStructure(), getName(), getValue());
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
   * @brief Assignment operator
   * @param data
   * @return ScalarData&
   */
  ScalarData& operator=(value_type data)
  {
    m_Data = data;
    return *this;
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return ScalarData&
   */
  ScalarData& operator=(const ScalarData& rhs)
  {
    m_Data = rhs.m_Data;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return
   */
  ScalarData& operator=(ScalarData&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
    return *this;
  }

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(value_type rhs) const
  {
    return m_Data == rhs;
  }

  /**
   * @brief Inequality operator
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
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType dataId) const
  {
    return H5::Writer::Generic::writeScalarAttribute(parentId, getName(), "Value", m_Data);
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

private:
  value_type m_Data;
};
} // namespace complex
