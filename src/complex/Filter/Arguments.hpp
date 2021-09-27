#pragma once

#include <any>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Arguments stores a map of strings to std::any. Meant for passing values to IFilter when executing.
 */
class COMPLEX_EXPORT Arguments
{
public:
  Arguments() = default;
  ~Arguments() noexcept = default;

  Arguments(const Arguments&) = default;
  Arguments(Arguments&&) noexcept = default;

  Arguments& operator=(const Arguments&) = default;
  Arguments& operator=(Arguments&&) noexcept = default;

  /**
   * @brief Insert the given key value pair.
   * @param key
   * @param value
   */
  void insert(const std::string& key, const std::any& value);

  /**
   * @brief Insert the given key value pair.
   * @param key
   * @param value
   */
  void insert(const std::string& key, std::any&& value);

  /**
   * @brief Returns a const reference to the value at the given key.
   * @param key
   * @return
   */
  [[nodiscard]] const std::any& at(const std::string& key) const;

  /**
   * @brief Returns a const reference to the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  [[nodiscard]] const T& valueRef(const std::string& key) const
  {
    const T* value = std::any_cast<T>(&m_Args.at(key));
    if(value == nullptr)
    {
      throw std::bad_any_cast();
    }
    return *value;
  }

  /**
   * @brief Returns a copy of the value at the given key cast to T.
   * Throws if T doesn't match the contained type.
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  [[nodiscard]] T value(const std::string& key) const
  {
    return std::any_cast<T>(m_Args.at(key));
  }

  /**
   * @brief Returns a reference to the value at the given key cast to T if the value is a std::reference_wrapper<T>.
   * Throws if T doesn't match the contained type.
   * @tparam T
   * @param key
   * @return
   */
  template <class T>
  [[nodiscard]] T& ref(const std::string& key) const
  {
    return std::any_cast<std::reference_wrapper<T>>(m_Args.at(key)).get();
  }

  /**
   * @brief Returns the size of the map of arguments.
   * @return
   */
  [[nodiscard]] usize size() const;

  /**
   * @brief Returns true if empty, false otherwise.
   * @return
   */
  [[nodiscard]] bool empty() const;

  /**
   * @brief Returns true if this contains the given key, false otherwise.
   * @param key
   * @return
   */
  [[nodiscard]] bool contains(const std::string& key) const;

  [[nodiscard]] auto begin()
  {
    return m_Args.begin();
  }

  [[nodiscard]] auto begin() const
  {
    return m_Args.begin();
  }

  [[nodiscard]] auto end()
  {
    return m_Args.end();
  }

  [[nodiscard]] auto end() const
  {
    return m_Args.end();
  }

private:
  std::map<std::string, std::any> m_Args;
};
} // namespace complex
