#pragma once

#include <optional>
#include <vector>

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"

namespace complex
{
/**
 * @brief Action for importing DataObjects from an HDF5 file.
 */
class COMPLEX_EXPORT ImportH5ObjectPathsAction : public IDataAction
{
public:
  using PathsType = std::optional<std::vector<DataPath>>;

  ImportH5ObjectPathsAction() = delete;

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

private:
  std::filesystem::path m_H5FilePath;
  PathsType m_Paths;
};
} // namespace complex
