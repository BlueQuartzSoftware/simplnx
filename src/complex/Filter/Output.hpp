#pragma once

#include <memory>
#include <vector>

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
/**
 * @brief IDataAction is the interface for declaratively stating what changes to be made to a DataStructure.
 * Can then be applied in preflight or execute mode to actually make those changes.
 */
class IDataAction
{
public:
  using UniquePointer = std::unique_ptr<IDataAction>;

  enum class Mode : uint8
  {
    Preflight = 0,
    Execute
  };

  virtual ~IDataAction() noexcept = default;

  IDataAction(const IDataAction&) = delete;
  IDataAction(IDataAction&&) noexcept = delete;
  IDataAction& operator=(const IDataAction&) = delete;
  IDataAction& operator=(IDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  virtual Result<> apply(DataStructure& dataStructure, Mode mode) const = 0;

protected:
  IDataAction() = default;
};

/**
 * @brief Container for IDataActions
 */
struct OutputActions
{
  std::vector<IDataAction::UniquePointer> actions;
};
} // namespace complex
