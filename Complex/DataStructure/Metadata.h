#pragma once

#include <any>
#include <map>
#include <string>

namespace Complex
{

/**
 * class Metadata
 *
 */

class Metadata
{
public:
  using KeyType = std::string;
  using ValueType = std::any;
  using Iterator = std::map<KeyType, ValueType>::iterator;
  using ConstIterator = std::map<KeyType, ValueType>::const_iterator;

  /**
   * @brief Constructs an empty Metadata.
   */
  Metadata();

  /**
   * @brief Copy constructor
   * @param other
   */
  Metadata(const Metadata& other);

  /**
   * @brief Move constructor
   * @param other
   */
  Metadata(Metadata&& other) noexcept;

  virtual ~Metadata();

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
   * @brief Removes the data with the specified key.
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

  /**
   * @brief Copy assignment operator.
   * @param rhs
   * @return Metadata&
   */
  Metadata& operator=(const Metadata& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs
   * @return Metadata&
   */
  Metadata& operator=(Metadata&& rhs) noexcept;

protected:
private:
  std::map<KeyType, ValueType> m_Map;
};
} // namespace Complex
