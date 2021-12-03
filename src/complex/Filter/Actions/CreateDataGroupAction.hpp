#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @brief Action to create a DataGroup with the DataStructure
 */
class COMPLEX_EXPORT CreateDataGroupAction : public IDataAction
{
public:
  CreateDataGroupAction() = delete;

  CreateDataGroupAction(const DataPath& path);

  ~CreateDataGroupAction() noexcept override;

  CreateDataGroupAction(const CreateDataGroupAction&) = delete;
  CreateDataGroupAction(CreateDataGroupAction&&) noexcept = delete;
  CreateDataGroupAction& operator=(const CreateDataGroupAction&) = delete;
  CreateDataGroupAction& operator=(CreateDataGroupAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the DataGroup to be created.
   * @return
   */
  [[nodiscard]] DataPath parentPath() const;

private:
  DataPath m_Path = {};
};
} // namespace complex
