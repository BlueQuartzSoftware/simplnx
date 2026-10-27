#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{
class AtomicFile;

struct SIMPLNXCORE_EXPORT WriteFeatureDataCSVInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  ChoicesParameter::ValueType DelimiterIndex;
  FileSystemPathParameter::ValueType FeatureDataFile;
  BoolParameter::ValueType WriteNeighborlistData;
  BoolParameter::ValueType WriteNumFeaturesLine;
};

/**
 * @class WriteFeatureDataCSV
 * @brief This algorithm implements support code for the WriteFeatureDataCSVFilter
 */

class SIMPLNXCORE_EXPORT WriteFeatureDataCSV
{
public:
  WriteFeatureDataCSV(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteFeatureDataCSVInputValues* inputValues);
  ~WriteFeatureDataCSV() noexcept;

  WriteFeatureDataCSV(const WriteFeatureDataCSV&) = delete;
  WriteFeatureDataCSV(WriteFeatureDataCSV&&) noexcept = delete;
  WriteFeatureDataCSV& operator=(const WriteFeatureDataCSV&) = delete;
  WriteFeatureDataCSV& operator=(WriteFeatureDataCSV&&) noexcept = delete;

  /**
   * @brief Writes the feature data to a temporary file and commits it onto the output path.
   * @return Success with the warnings of the file-replace hook, or the first error with those warnings in front.
   *
   * AtomicFile::Create() runs the file-replace hook on the output path before anything is written.
   */
  Result<> operator()();

private:
  /**
   * @brief Writes the CSV file to the temporary path and commits it.
   * @param atomicFile Temporary file whose commit() replaces the output file.
   * @return Success, or the directory, open, cancel or commit error.
   */
  Result<> writeAndCommit(AtomicFile& atomicFile) const;

  DataStructure& m_DataStructure;
  const WriteFeatureDataCSVInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
