#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class SIMPLNX_EXPORT EmptyAction : public IDataAction
{
public:
  EmptyAction() = default;

  ~EmptyAction() noexcept override = default;

  EmptyAction(const EmptyAction&) = delete;
  EmptyAction(EmptyAction&&) noexcept = delete;
  EmptyAction& operator=(const EmptyAction&) = delete;
  EmptyAction& operator=(EmptyAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override
  {
    return {};
  }

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override
  {
    return std::make_unique<EmptyAction>();
  }
};
} // namespace nx::core
