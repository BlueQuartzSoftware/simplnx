#include "Arguments.hpp"

#include <fmt/core.h>

#include <stdexcept>

namespace nx::core
{
bool Arguments::insert(std::string key, std::any value)
{
  auto [iter, result] = m_Args.insert({std::move(key), std::move(value)});
  return result;
}

void Arguments::insertOrAssign(const std::string& key, std::any value)
{
  m_Args.insert_or_assign(key, std::move(value));
}

void Arguments::insertOrAssign(std::string&& key, std::any value)
{
  m_Args.insert_or_assign(std::move(key), std::move(value));
}

const std::any& Arguments::at(std::string_view key) const
{
  auto iter = m_Args.find(key);
  if(iter == m_Args.cend())
  {
    throw std::out_of_range(fmt::format("Key '{}' does not exist in Arguments", key));
  }
  return iter->second;
}

usize Arguments::size() const
{
  return m_Args.size();
}

bool Arguments::empty() const
{
  return m_Args.empty();
}

bool Arguments::contains(std::string_view key) const
{
  return m_Args.count(key) > 0;
}
} // namespace nx::core
