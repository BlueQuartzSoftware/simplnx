#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/AnyCloneable.hpp"
#include "complex/complex_export.hpp"

#include <nonstd/span.hpp>

#include <memory>
#include <vector>

namespace complex
{
/**
 * @brief IDataAction is the interface for declaratively stating what changes to be made to a DataStructure.
 * Can then be applied in preflight or execute mode to actually make those changes.
 */
class COMPLEX_EXPORT IDataAction
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

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  virtual UniquePointer clone() const = 0;

protected:
  IDataAction() = default;
};

using AnyDataAction = AnyCloneable<IDataAction>;

/**
 * @brief The IDataCreationAction class is a subclass of IDataAction used for
 * inserting a DataObject to a target DataStructure.
 */
class COMPLEX_EXPORT IDataCreationAction : public IDataAction
{
public:
  enum class ArrayHandlingType : uint64
  {
    Copy = 0,      // Copy an existing array to the constructed object
    Move = 1,      // Move an existing array to the constructed object
    Reference = 2, // Reference an existing array
    Create = 3     // Allocate a new array
  };

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
   * @brief Returns the DataPath of the top level object to be created.
   * @return DataPath
   */
  DataPath getCreatedPath() const
  {
    return m_CreatedPath;
  }

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  virtual std::vector<DataPath> getAllCreatedPaths() const = 0;

protected:
  IDataCreationAction() = default;

private:
  DataPath m_CreatedPath;
};

/**
 * @brief Container for IDataActions
 */
struct COMPLEX_EXPORT OutputActions
{
  std::vector<AnyDataAction> actions;
  std::vector<AnyDataAction> deferredActions;

  OutputActions() = default;

  ~OutputActions() noexcept = default;

  OutputActions(const OutputActions&) = default;
  OutputActions(OutputActions&&) noexcept = default;

  OutputActions& operator=(const OutputActions&) = default;
  OutputActions& operator=(OutputActions&&) noexcept = default;

  template <class T>
  void appendAction(std::unique_ptr<T> action)
  {
    static_assert(std::is_base_of_v<IDataAction, T>, "OutputActions::appendAction requires T to be derived from IDataAction");
    actions.emplace_back(std::move(action));
  }

  template <class T>
  void appendDeferredAction(std::unique_ptr<T> action)
  {
    static_assert(std::is_base_of_v<IDataAction, T>, "OutputActions::appendDeferredAction requires T to be derived from IDataAction");
    deferredActions.emplace_back(std::move(action));
  }

  static Result<> ApplyActions(nonstd::span<const AnyDataAction> actions, DataStructure& dataStructure, IDataAction::Mode mode);

  Result<> applyRegular(DataStructure& dataStructure, IDataAction::Mode mode) const;

  Result<> applyDeferred(DataStructure& dataStructure, IDataAction::Mode mode) const;

  Result<> applyAll(DataStructure& dataStructure, IDataAction::Mode mode) const;
};
} // namespace complex
