#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadNotesFileInputValues
{
  FileSystemPathParameter::ValueType InputFilePath;
  DataPath CreatedDataPath;
};

/**
 * @class ReadNotesFile
 * @brief This algorithm implements support code for the ReadNotesFileFilter
 */

class SIMPLNXCORE_EXPORT ReadNotesFile
{
public:
  ReadNotesFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNotesFileInputValues* inputValues);
  ~ReadNotesFile() noexcept;

  ReadNotesFile(const ReadNotesFile&) = delete;
  ReadNotesFile(ReadNotesFile&&) noexcept = delete;
  ReadNotesFile& operator=(const ReadNotesFile&) = delete;
  ReadNotesFile& operator=(ReadNotesFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadNotesFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
