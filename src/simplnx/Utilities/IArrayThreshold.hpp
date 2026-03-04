#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <set>

namespace nx::core
{

/**
 * @brief Pure interface for array threshold operations used in multi-threshold filtering.
 */
class SIMPLNX_EXPORT IArrayThreshold
{
public:
  enum class UnionOperator : uint8
  {
    And,
    Or
  };

  virtual ~IArrayThreshold() = default;

  IArrayThreshold& operator=(const IArrayThreshold&) = delete;
  IArrayThreshold& operator=(IArrayThreshold&&) = delete;

  /**
   * @brief Returns whether the threshold result should be inverted.
   * @return bool
   */
  [[nodiscard]] virtual bool isInverted() const = 0;

  /**
   * @brief Sets whether the threshold result should be inverted.
   * @param inverted
   */
  virtual void setInverted(bool inverted) = 0;

  /**
   * @brief Returns the union operator used to combine this threshold with others.
   * @return UnionOperator
   */
  [[nodiscard]] virtual UnionOperator getUnionOperator() const = 0;

  /**
   * @brief Sets the union operator used to combine this threshold with others.
   * @param unionType
   */
  virtual void setUnionOperator(UnionOperator unionType) = 0;

  /**
   * @brief Returns the set of DataPaths required by this threshold.
   * @return std::set<DataPath>
   */
  [[nodiscard]] virtual std::set<DataPath> getRequiredPaths() const = 0;

  /**
   * @brief Serializes this threshold to JSON.
   * @return nlohmann::json
   */
  [[nodiscard]] virtual nlohmann::json toJson() const = 0;

protected:
  IArrayThreshold() = default;
  IArrayThreshold(const IArrayThreshold&) = default;
  IArrayThreshold(IArrayThreshold&&) = default;
};
} // namespace nx::core
