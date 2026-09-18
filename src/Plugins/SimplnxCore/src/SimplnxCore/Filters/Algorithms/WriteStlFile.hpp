#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <atomic>

namespace nx::core
{

/**
 * @enum GroupingType
 * @brief Selects how triangles map to output STL files.
 */
enum class GroupingType : ChoicesParameter::ValueType
{
  Features,          ///< Creates one file for each referenced feature.
  FeaturesAndPhases, ///< Adds phase and feature IDs to each output name.
  SingleFile,        ///< Writes all triangles to one file sequence.
  PartNumber         ///< Creates one file for each referenced part number.
};

/**
 * @struct WriteStlFileInputValues
 * @brief Stores grouping choices, paths, names, and output limits.
 *
 * HIDDEN_MaxTrianglesPerFile lowers the overflow threshold for tests. Production
 * uses the maximum int32 triangle count.
 */
struct SIMPLNXCORE_EXPORT WriteStlFileInputValues
{
  ChoicesParameter::ValueType GroupingType;
  FileSystemPathParameter::ValueType OutputStlFile;
  FileSystemPathParameter::ValueType OutputStlDirectory;
  StringParameter::ValueType OutputStlPrefix;
  DataPath FeatureIdsPath;
  DataPath FeaturePhasesPath;
  DataPath TriangleGeomPath;
  DataPath PartNumberPath;

  usize HIDDEN_MaxTrianglesPerFile = std::numeric_limits<int32>::max();
};

/**
 * @class WriteStlFile
 * @brief Writes binary STL files from a TriangleGeom.
 *
 * Grouped modes build resident triangle-index buckets before parallel file writes.
 * Source geometry and label stores use direct per-value access.
 *
 * Every stdio failure becomes an error, and no temporary file is committed once a
 * worker has reported one, so a truncated STL cannot replace a destination file.
 *
 * Each file uses an AtomicFile. Multi-file commits occur sequentially and are not
 * atomic as a group.
 */
class SIMPLNXCORE_EXPORT WriteStlFile
{
public:
  /**
   * @brief Creates a binary STL writer.
   * @param dataStructure Provides mesh and grouping arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later triangles, files, or commits when true.
   * @param inputValues Specifies validated paths and output settings. The caller
   * must keep this object alive for the writer lifetime.
   */
  WriteStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStlFileInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteStlFile() noexcept;

  WriteStlFile(const WriteStlFile&) = delete;
  WriteStlFile(WriteStlFile&&) noexcept = delete;
  WriteStlFile& operator=(const WriteStlFile&) = delete;
  WriteStlFile& operator=(WriteStlFile&&) noexcept = delete;

  /**
   * @brief Writes the selected single or grouped STL outputs.
   * @return The first setup, worker, cancellation, or commit error, or success after completion.
   *
   * Cancellation before commit preserves existing destination files. A later
   * multi-file commit failure can leave earlier files published.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Latches one worker result while holding the shared result mutex.
   * @param result Provides the warnings and any error from one file task.
   *
   * The first error is kept so that a later worker cannot bury the original cause,
   * while every warning from every worker is collected.
   */
  void sendThreadSafeProgressMessage(Result<>&& result);

  /**
   * @brief Returns a copy of the latched worker result.
   * @return The first worker error with every collected warning, or the collected
   * warnings when no worker failed.
   *
   * Callers must consult this before committing any temporary file.
   */
  [[nodiscard]] Result<> getWorkerResult() const;

private:
  DataStructure& m_DataStructure;
  const WriteStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;

  /**
   * @brief Records that one worker has already reported an error.
   *
   * The scheduling loops read this flag without holding m_ProgressMessage_Mutex so that
   * they stop queueing new file tasks, so the flag is atomic. Every write happens under
   * that mutex, which keeps it in step with m_Result.
   */
  mutable std::atomic_bool m_HasErrors{false};
  Result<> m_Result;
};

} // namespace nx::core
