#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "complex/Common/Types.hpp"
#include "complex/Filter/IParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Parameters stores a map of strings to IParameter.
 */
class COMPLEX_EXPORT Parameters
{
public:
  Parameters() = default;
  ~Parameters() noexcept = default;

  Parameters(const Parameters& rhs);
  Parameters(Parameters&&) noexcept = default;

  Parameters& operator=(const Parameters& rhs);
  Parameters& operator=(Parameters&&) noexcept = default;

  /**
   * @brief Returns true if contains a parameter with the given name.
   * @param name
   * @return
   */
  bool contains(std::string_view name) const;

  /**
   * @brief Returns the size of the collection.
   * @return
   */
  usize size() const;

  /**
   * @brief Returns true if empty, otherwise false.
   * @return
   */
  bool empty() const;

  /**
   * @brief Inserts the given parameter.
   * @param parameter
   */
  void insert(IParameter::UniquePointer parameter);

  auto begin()
  {
    return m_Params.begin();
  }

  auto begin() const
  {
    return m_Params.begin();
  }

  auto end()
  {
    return m_Params.end();
  }

  auto end() const
  {
    return m_Params.end();
  }

private:
  std::map<std::string, std::unique_ptr<IParameter>, std::less<>> m_Params;
};
} // namespace complex
