#include "ExportDREAM3DFilter.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

namespace complex
{
std::string ExportDREAM3DFilter::name() const
{
  return FilterTraits<ExportDREAM3DFilter>::name;
}

std::string ExportDREAM3DFilter::className() const
{
  return FilterTraits<ExportDREAM3DFilter>::className;
}

Uuid ExportDREAM3DFilter::uuid() const
{
  return FilterTraits<ExportDREAM3DFilter>::uuid;
}

std::string ExportDREAM3DFilter::humanName() const
{
  return "Write DREAM.3D File (V7)";
}

Parameters ExportDREAM3DFilter::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<FileSystemPathParameter>(k_ExportFilePath, "Export File Path", "The file path the DataStructure should be written to as an HDF5 file.", "",
                                                          FileSystemPathParameter::PathType::OutputFile));
  return params;
}

IFilter::UniquePointer ExportDREAM3DFilter::clone() const
{
  return std::make_unique<ExportDREAM3DFilter>();
}

Result<OutputActions> ExportDREAM3DFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
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

Result<> ExportDREAM3DFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
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
