#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for renaming DataObjects in a DataStructure
 */
class COMPLEX_EXPORT MoveDataAction : public IDataAction
{
public:
  MoveDataAction() = delete;

  MoveDataAction(const DataPath& path, const DataPath& newParentPath);

  ~MoveDataAction() noexcept override;

  MoveDataAction(const MoveDataAction&) = delete;
  MoveDataAction(MoveDataAction&&) noexcept = delete;
  MoveDataAction& operator=(const MoveDataAction&) = delete;
  MoveDataAction& operator=(MoveDataAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the target name.
   * @return std::string
   */
  DataPath newParentPath() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return
   */
  const DataPath& path() const;

private:
  DataPath m_NewParentPath;
  DataPath m_Path;
};
} // namespace complex
