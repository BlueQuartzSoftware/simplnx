#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action for renaming DataObjects in a DataStructure
 */
class SIMPLNX_EXPORT MoveDataAction : public IDataAction
{
public:
  MoveDataAction() = delete;

  /**
   * @brief Constructs a MoveDataAction.
   * @param path The path to the DataObject to move
   * @param newParentPath The new parent path
   */
  MoveDataAction(const DataPath& path, const DataPath& newParentPath);

  ~MoveDataAction() noexcept override;

  MoveDataAction(const MoveDataAction&) = delete;
  MoveDataAction(MoveDataAction&&) noexcept = delete;
  MoveDataAction& operator=(const MoveDataAction&) = delete;
  MoveDataAction& operator=(MoveDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Result<> Result with any errors or warnings
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the target name.
   * @return std::string
   */
  DataPath newParentPath() const;

  /**
   * @brief Returns the path of the DataObject to be moved.
   * @return const DataPath& The object path
   */
  const DataPath& path() const;

private:
  DataPath m_NewParentPath;
  DataPath m_Path;
};
} // namespace nx::core
