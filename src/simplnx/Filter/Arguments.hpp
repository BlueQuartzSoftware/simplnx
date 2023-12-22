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
   * @param key
   * @param value
   * @return bool if the insert worked.
   */
  bool insert(std::string key, std::any value);

  /**
   * @brief Insert or assign if already present the given key value pair.
   * @param key
   * @param value
   */
  void insertOrAssign(const std::string& key, std::any value);

  /**
   * @brief Insert or assign if already present the given key value pair.
   * @param key
   * @param value
   */
  void insertOrAssign(std::string&& key, std::any value);

  /**
   * @brief Returns a const reference to the value at the given key.
   * @param key
   * @return
   */
  const std::any& at(std::string_view key) const;

  /**
   * @brief Returns a const reference to the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  const T& valueRef(std::string_view key) const
  {
    return GetAnyRef<T>(at(key));
  }

  /**
   * @brief Returns a copy of the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  T value(std::string_view key) const
  {
    return std::any_cast<T>(at(key));
  }

  /**
   * @brief Returns the value or a default value
   * @tparam T
   * @param key
   * @param defaultValue
   * @return
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
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  T& ref(std::string_view key) const
  {
    return std::any_cast<std::reference_wrapper<T>>(at(key)).get();
  }

  /**
   * @brief Returns the size of the map of arguments.
   * @return
   */
  usize size() const;

  /**
   * @brief Returns true if empty, false otherwise.
   * @return
   */
  bool empty() const;

  /**
   * @brief Returns true if this contains the given key, false otherwise.
   * @param key
   * @return
   */
  bool contains(std::string_view key) const;

  auto begin()
  {
    return m_Args.begin();
  }

  auto begin() const
  {
    return m_Args.begin();
  }

  auto end()
  {
    return m_Args.end();
  }

  auto end() const
  {
    return m_Args.end();
  }

private:
  std::map<std::string, std::any, std::less<>> m_Args;
};
} // namespace nx::core
