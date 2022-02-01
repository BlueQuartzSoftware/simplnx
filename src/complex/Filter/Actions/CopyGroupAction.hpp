#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for copying a BaseGroup in a DataStructure
 */
class COMPLEX_EXPORT CopyGroupAction : public IDataAction
{
public:
  CopyGroupAction() = delete;

  CopyGroupAction(const DataPath& path, const DataPath& newPath);

  ~CopyGroupAction() noexcept override;

  CopyGroupAction(const CopyGroupAction&) = delete;
  CopyGroupAction(CopyGroupAction&&) noexcept = delete;
  CopyGroupAction& operator=(const CopyGroupAction&) = delete;
  CopyGroupAction& operator=(CopyGroupAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the BaseGroup to be copied.
   * @return const DataPath&
   */
  const DataPath& path() const;

  /**
   * @brief Returns the new DataPath of the BaseGroup to be created.
   * @return const DataPath&
   */
  const DataPath& newPath() const;

private:
  /**
   * @brief Recursively copies data
   * @param dataStructure
   * @param targetPath
   * @param copyPath
   * @return std::shared_ptr<DataObject>
   */
  std::shared_ptr<DataObject> copyData(DataStructure& dataStructure, const DataPath& targetPath, const DataPath& copyPath) const;

  ////////////
  // Variables
  DataPath m_Path;
  DataPath m_NewPath;
};
} // namespace complex
