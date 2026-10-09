#include "Hdf5Utilities.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

namespace nx::core::HDF5
{
namespace
{
void CollectPaths(const GroupIO& groupIO, const std::string& groupPath, std::vector<std::string>& groupPaths, std::vector<std::string>& datasetPaths)
{
  for(const std::string& childName : groupIO.getChildNames())
  {
    const std::string childPath = groupPath + "/" + childName;
    if(groupIO.isGroup(childName))
    {
      groupPaths.push_back(childPath);
      GroupIO childGroup = groupIO.openGroup(childName);
      if(childGroup.isValid())
      {
        CollectPaths(childGroup, childPath, groupPaths, datasetPaths);
      }
    }
    else if(groupIO.isDataset(childName))
    {
      datasetPaths.push_back(childPath);
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
std::vector<std::string> getGroupPaths(const FileIO& fileIO)
{
  std::vector<std::string> groupPaths;
  std::vector<std::string> datasetPaths;
  if(fileIO.isValid())
  {
    CollectPaths(fileIO, "", groupPaths, datasetPaths);
  }
  return groupPaths;
}

// -----------------------------------------------------------------------------
std::vector<std::string> getDatasetPaths(const FileIO& fileIO)
{
  std::vector<std::string> groupPaths;
  std::vector<std::string> datasetPaths;
  if(fileIO.isValid())
  {
    CollectPaths(fileIO, "", groupPaths, datasetPaths);
  }
  return datasetPaths;
}
} // namespace nx::core::HDF5
