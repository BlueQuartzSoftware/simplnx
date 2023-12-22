#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{

/**
 * @brief Action to create a DataGroup with the DataStructure
 */
class SIMPLNX_EXPORT CreateDataGroupAction : public IDataCreationAction
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
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
};
} // namespace nx::core
