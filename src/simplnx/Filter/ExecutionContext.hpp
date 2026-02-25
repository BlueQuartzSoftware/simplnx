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

  /**
   * @brief Default constructor that returns paths unchanged.
   */
  ExecutionContext()
  : m_PathFunc([](const std::filesystem::path& path) { return path; })
  {
  }

  /**
   * @brief Constructs with a custom path resolution function.
   * @param func The function to use for resolving paths
   */
  ExecutionContext(FunctionType func)
  : m_PathFunc(std::move(func))
  {
  }

  /**
   * @brief Constructs with an absolute base path for resolving relative paths.
   * @param path The absolute base path
   */
  ExecutionContext(std::filesystem::path path)
  {
    if(path.is_relative())
    {
      throw std::runtime_error("ExecutionContext(): Path cannot be relative");
    }
    m_PathFunc = [relativeRoot = std::move(path)](const std::filesystem::path& inputPath) { return relativeRoot / inputPath; };
  }

  /**
   * @brief Returns the absolute path for the given path.
   * If the path is already absolute, returns it unchanged.
   * If the path is relative, applies the configured resolution function.
   * @param path The path to resolve
   * @return std::filesystem::path The absolute path
   */
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
