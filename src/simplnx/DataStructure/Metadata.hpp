#pragma once

#include "simplnx/DataStructure/Metadata/AbstractMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <any>
#include <map>
#include <memory>
#include <string>

namespace nx::core
{

/**
 * @class Metadata
 * @brief The Metadata class stores additional information related to a
 * DataObject. Information is stored using key value pairs such that many
 * pieces of information can be quickly inserted or retrieved. The Metadata
 * class is designed such that any type of information can be included from
 * color formats, descriptions, data source, etc.
 */
class SIMPLNX_EXPORT Metadata
{
public:
  using KeyType = std::string;
  using ValueType = BaseMetadataValue;
  using ValuePtr = std::shared_ptr<ValueType>;
  using Iterator = typename std::map<KeyType, ValuePtr>::iterator;
  using ConstIterator = std::map<KeyType, ValuePtr>::const_iterator;

  /**
   * @brief Default constructor.
   */
  Metadata();

  /**
   * @brief Copy constructor.
   * @param other The Metadata to copy from
   */
  Metadata(const Metadata& other);

  /**
   * @brief Move constructor.
   * @param other The Metadata to move from
   */
  Metadata(Metadata&& other);

  /**
   * @brief Copy assignment operator.
   * @param rhs The Metadata to copy from
   * @return Reference to this Metadata after assignment
   */
  Metadata& operator=(const Metadata& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs The Metadata to move from
   * @return Reference to this Metadata after assignment
   */
  Metadata& operator=(Metadata&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~Metadata() noexcept;

  /**
   * @brief Returns true if there are no metadata values stored.
   * Returns false otherwise
   * @return bool is empty
   */
  bool isEmpty() const;

  /**
   * @brief Checks if metadata exists for the specified key.
   * @param key The key to check for
   * @return True if the key exists in the metadata, false otherwise
   */
  bool contains(const KeyType& key) const;

  /**
   * @brief Returns the ValuePtr for the target key. Returns nullptr
   * if the key does not exist in the Metadata.
   * @param key The key to retrieve data for
   * @return ValuePtr containing the metadata value, or nullptr if key doesn't exist
   */
  const ValuePtr& getDataValuePtr(const KeyType& key) const;

  /**
   * @brief Returns the metadata value for the target key.
   * Throws if the key does not exist in the Metadata.
   * @param key The key to retrieve data for
   * @return metadata value for key of type T
   */
  template <typename T>
  std::shared_ptr<T> getDataValuePtrAs(const KeyType& key) const
  {
    ValuePtr dataPtr = getDataValuePtr(key);
    if(const auto typedDataPtr = std::static_pointer_cast<T>(dataPtr); typedDataPtr != nullptr)
    {
      return typedDataPtr;
    }

    std::string errorStr = fmt::format("Metadata '{}' does not exist or cannot be cast to type '{}'", key, typeid(T).name());
    throw std::runtime_error(errorStr);
  }

  /**
   * @brief Adds or assigns the specified value for the target key.
   * @param key The key to set data for
   * @param value The value to associate with the key
   */
  void setDataValuePtr(const KeyType& key, const ValuePtr& value);

  /**
   * @brief Sets or creates a value with the specified key.
   * @param key Name of the stored value.
   * @param value Value to store
   */
  template <typename T>
  void setData(const KeyType& key, const typename T::ValueType& value)
  {
    auto dataPtr = std::make_shared<T>();
    *dataPtr.get() = value;
    setDataValuePtr(key, dataPtr);
  }

  /**
   * @brief Returns the value stored in the metadata value specified by the given key.
   * @param key Name of the stored value to lookup.
   * @return T Stored value
   */
  template <typename T>
  T getDataAs(const KeyType& key) const
  {
    const auto& dataValue = getDataValuePtrAs<AbstractMetadataValue<T>>(key);
    return dataValue->getValue();
  }

  /**
   * @brief Clears the metadata with the specified key. Does nothing if the key
   * has no data assigned to it.
   * @param key The key to remove from metadata
   */
  void remove(const KeyType& key);

  /**
   * @brief Clears all metadata.
   */
  void clear();

  /**
   * @brief Returns an iterator to the beginning of the Metadata collection.
   * @return Iterator to the beginning
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the Metadata collection.
   * @return Iterator to the end
   */
  Iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the Metadata collection.
   * @return ConstIterator to the beginning
   */
  ConstIterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the Metadata collection.
   * @return ConstIterator to the end
   */
  ConstIterator end() const;

  nlohmann::json toJson() const;

  void fromJson(const std::string& json);

private:
  std::map<KeyType, ValuePtr> m_Map;
};
} // namespace nx::core
