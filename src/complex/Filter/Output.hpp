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
 * @brief The IDataCreationAction class is a subclass of IDataAction used for
 * inserting a DataObject to a target DataStructure.
 */
class IDataCreationAction : public IDataAction
{
public:
  /**
   * @brief Constructs an IDataCreationAction that takes a DataPath specifying the path to be created.
   * @param createdPath
   */
  IDataCreationAction(const DataPath& createdPath)
  : m_CreatedPath(createdPath)
  {
  }
  ~IDataCreationAction() noexcept override = default;

  IDataCreationAction(const IDataCreationAction&) = delete;
  IDataCreationAction(IDataCreationAction&&) noexcept = delete;
  IDataCreationAction& operator=(const IDataCreationAction&) = delete;
  IDataCreationAction& operator=(IDataCreationAction&&) noexcept = delete;

  /**
   * @brief Returns the DataPath to be created.
   * @return DataPath
   */
  DataPath getCreatedPath() const
  {
    return m_CreatedPath;
  }

private:
  DataPath m_CreatedPath;
};

/**
 * @brief Container for IDataActions
 */
struct OutputActions
{
  std::vector<IDataAction::UniquePointer> actions;
};
} // namespace complex
