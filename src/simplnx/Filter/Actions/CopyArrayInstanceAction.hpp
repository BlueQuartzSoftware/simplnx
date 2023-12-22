#pragma once

#include "simplnx/Filter/Output.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @brief Action that will copy a Source DataArray to a Destination DataPath.
 */
class SIMPLNX_EXPORT CopyArrayInstanceAction : public IDataCreationAction
{
public:
  CopyArrayInstanceAction() = delete;

  CopyArrayInstanceAction(const DataPath& selectedDataPath, const DataPath& createdDataPath);

  ~CopyArrayInstanceAction() noexcept override;

  CopyArrayInstanceAction(const CopyArrayInstanceAction&) = delete;
  CopyArrayInstanceAction(CopyArrayInstanceAction&&) noexcept = delete;
  CopyArrayInstanceAction& operator=(const CopyArrayInstanceAction&) = delete;
  CopyArrayInstanceAction& operator=(CopyArrayInstanceAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return
   */
  UniquePointer clone() const override;

  /**
   * @brief The selecte data array path to be copied
   * @return DataPath
   */
  DataPath selectedDataPath() const;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return DataPath
   */
  DataPath createdDataPath() const;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

private:
  DataPath m_SelectedDataPath;
};
} // namespace nx::core
