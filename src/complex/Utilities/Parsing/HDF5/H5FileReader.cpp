#include "H5FileReader.hpp"

#include <H5Apublic.h>

#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

using namespace complex;

H5::FileReader::FileReader(const std::filesystem::path& filepath)
: GroupReader(0, H5Fopen(filepath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT))
{
}

H5::FileReader::FileReader(H5::IdType fileId)
: GroupReader(0, fileId)
{
}

H5::FileReader::~FileReader()
{
  closeHdf5();
}

void H5::FileReader::closeHdf5()
{
  if(isValid())
  {
    H5Fclose(getId());
    setId(0);
  }
}

std::string H5::FileReader::getName() const
{
  if(!isValid())
  {
    return "";
  }

  constexpr size_t size = 1024;
  char buffer[size];
  H5Fget_name(getId(), buffer, size);

  return H5::GetNameFromBuffer(buffer);
}
