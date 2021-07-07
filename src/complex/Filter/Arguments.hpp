#pragma once

#include <any>
#include <map>
#include <string>
#include <type_traits>
#include <stdexcept>

#include "complex/Common/Types.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT Arguments
{
public:
  Arguments() = default;
  ~Arguments() noexcept = default;

  Arguments(const Arguments&) = default;
  Arguments(Arguments&&) noexcept = default;

  Arguments& operator=(const Arguments&) = default;
  Arguments& operator=(Arguments&&) noexcept = default;

  void insert(const std::string& key, const std::any& value);

  void insert(const std::string& key, std::any&& value);

  [[nodiscard]] const std::any& at(const std::string& key) const;

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

  template <class T>
  [[nodiscard]] T value(const std::string& key) const
  {
    return std::any_cast<T>(m_Args.at(key));
  }

  template <class T>
  [[nodiscard]] T& ref(const std::string& key) const
  {
    return std::any_cast<std::reference_wrapper<T>>(m_Args.at(key)).get();
  }

  [[nodiscard]] usize size() const;

  [[nodiscard]] bool empty() const;

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
