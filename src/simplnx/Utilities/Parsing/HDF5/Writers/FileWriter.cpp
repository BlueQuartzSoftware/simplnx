#include "FileWriter.hpp"

#include <fmt/format.h>

#include <stdexcept>

namespace nx::core::HDF5
{
Result<FileWriter> FileWriter::CreateFile(const std::filesystem::path& filepath)
{
  Result<FileWriter> result;

  auto parentPath = filepath.parent_path();
  try
  {
    if(!parentPath.empty() && !std::filesystem::exists(parentPath))
    {
      if(!std::filesystem::create_directories(parentPath))
      {
        return MakeErrorResult<FileWriter>(-300, fmt::format("Error creating Output HDF5 file at path '{}'. "
                                                             "Parent path could not be created.",
                                                             filepath.string()));
      }
    }
  } catch(std::filesystem::filesystem_error& fsError)
  {
    return MakeErrorResult<FileWriter>(-300,
                                       fmt::format("Error creating Output HDF5 file at path '{}'. Parent path could not be created. C++ error reported was\n'{}'", filepath.string(), fsError.what()));
  }

  try
  {
    return {FileWriter(filepath)};
  } catch(const std::runtime_error& error)
  {
    return MakeErrorResult<FileWriter>(-301, fmt::format("Error creating Output HDF5 file at path '{}'. "
                                                         "Parent path could not be created.",
                                                         filepath.string()));
  }
  return {}; // Code should not get here. We return everywhere else.
}

Result<FileWriter> FileWriter::WrapHdf5FileId(IdType fileId)
{
  if(fileId <= 0)
  {
    return MakeErrorResult<FileWriter>(-302, fmt::format("Error wrapping existing HDF5 FileId with value '{}'.", fileId));
  }
  return {FileWriter(fileId)};
}

FileWriter::FileWriter() = default;

FileWriter::FileWriter(FileWriter&& rhs) noexcept
: GroupWriter()
{
  auto rhsId = rhs.getId();
  setId(rhsId);
  rhs.setId(-1);
}

FileWriter::FileWriter(const std::filesystem::path& filepath)
: GroupWriter(0, H5Fcreate(filepath.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT))
{
  if(getId() < 0)
  {
    throw std::runtime_error(fmt::format("Error creating Output HDF5 file at "
                                         "path '{}'. HDF5Library threw error.",
                                         filepath.string()));
  }
}

FileWriter::FileWriter(IdType fileId)
: GroupWriter(0, fileId)
{
}

FileWriter::~FileWriter()
{
  closeHdf5();
}

void FileWriter::closeHdf5()
{
  if(isValid())
  {
    H5Fclose(getId());
    setId(0);
  }
}

std::string FileWriter::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 1024;
  char buffer[size];
  H5Fget_name(getId(), buffer, size);
  return GetNameFromBuffer(buffer);
}
} // namespace nx::core::HDF5
