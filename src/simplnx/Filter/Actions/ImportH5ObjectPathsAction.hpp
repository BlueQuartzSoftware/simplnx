#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
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
  using PathsType = std::vector<DataPath>;

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
   * @brief Plans selected metadata, materializes requested values, and publishes completed objects.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Import diagnostics, including retained paths when exact rollback cannot complete.
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns a copy of the action.
   * @return UniquePointer A unique pointer to the cloned action
   */
  UniquePointer clone() const override;

  /**
   * @brief Returns all of the DataPaths to be created.
   * @return std::vector<DataPath>
   */
  std::vector<DataPath> getAllCreatedPaths() const override;

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  /**
   * @brief Selects an action-local failure in the destination-owned publication operation.
   */
  using PublicationFaultType = DataStructure::ImportPublicationTestControl::Fault;

  /**
   * @brief Arms one action-local publication fault.
   * @param fault Selects the failure boundary.
   * @param refuseCleanup Retains the second object's exact placement during automatic cleanup.
   */
  void setPublicationFaultForTesting(PublicationFaultType fault, bool refuseCleanup = false) const;
  bool publicationFaultConsumedForTesting() const;
  bool cleanupFaultConsumedForTesting() const;
  std::shared_ptr<DataObject> retainedOwnerForTesting() const;

  /**
   * @brief Completes a deliberately refused rollback through the same private cleanup route.
   * @param destination Contains the retained transaction objects.
   * @return Cleanup diagnostics.
   */
  Result<> cleanupFailedImportForTesting(DataStructure& destination) const;
#endif

private:
  struct TestState;
  std::filesystem::path m_H5FilePath;
  PathsType m_Paths;
  mutable std::shared_ptr<TestState> m_TestState;
};
} // namespace nx::core
