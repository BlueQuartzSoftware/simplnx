#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

/**
 * @struct WriteBinaryDataInputValues
 * @brief Stores output directory, extension, byte order, and selected arrays.
 */
struct SIMPLNXCORE_EXPORT WriteBinaryDataInputValues
{
  ChoicesParameter::ValueType EndianIndex;
  StringParameter::ValueType FileExtension;
  MultiArraySelectionParameter::ValueType InputDataArrayPaths;
  FileSystemPathParameter::ValueType OutputPath;
};

/**
 * @class WriteBinaryData
 * @brief Writes selected DataArrays to separate raw binary files.
 *
 * The shared writer reads fixed pages and swaps only its local buffer. This
 * supports disk-backed sources and keeps selected arrays unchanged.
 */
class SIMPLNXCORE_EXPORT WriteBinaryData
{
public:
  /**
   * @brief Creates a raw binary writer.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Receives writer progress.
   * @param shouldCancel Stops before later pages or files when true.
   * @param inputValues Specifies validated output settings. The caller must keep
   * this object alive for the writer lifetime.
   */
  WriteBinaryData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteBinaryDataInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteBinaryData() noexcept;

  WriteBinaryData(const WriteBinaryData&) = delete;
  WriteBinaryData(WriteBinaryData&&) noexcept = delete;
  WriteBinaryData& operator=(const WriteBinaryData&) = delete;
  WriteBinaryData& operator=(WriteBinaryData&&) noexcept = delete;

  /**
   * @brief Writes one binary file for each selected array.
   * @return Directory or shared-writer error, or success after cancellation.
   *
   * Files form independent outputs. A later failure does not roll back earlier files.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteBinaryDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
