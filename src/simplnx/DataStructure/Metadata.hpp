#pragma once

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
  using ValueType = std::any;
  using Iterator = std::map<KeyType, ValueType>::iterator;
  using ConstIterator = std::map<KeyType, ValueType>::const_iterator;

  /**
   * @brief Default constructor.
   */
  Metadata();

  /**
   * @brief Copy constructor.
   * @param rhs
   */
  Metadata(const Metadata& other);

  /**
   * @brief Move constructor.
   * @param rhs
   */
  Metadata(Metadata&& other);

  /**
   * @brief Copy assignment.
   * @param rhs
   * @return
   */
  Metadata& operator=(const Metadata& rhs);

  /**
   * @brief Move assignment.
   * @param rhs
   * @return
   */
  Metadata& operator=(Metadata&& rhs) noexcept;

  /**
   * @brief Destructor.
   */
  ~Metadata() noexcept;

  bool contains(const KeyType& key) const;

  /**
   * @brief Returns the ValueType for the target key. Returns an empty std::any
   * if the key does not exist in the Metadata.
   * @param key
   * @return ValueType
   */
  ValueType getData(const KeyType& key) const;

  /**
   * @brief Adds or assigns the specified value for the target key.
   * @param key
   * @param value
   */
  void setData(const KeyType& key, const ValueType& value);

  /**
   * @brief Clears the metadata with the specified key. Does nothing if the key
   * has no data assigned to it.
   * @param key
   */
  void remove(const KeyType& key);

  /**
   * @brief Clears all metadata.
   */
  void clear();

  /**
   * @brief Returns a reference to the data with the target key.
   * Returns and adds an empty std::any if no data exists with the key value.
   * @param key
   * @return ValueType&
   */
  ValueType& operator[](const KeyType& key);

  /**
   * @brief Returns an iterator to the beginning of the Metadata collection.
   * @return Iterator
   */
  Iterator begin();

  /**
   * @brief Returns an iterator to the end of the Metadata collection.
   * @return Iterator
   */
  Iterator end();

  /**
   * @brief Returns an iterator to the beginning of the Metadata collection.
   * @return ConstIterator
   */
  ConstIterator begin() const;

  /**
   * @brief Returns an iterator to the end of the Metadata collection.
   * @return ConstIterator
   */
  ConstIterator end() const;

private:
  std::map<KeyType, ValueType> m_Map;
};
} // namespace nx::core
