#pragma once

#include <string>

namespace nx::core
{
class Pipeline;

/**
 * @brief Pure interface for JSON pipeline parsing.
 */
class IJsonPipelineParser
{
public:
  virtual ~IJsonPipelineParser() = default;

  IJsonPipelineParser& operator=(const IJsonPipelineParser&) = delete;
  IJsonPipelineParser& operator=(IJsonPipelineParser&&) = delete;

  /**
   * @param json
   * @return Pipeline*
   */
  virtual Pipeline* fromJson(const std::string& json) const = 0;

  /**
   * @param pipeline
   * @return std::string
   */
  virtual std::string toJson(Pipeline* pipeline) const = 0;

protected:
  IJsonPipelineParser() = default;
  IJsonPipelineParser(const IJsonPipelineParser&) = default;
  IJsonPipelineParser(IJsonPipelineParser&&) = default;
};
} // namespace nx::core
