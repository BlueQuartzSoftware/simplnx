#pragma once

#include "complex/DataStructure/DataObject.hpp"

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
   * @brief Returns the current scalar value.
   * @return value_type
   */
  value_type getData() const
  {
    return m_Data;
  }

  /**
   * @brief Sets the current scalar value.
   * @param data
   */
  void setData(value_type data)
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

protected:
  /**
   * @brief Constructs a ScalarData object with the target name and value.
   * @param ds
   * @param name
   * @param defaultValue
   */
  ScalarData(DataStructure* ds, const std::string& name, value_type defaultValue)
  : DataObject(ds, name)
  , m_Data(defaultValue)
  {
  }

private:
  value_type m_Data;
};
} // namespace complex
