#pragma once

#include <map>
#include <memory>
#include <string>

#include "complex/Common/Types.hpp"
#include "complex/Filter/IParameter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT Parameters
{
public:
  Parameters() = default;
  ~Parameters() noexcept = default;

  Parameters(const Parameters&) = delete;
  Parameters(Parameters&&) noexcept = default;

  Parameters& operator=(const Parameters&) = delete;
  Parameters& operator=(Parameters&&) noexcept = default;

  bool contains(const std::string& name) const
  {
    return m_Params.count(name) > 0;
  }

  usize size() const
  {
    return m_Params.size();
  }

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

  void insert(std::unique_ptr<IParameter> parameter);

private:
  std::map<std::string, std::unique_ptr<IParameter>> m_Params;
};
} // namespace complex
