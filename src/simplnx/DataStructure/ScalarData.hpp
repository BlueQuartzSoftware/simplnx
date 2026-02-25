#pragma once

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

namespace nx::core
{

namespace ScalarDataConstants
{
inline constexpr StringLiteral k_TypeName = "ScalarData";
}

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
   * @param dataStructure The DataStructure to add the ScalarData to
   * @param name The name for the ScalarData object
   * @param defaultValue The initial value for the scalar
   * @param parentId Optional parent ID for hierarchical organization
   * @return Pointer to the created ScalarData, or nullptr if creation failed
   */
  static ScalarData* Create(DataStructure& dataStructure, const std::string& name, value_type defaultValue, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<ScalarData>(new ScalarData(dataStructure, name, defaultValue));
    if(!AttemptToAddObject(dataStructure, data, parentId))
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
   * @param dataStructure The DataStructure to add the ScalarData to
   * @param name The name for the ScalarData object
   * @param importId The ID to assign to the imported ScalarData
   * @param defaultValue The initial value for the scalar
   * @param parentId Optional parent ID for hierarchical organization
   * @return Pointer to the imported ScalarData, or nullptr if import failed
   */
  static ScalarData* Import(DataStructure& dataStructure, const std::string& name, IdType importId, value_type defaultValue, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<ScalarData>(new ScalarData(dataStructure, name, importId, defaultValue));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates a copy of the ScalarData that is not added to the
   * DataStructure. It is up to the caller to delete the returned object.
   * @param other The ScalarData to copy from
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
   * @param other The ScalarData to move from
   */
  ScalarData(ScalarData&& other) noexcept
  : DataObject(std::move(other))
  , m_Data(std::move(other.m_Data))
  {
  }

  ~ScalarData() override = default;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return The DataObject type enumeration value (Type::ScalarData)
   */
  DataObject::Type getDataObjectType() const override
  {
    return Type::ScalarData;
  }

  /**
   * @brief Static function to get the typename based on the template parameter.
   * @return String representation of the ScalarData type
   */
  static std::string GetTypeName()
  {
    if constexpr(std::is_same_v<T, int8>)
    {
      return "ScalarData<int8>";
    }
    else if constexpr(std::is_same_v<T, uint8>)
    {
      return "ScalarData<uint8>";
    }
    else if constexpr(std::is_same_v<T, int16>)
    {
      return "ScalarData<int16>";
    }
    else if constexpr(std::is_same_v<T, uint16>)
    {
      return "ScalarData<uint16>";
    }
    else if constexpr(std::is_same_v<T, int32>)
    {
      return "ScalarData<int32>";
    }
    else if constexpr(std::is_same_v<T, uint32>)
    {
      return "ScalarData<uint32>";
    }
    else if constexpr(std::is_same_v<T, int64>)
    {
      return "ScalarData<int64>";
    }
    else if constexpr(std::is_same_v<T, uint64>)
    {
      return "ScalarData<uint64>";
    }
    else if constexpr(std::is_same_v<T, float32>)
    {
      return "ScalarData<float32>";
    }
    else if constexpr(std::is_same_v<T, float64>)
    {
      return "ScalarData<float64>";
    }
    else if constexpr(std::is_same_v<T, bool>)
    {
      return "ScalarData<bool>";
    }
    else
    {
      static_assert(dependent_false<T>, "Unsupported type T in ScalarData");
    }
  }

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return String representation of the DataObject type name
   */
  std::string getTypeName() const override
  {
    return GetTypeName();
  }

  /**
   * @brief Returns a shallow copy of the ScalarData. The created ScalarData
   * is not added to the DataStructure, and it is up to the caller to delete
   * it.
   * @return Pointer to the shallow copy
   */
  DataObject* shallowCopy() override
  {
    return new ScalarData(*this);
  }

  /**
   * @brief Returns a deep copy of the ScalarData at the specified path.
   * @param copyPath The DataPath where the deep copy will be placed
   * @return Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override
  {
    auto& dataStruct = *getDataStructure();
    if(dataStruct.containsData(copyPath))
    {
      return nullptr;
    }
    std::shared_ptr<ScalarData<T>> copy = std::shared_ptr<ScalarData<T>>(new ScalarData<T>(dataStruct, copyPath.getTargetName(), getId(), getValue()));
    if(dataStruct.insert(copy, copyPath.getParent()))
    {
      return copy;
    }
    return nullptr;
  }

  /**
   * @brief Returns the current scalar value.
   * @return The current scalar value
   */
  value_type getValue() const
  {
    return m_Data;
  }

  /**
   * @brief Sets the current scalar value.
   * @param data The new value to assign
   */
  void setValue(value_type data)
  {
    m_Data = data;
  }

  /**
   * @brief Assigns a new scalar value.
   * @param data The new value to assign
   * @return Reference to this ScalarData
   */
  ScalarData& operator=(value_type data)
  {
    m_Data = data;
    return *this;
  }

  /**
   * @brief Copies the value from another ScalarData.
   * @param rhs The ScalarData to copy from
   * @return Reference to this ScalarData
   */
  ScalarData& operator=(const ScalarData& rhs)
  {
    m_Data = rhs.m_Data;
    return *this;
  }

  /**
   * @brief Moves the value from another ScalarData.
   * @param rhs The ScalarData to move from
   * @return Reference to this ScalarData
   */
  ScalarData& operator=(ScalarData&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
    return *this;
  }

  /**
   * @brief Checks for value equality. Returns true if the values match.
   * Returns false otherwise.
   * @param rhs The value to compare against
   * @return True if values are equal, false otherwise
   */
  bool operator==(value_type rhs) const
  {
    return m_Data == rhs;
  }

  /**
   * @brief Checks for value inequality. Returns true if the values do not
   * match. Returns false otherwise.
   * @param rhs The value to compare against
   * @return True if values are not equal, false otherwise
   */
  bool operator!=(value_type rhs) const
  {
    return m_Data != rhs;
  }

protected:
  /**
   * @brief Protected constructor creates a ScalarData object with the target name and value.
   * @param dataStructure The DataStructure that will own this ScalarData
   * @param name The name for the ScalarData object
   * @param defaultValue The initial value for the scalar
   */
  ScalarData(DataStructure& dataStructure, const std::string& name, value_type defaultValue)
  : DataObject(dataStructure, name)
  , m_Data(defaultValue)
  {
  }

  /**
   * @brief Protected constructor creates a ScalarData object with the target name, import ID, and value.
   * @param dataStructure The DataStructure that will own this ScalarData
   * @param name The name for the ScalarData object
   * @param importId The ID to assign to this ScalarData
   * @param defaultValue The initial value for the scalar
   */
  ScalarData(DataStructure& dataStructure, const std::string& name, IdType importId, value_type defaultValue)
  : DataObject(dataStructure, name, importId)
  , m_Data(defaultValue)
  {
  }

private:
  value_type m_Data;
};
} // namespace nx::core
