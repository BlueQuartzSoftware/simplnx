#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/AnyCloneable.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nonstd/span.hpp>

#include <memory>
#include <vector>

namespace nx::core
{
/**
 * @brief IDataAction is the interface for declaratively stating what changes to be made to a DataStructure.
 * Can then be applied in preflight or execute mode to actually make those changes.
 */
class SIMPLNX_EXPORT IDataAction
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
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  virtual Result<> apply(DataStructure& dataStructure, Mode mode) const = 0;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  virtual UniquePointer clone() const = 0;

protected:
  /**
   * @brief Protected default constructor.
   */
  IDataAction() = default;
};

using AnyDataAction = AnyCloneable<IDataAction>;

/**
 * @brief The IDataCreationAction class is a subclass of IDataAction used for
 * inserting a DataObject to a target DataStructure.
 */
class SIMPLNX_EXPORT IDataCreationAction : public IDataAction
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
  /**
   * @brief Protected default constructor.
   */
  IDataCreationAction() = default;

private:
  DataPath m_CreatedPath;
};

/**
 * @brief Represents a modification to a DataObject.
 */
struct SIMPLNX_EXPORT DataObjectModification
{
  /**
   * @brief Type of modification made to the DataObject.
   */
  enum class ModifiedType : uint64
  {
    Modified = 0,
    PossiblyDeleted = 1,
    Unknown = 2,
  };

  DataPath modifiedPath;
  ModifiedType modifiedType = ModifiedType::Unknown;
  DataObject::Type dataObjectType = DataObject::Type::Unknown;
};

/**
 * @brief Container for IDataActions
 */
struct SIMPLNX_EXPORT OutputActions
{
  std::vector<AnyDataAction> actions = {};
  std::vector<AnyDataAction> deferredActions = {};
  std::vector<DataObjectModification> modifiedActions = {};

  OutputActions() = default;

  ~OutputActions() noexcept = default;

  OutputActions(const OutputActions&) = default;
  OutputActions(OutputActions&&) noexcept = default;

  OutputActions& operator=(const OutputActions&) = default;
  OutputActions& operator=(OutputActions&&) noexcept = default;

  /**
   * @brief Appends an action to the regular actions list.
   * @tparam T The action type (must derive from IDataAction)
   * @param action The action to append
   */
  template <class T>
  void appendAction(std::unique_ptr<T> action)
  {
    static_assert(std::is_base_of_v<IDataAction, T>, "OutputActions::appendAction requires T to be derived from IDataAction");
    actions.emplace_back(std::move(action));
  }

  /**
   * @brief Appends an action to the deferred actions list.
   * @tparam T The action type (must derive from IDataAction)
   * @param action The action to append
   */
  template <class T>
  void appendDeferredAction(std::unique_ptr<T> action)
  {
    static_assert(std::is_base_of_v<IDataAction, T>, "OutputActions::appendDeferredAction requires T to be derived from IDataAction");
    deferredActions.emplace_back(std::move(action));
  }

  /**
   * @brief Appends a data object modification notification.
   * @param dataPath The path to the modified data object
   * @param modifiedType The type of modification
   */
  void appendDataObjectModificationNotification(const DataPath& dataPath, DataObjectModification::ModifiedType modifiedType)
  {
    modifiedActions.push_back(DataObjectModification{dataPath, modifiedType});
  }

  /**
   * @brief Applies a span of actions to a DataStructure.
   * @param actions The actions to apply
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  static Result<> ApplyActions(nonstd::span<const AnyDataAction> actions, DataStructure& dataStructure, IDataAction::Mode mode);

  /**
   * @brief Applies regular actions to a DataStructure.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> applyRegular(DataStructure& dataStructure, IDataAction::Mode mode) const;

  /**
   * @brief Applies deferred actions to a DataStructure.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> applyDeferred(DataStructure& dataStructure, IDataAction::Mode mode) const;

  /**
   * @brief Applies all actions (regular and deferred) to a DataStructure.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> applyAll(DataStructure& dataStructure, IDataAction::Mode mode) const;
};

/**
 * @brief Merges two OutputActions into one.
 * @param first OutputActions
 * @param second OutputActions
 * @return OutputActions a new OutputActions created from the two input OutputActions
 */
inline OutputActions MergeOutputActions(OutputActions first, OutputActions second)
{
  OutputActions outputActions = {};

  outputActions.actions.reserve(first.actions.size() + second.actions.size());
  for(auto&& action : first.actions)
  {
    outputActions.actions.push_back(std::move(action));
  }
  for(auto&& action : second.actions)
  {
    outputActions.actions.push_back(std::move(action));
  }

  outputActions.deferredActions.reserve(first.deferredActions.size() + second.deferredActions.size());
  for(auto&& deferredAction : first.deferredActions)
  {
    outputActions.deferredActions.push_back(std::move(deferredAction));
  }
  for(auto&& deferredAction : second.deferredActions)
  {
    outputActions.deferredActions.push_back(std::move(deferredAction));
  }

  return outputActions;
}

/**
 * @brief Merges two Result<OutputActions> into one.
 * @param first Result<OutputActions>
 * @param second Result<OutputActions>
 * @return Result<OutputActions> a new result created from the two input results
 */
inline Result<OutputActions> MergeOutputActionResults(Result<OutputActions> first, Result<OutputActions> second)
{
  Result<OutputActions> result = MergeResults(first, second);
  result.value() = MergeOutputActions(first.value(), second.value());
  return result;
}
} // namespace nx::core
