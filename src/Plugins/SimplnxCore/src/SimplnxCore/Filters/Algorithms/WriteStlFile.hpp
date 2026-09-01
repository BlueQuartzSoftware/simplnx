#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

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
 * Each file uses an AtomicFile. Multi-file commits occur sequentially and are
 * not atomic as a group. Some stdio and overflow-commit failures are not returned.
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
   * @return Setup, worker, or reported commit errors, or success after cancellation.
   *
   * Cancellation before commit preserves existing destination files. A later
   * multi-file commit failure can leave earlier files published.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Merges one worker result while holding the shared result mutex.
   * @param result Provides a warning or error from one file task.
   */
  void sendThreadSafeProgressMessage(Result<>&& result);

private:
  DataStructure& m_DataStructure;
  const WriteStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;

  mutable bool m_HasErrors = false;
  Result<> m_Result;
};

} // namespace nx::core
