#pragma once

#include "Complex/DataStructure/DataObject.h"

namespace Complex
{

/**
 * class ScalarData
 *
 */

template <typename T>
class ScalarData : public DataObject
{
public:
  ScalarData(const ScalarData& other)
  : DataObject(other)
  , m_Data(other.m_Data)
  {
  }

  ScalarData(ScalarData&& other) noexcept
  : DataObject(std::move(other))
  , m_Data(std::move(other.m_Data))
  {
  }

  virtual ~ScalarData() = default;

  /**
   * @brief
   * @return
   */
  T getData() const
  {
    return m_Data;
  }

  /**
   * @brief
   * @param data
   */
  void setData(T data)
  {
    m_Data = data;
  }

  ScalarData& operator=(T data)
  {
    m_Data = data;
    return *this;
  }
  ScalarData& operator=(const ScalarData& rhs)
  {
    m_Data = rhs.m_Data;
    return *this;
  }
  ScalarData& operator=(ScalarData&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
    return *this;
  }

protected:
  /**
   * @brief
   * @param  name
   * @param  defaultValue
   */
  ScalarData(DataStructure* ds, const std::string& name, T defaultValue)
  : DataObject(ds, name)
  , m_Data(defaultValue)
  {
  }

private:
  T m_Data;
};
} // namespace SIMPL
