#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for renaming DataObjects in a DataStructure
 */
class COMPLEX_EXPORT RenameDataAction : public IDataAction
{
public:
  RenameDataAction() = delete;

  RenameDataAction(const DataPath& path, const std::string& newName);

  ~RenameDataAction() noexcept override;

  RenameDataAction(const RenameDataAction&) = delete;
  RenameDataAction(RenameDataAction&&) noexcept = delete;
  RenameDataAction& operator=(const RenameDataAction&) = delete;
  RenameDataAction& operator=(RenameDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns the target name.
   * @return std::string
   */
  std::string newName() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return
   */
  const DataPath& path() const;

private:
  std::string m_NewName;
  DataPath m_Path;
};
} // namespace complex
