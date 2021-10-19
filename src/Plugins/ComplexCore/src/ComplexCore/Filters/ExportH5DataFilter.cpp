#include "ExportH5DataFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

namespace complex
{
std::string ExportH5DataFilter::name() const
{
  return FilterTraits<ExportH5DataFilter>::name;
}

std::string ExportH5DataFilter::className() const
{
  return FilterTraits<ExportH5DataFilter>::className;
}

Uuid ExportH5DataFilter::uuid() const
{
  return FilterTraits<ExportH5DataFilter>::uuid;
}

std::string ExportH5DataFilter::humanName() const
{
  return "Write DREAM.3D File (V7)";
}

Parameters ExportH5DataFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Export File Path", "The file path the DataStructure should be written to as an HDF5 file.", "",
                                                          FileSystemPathParameter::PathType::OutputFile));
  return params;
}

IFilter::UniquePointer ExportH5DataFilter::clone() const
{
  return std::make_unique<ExportH5DataFilter>();
}

Result<OutputActions> ExportH5DataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto h5FilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  if(h5FilePath.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-1, "Export file path not provided."}})};
  }
  auto exportDirectoryPath = h5FilePath.parent_path();
  if(std::filesystem::exists(exportDirectoryPath) == false)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-3, "Export parent directory does not exist."}})};
  }
  return {};
}

Result<> ExportH5DataFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto h5FilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  H5::FileWriter fileWriter(h5FilePath);
  auto errorCode = dataStructure.writeHdf5(fileWriter);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to write DataStructure to HDF5 file."}})};
  }

  return {};
}
} // namespace complex
