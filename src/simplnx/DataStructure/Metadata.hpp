#pragma once

#include "simplnx/DataStructure/Metadata/BaseMetadataValue.hpp"

#include "simplnx/simplnx_export.hpp"

#include <any>
#include <map>
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
  using Iterator = std::map<KeyType, ValueType>::iterator;
  using ConstIterator = std::map<KeyType, ValueType>::const_iterator;

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
   * @brief Checks if metadata exists for the specified key.
   * @param key The key to check for
   * @return True if the key exists in the metadata, false otherwise
   */
  bool contains(const KeyType& key) const;

  /**
   * @brief Returns the ValueType for the target key. Returns an empty std::any
   * if the key does not exist in the Metadata.
   * @param key The key to retrieve data for
   * @return ValueType containing the metadata value, or empty std::any if key doesn't exist
   */
  const ValueType& getData(const KeyType& key) const;

  /**
   * @brief Adds or assigns the specified value for the target key.
   * @param key The key to set data for
   * @param value The value to associate with the key
   */
  void setData(const KeyType& key, const ValueType& value);

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
   * @brief Returns a reference to the data with the target key.
   * Returns and adds an empty std::any if no data exists with the key value.
   * @param key The key to retrieve or create
   * @return Reference to the ValueType at the specified key
   */
  ValueType& operator[](const KeyType& key);

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

  std::string toJson() const;

  void fromJson(const std::string& json);

private:
  std::map<KeyType, ValueType> m_Map;
};
} // namespace nx::core
