#include "ExportH5DataFilter.hpp"

#include "complex/Core/Parameters/FileSystemPathParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

namespace complex
{
std::string ExportH5DataFilter::name() const
{
  return FilterTraits<ExportH5DataFilter>::name;
}

Uuid ExportH5DataFilter::uuid() const
{
  return FilterTraits<ExportH5DataFilter>::uuid;
}

std::string ExportH5DataFilter::humanName() const
{
  return "Export HDF5 Data Filter";
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
  if(std::filesystem::exists(exportDirectoryPath))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-3, "Export parent directory does not exist."}})};
  }
  return {};
}

Result<> ExportH5DataFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto h5FilePath = args.value<std::filesystem::path>(k_ExportFilePath);
  H5::FileWriter fileWriter(h5FilePath);
  dataStructure.writeHdf5(fileWriter);

  return {};
}
} // namespace complex
