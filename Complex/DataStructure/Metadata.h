#pragma once

#include <any>
#include <map>
#include <string>

namespace SIMPL
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

  // Constructors/Destructors
  //

  /**
   * @brief
   */
  Metadata();

  /**
   * @brief
   * @param other
   */
  Metadata(const Metadata& other);

  /**
   * @brief
   * @param other
   */
  Metadata(Metadata&& other) noexcept;

  /**
   * Empty Destructor
   */
  virtual ~Metadata();

  /**
   * @brief
   * @param key
   * @return ValueType
   */
  ValueType getData(const KeyType& key) const;

  /**
   * @brief
   * @param key
   * @param value
   */
  void setData(const KeyType& key, const ValueType& value);

  /**
   * @brief
   * @param key
   */
  void remove(const KeyType& key);

  /**
   * @brief
   */
  void clear();

  /**
   * @brief
   * @param key
   * @return
   */
  ValueType& operator[](const KeyType& key);

  /**
   * @brief
   * @return iterator
   */
  Iterator begin();

  /**
   * @brief
   * @return iterator
   */
  Iterator end();

  /**
   * @brief
   * @return iterator
   */
  ConstIterator begin() const;

  /**
   * @brief
   * @return iterator
   */
  ConstIterator end() const;

  /**
   * @brief
   * @param rhs
   * @return
   */
  Metadata& operator=(const Metadata& rhs);

  /**
   * @brief
   * @param rhs
   * @return
   */
  Metadata& operator=(Metadata&& rhs) noexcept;

protected:
private:
  std::map<KeyType, ValueType> m_Map;
};
} // namespace SIMPL
