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

struct SIMPLNXCORE_EXPORT WriteBinaryDataInputValues
{
  ChoicesParameter::ValueType EndianIndex;
  StringParameter::ValueType FileExtension;
  MultiArraySelectionParameter::ValueType InputDataArrayPaths;
  FileSystemPathParameter::ValueType OutputPath;
};

/**
 * @class WriteBinaryData
 * @brief This algorithm implements support code for the WriteBinaryDataFilter
 */

class SIMPLNXCORE_EXPORT WriteBinaryData
{
public:
  WriteBinaryData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteBinaryDataInputValues* inputValues);
  ~WriteBinaryData() noexcept;

  WriteBinaryData(const WriteBinaryData&) = delete;
  WriteBinaryData(WriteBinaryData&&) noexcept = delete;
  WriteBinaryData& operator=(const WriteBinaryData&) = delete;
  WriteBinaryData& operator=(WriteBinaryData&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteBinaryDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
