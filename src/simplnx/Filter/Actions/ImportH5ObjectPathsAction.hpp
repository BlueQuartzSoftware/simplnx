#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Output.hpp"

#include <optional>
#include <vector>

namespace nx::core
{
/**
 * @brief Action for importing DataObjects from an HDF5 file.
 */
class SIMPLNX_EXPORT ImportH5ObjectPathsAction : public IDataCreationAction
{
public:
  using PathsType = std::optional<std::vector<DataPath>>;

  ImportH5ObjectPathsAction() = delete;

  /**
   * @brief Constructs the action
   * @param importFile The file to import data from
   * @param paths The vector of paths to import.
   *
   * <b>IMPORTANT NOTE</b>. If the std::optional<> paths argument does NOT have a value then
   * then entire file will be imported. If it has a value, but the std::vector<> has a size of
   * zero (0), then NOTHING will be imported.
   */
  ImportH5ObjectPathsAction(const std::filesystem::path& importFile, const PathsType& paths);

  ~ImportH5ObjectPathsAction() noexcept override;

  ImportH5ObjectPathsAction(const ImportH5ObjectPathsAction&) = delete;
  ImportH5ObjectPathsAction(ImportH5ObjectPathsAction&&) noexcept = delete;
  ImportH5ObjectPathsAction& operator=(const ImportH5ObjectPathsAction&) = delete;
  ImportH5ObjectPathsAction& operator=(ImportH5ObjectPathsAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @param mode
   * @return Result<>
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
  std::filesystem::path m_H5FilePath;
  PathsType m_Paths;
};
} // namespace nx::core
