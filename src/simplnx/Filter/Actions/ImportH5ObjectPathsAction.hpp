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
   * @enum PublicationFault
   * @brief Selects one failure in this action's next publication transaction.
   */
  enum class PublicationFault : uint8
  {
    None,                  ///< Runs without injection.
    BeforePublication,     ///< Fails after ID reservation and before insertion.
    AfterSecondInsert,     ///< Fails after two complete real insertions.
    AfterHierarchy,        ///< Fails after the second object's hierarchy placement, before its parent list.
    BeforeTracking,        ///< Fails after the second object's parent list, before its weak entry.
    AllocationAfterSecond, ///< Throws bad_alloc after two complete insertions.
    ReturnFalse            ///< Makes the second insertion report failure before mutation.
  };

  /**
   * @brief Arms one action-local publication fault.
   * @param fault Selects the failure boundary.
   * @param refuseCleanup Retains the second object's exact placement during automatic cleanup.
   */
  void setPublicationFaultForTesting(PublicationFault fault, bool refuseCleanup = false) const;
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
  /**
   * @brief Cleans structures before attempting callbacks and conditionally restores the reserved ID counter.
   * @param destination Owns the publication target.
   * @param records Owns the exact shells and preallocated removal messages.
   * @param savedNextId Supplies the counter before reservation.
   * @param reservedNextId Supplies the transaction's checked high-water counter.
   * @param allowRefusal Enables only this action's deterministic test refusal.
   * @return True if every transaction object and index entry was removed.
   */
  bool rollbackPublication(DataStructure& destination, std::vector<DataStructure::ImportPublicationRecord>& records, DataObject::IdType savedNextId, DataObject::IdType reservedNextId,
                           bool allowRefusal) const;

  std::filesystem::path m_H5FilePath;
  PathsType m_Paths;
  mutable std::shared_ptr<TestState> m_TestState;
};
} // namespace nx::core
