#include "Arguments.hpp"

namespace complex
{
void Arguments::insert(const std::string& key, const std::any& value)
{
  m_Args.insert({key, value});
}

void Arguments::insert(const std::string& key, std::any&& value)
{
  m_Args.insert({key, value});
}

const std::any& Arguments::at(const std::string& key) const
{
  return m_Args.at(key);
}

usize Arguments::size() const
{
  return m_Args.size();
}

bool Arguments::empty() const
{
  return m_Args.empty();
}

bool Arguments::contains(const std::string& key) const
{
  return m_Args.count(key) > 0;
}
} // namespace complex
