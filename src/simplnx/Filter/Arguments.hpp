#pragma once

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/simplnx_export.hpp"

#include <any>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace nx::core
{
/**
 * @brief Arguments stores a map of strings to std::any. Meant for passing values to IFilter when executing.
 */
class SIMPLNX_EXPORT Arguments
{
public:
  Arguments() = default;
  ~Arguments() noexcept = default;

  Arguments(const Arguments&) = default;
  Arguments(Arguments&&) noexcept = default;

  Arguments& operator=(const Arguments&) = default;
  Arguments& operator=(Arguments&&) noexcept = default;

  /**
   * @brief Insert the given key value pair IF and ONLY IF there isn't a key already in the underlying map.
   * If you want to overwrite the current value then use  insertOrAssign() function
   * @param key The key to insert
   * @param value The value to associate with the key
   * @return bool True if the insert succeeded, false if key already exists
   */
  bool insert(std::string key, std::any value);

  /**
   * @brief Insert or assign if already present the given key value pair.
   * @param key The key to insert or update
   * @param value The value to associate with the key
   */
  void insertOrAssign(const std::string& key, std::any value);

  /**
   * @brief Insert or assign if already present the given key value pair.
   * @param key The key to insert or update (moved)
   * @param value The value to associate with the key
   */
  void insertOrAssign(std::string&& key, std::any value);

  /**
   * @brief Returns a const reference to the value at the given key.
   * @param key The key to look up
   * @return const std::any& Reference to the value at the key
   */
  const std::any& at(std::string_view key) const;

  /**
   * @brief Returns a const reference to the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T The type to cast to
   * @param key The key to look up
   * @return const T& Reference to the value cast to type T
   */
  template <class T>
  const T& valueRef(std::string_view key) const
  {
    return GetAnyRef<T>(at(key));
  }

  /**
   * @brief Returns a copy of the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T The type to cast to
   * @param key The key to look up
   * @return T Copy of the value cast to type T
   */
  template <class T>
  T value(std::string_view key) const
  {
    return std::any_cast<T>(at(key));
  }

  /**
   * @brief Returns the value or a default value
   * @tparam T The type to cast to
   * @param key The key to look up
   * @param defaultValue The default value to return if key not found
   * @return T The value at key or the default value
   */
  template <class T>
  T valueOrDefault(std::string_view key, T defaultValue) const
  {
    if(contains(key))
    {
      return value<T>(key);
    }
    return defaultValue;
  }

  /**
   * @brief Returns a reference to the value at the given key cast to T if the value is a std::reference_wrapper<T>.
   * Throws if T doesn't match the contained type.
   * @tparam T The type to cast to
   * @param key The key to look up
   * @return T& Reference to the unwrapped value
   */
  template <class T>
  T& ref(std::string_view key) const
  {
    return std::any_cast<std::reference_wrapper<T>>(at(key)).get();
  }

  /**
   * @brief Returns the size of the map of arguments.
   * @return usize The number of key-value pairs
   */
  usize size() const;

  /**
   * @brief Returns true if empty, false otherwise.
   * @return bool True if the arguments map is empty
   */
  bool empty() const;

  /**
   * @brief Returns true if this contains the given key, false otherwise.
   * @param key The key to check
   * @return bool True if the key exists
   */
  bool contains(std::string_view key) const;

  /**
   * @brief Returns an iterator to the beginning of the arguments map.
   * @return auto Iterator to the first element
   */
  auto begin()
  {
    return m_Args.begin();
  }

  /**
   * @brief Returns a const iterator to the beginning of the arguments map.
   * @return auto Const iterator to the first element
   */
  auto begin() const
  {
    return m_Args.begin();
  }

  /**
   * @brief Returns an iterator to the end of the arguments map.
   * @return auto Iterator to past-the-end element
   */
  auto end()
  {
    return m_Args.end();
  }

  /**
   * @brief Returns a const iterator to the end of the arguments map.
   * @return auto Const iterator to past-the-end element
   */
  auto end() const
  {
    return m_Args.end();
  }

private:
  std::map<std::string, std::any, std::less<>> m_Args;
};
} // namespace nx::core
