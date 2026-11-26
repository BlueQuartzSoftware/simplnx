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
   * @param paths The paths to import. An empty list imports no objects.
   * @param useDefaultGeometryValues Use 0 for invalid stored image origin components and 1 for invalid spacing components.
   */
  ImportH5ObjectPathsAction(std::filesystem::path importFile, PathsType paths, bool useDefaultGeometryValues = false);

  ~ImportH5ObjectPathsAction() noexcept override;

  ImportH5ObjectPathsAction(const ImportH5ObjectPathsAction&) = delete;
  ImportH5ObjectPathsAction(ImportH5ObjectPathsAction&&) noexcept = delete;
  ImportH5ObjectPathsAction& operator=(const ImportH5ObjectPathsAction&) = delete;
  ImportH5ObjectPathsAction& operator=(ImportH5ObjectPathsAction&&) noexcept = delete;

  /**
   * @brief Plans selected metadata, materializes requested values, and publishes completed objects.
   * Metadata validation errors leave the destination unchanged.
   * @param dataStructure The DataStructure to modify
   * @param mode The mode (Preflight or Execute)
   * @return Success, with warning 502693 for each selected image geometry that uses default values.
   * With defaults disabled, invalid stored image values return -1557 for origin or -1556 for spacing.
   * Invalid rectilinear bounds propagate errors -4006, -4048, -4008, or -4007. Store read errors also propagate.
   * Publication errors identify retained paths when exact rollback cannot complete.
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
  bool m_UseDefaultGeometryValues = false;
  mutable std::shared_ptr<TestState> m_TestState;
};
} // namespace nx::core
