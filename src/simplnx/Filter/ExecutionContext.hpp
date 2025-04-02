#pragma once

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <utility>

namespace nx::core
{
/**
 * @brief This class holds can help filters determine the correct absolute file path
 * to a file or folder on disk.
 */
class ExecutionContext
{
public:
  using FunctionType = std::function<std::filesystem::path(const std::filesystem::path&)>;

  ExecutionContext()
  : m_PathFunc([](const std::filesystem::path& path) { return path; })
  {
  }

  ExecutionContext(FunctionType func)
  : m_PathFunc(std::move(func))
  {
  }

  ExecutionContext(std::filesystem::path path)
  {
    if(path.is_relative())
    {
      throw std::runtime_error("ExecutionContext(): Path cannot be relative");
    }
    m_PathFunc = [relativeRoot = std::move(path)](const std::filesystem::path& inputPath) { return relativeRoot / inputPath; };
  }

  std::filesystem::path getAbsolutePath(const std::filesystem::path& path) const
  {
    if(path.is_absolute())
    {
      return path;
    }
    return m_PathFunc(path);
  }

private:
  FunctionType m_PathFunc;
};
} // namespace nx::core
