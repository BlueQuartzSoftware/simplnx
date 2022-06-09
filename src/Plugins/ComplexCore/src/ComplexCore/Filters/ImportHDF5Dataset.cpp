#include "ImportHDF5Dataset.hpp"

#include <set>

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

using namespace H5Support;

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ImportHDF5DatasetParameter.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
// -----------------------------------------------------------------------------
std::multiset<size_t> createComponentDimensions(const std::string& cDimsStr)
{
  std::multiset<size_t> cDims;
  std::vector<std::string> dimsStrVec = StringUtilities::split(cDimsStr, std::vector<char>{','}, true);
  for(int i = 0; i < dimsStrVec.size(); i++)
  {
    std::string dimsStr = dimsStrVec[i];
    dimsStr = StringUtilities::trimmed(dimsStr);

    try
    {
      int val = std::stoi(dimsStr);
      cDims.insert(val);
    } catch(std::invalid_argument const& e)
    {
      return std::multiset<size_t>();
    } catch(std::out_of_range const& e)
    {
      return std::multiset<size_t>();
    }
  }

  return cDims;
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::name() const
{
  return FilterTraits<ImportHDF5Dataset>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::className() const
{
  return FilterTraits<ImportHDF5Dataset>::className;
}

//------------------------------------------------------------------------------
Uuid ImportHDF5Dataset::uuid() const
{
  return FilterTraits<ImportHDF5Dataset>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportHDF5Dataset::humanName() const
{
  return "Import HDF5 Dataset";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportHDF5Dataset::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportHDF5Dataset::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ImportHDF5DatasetParameter>(k_ImportHDF5File_Key, "Select HDF5 File", "", ImportHDF5DatasetParameter::ValueType{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_SelectedAttributeMatrix_Key, "Attribute Matrix", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportHDF5Dataset::clone() const
{
  return std::make_unique<ImportHDF5Dataset>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportHDF5Dataset::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pImportHDF5FileValue = filterArgs.value<ImportHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  auto inputFile = pImportHDF5FileValue.first;
  auto datasetImportInfoList = pImportHDF5FileValue.second;

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  if(inputFile.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20001, "The HDF5 file path is empty.  Please select an HDF5 file."}})};
  }

  fs::path hdf5File(inputFile);
  fs::path inputFilePath(inputFile);
  std::string ext = inputFilePath.extension().string();
  if(ext != ".h5" && ext != ".hdf5" && ext != ".dream3d")
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20002, fmt::format("The selected file '{}' is not an HDF5 file.", inputFilePath.filename().string())}})};
  }

  if(!fs::exists(hdf5File))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20003, fmt::format("The selected file '{}' does not exist.", inputFilePath.filename().string())}})};
  }

  if(datasetImportInfoList.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20004, "No dataset has been checked.  Please check a dataset."}})};
  }

  auto selectedDataGroup = dataStructure.getDataAs<DataGroup>(pSelectedAttributeMatrixValue);
  if(selectedDataGroup == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20005, fmt::format("The selected data group '{}' does not exist.", pSelectedAttributeMatrixValue.toString())}})};
  }

  int err = 0;
  hid_t fileId = H5::FileReader(inputFilePath).getId();
  if(fileId < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20006, fmt::format("Error Reading HDF5 file: '{}'", inputFile)}})};
  }
  H5ScopedFileSentinel sentinel(fileId, true);

  std::map<std::string, hid_t> openedParentPathsMap;
  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    std::string datasetPath = datasetImportInfo.dataSetPath;

    std::string parentPath = H5Utilities::getParentPath(datasetPath);
    hid_t parentId;
    if(parentPath.empty())
    {
      parentId = fileId;
    }
    else
    {
      if(openedParentPathsMap.find(parentPath) == openedParentPathsMap.end())
      {
        parentId = H5Utilities::openHDF5Object(fileId, parentPath);
        sentinel.addGroupId(parentId);
        openedParentPathsMap[parentPath] = parentId;
      }
      else
      {
        parentId = openedParentPathsMap[parentPath];
      }
    }

    // Read dataset into DREAM.3D structure
    std::string objectName = H5Utilities::getObjectNameFromPath(datasetPath);

    std::vector<hsize_t> dims;
    H5T_class_t type_class;
    size_t type_size;
    err = H5Lite::getDatasetInfo(parentId, objectName, dims, type_class, type_size);
    if(err < 0)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20007, fmt::format("Error reading type info from dataset with path '{}'", datasetPath)}})};
    }

    std::string cDimsStr = datasetImportInfo.componentDimensions;
    if(cDimsStr.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20008, fmt::format("The component dimensions are empty for dataset with path '{}'.  Please enter the component dimensions, using comma-separated values (ex: 4x2 would be '4, 2').",
                                    datasetPath)}})};
    }

    std::multiset<size_t> cDimsSet = createComponentDimensions(cDimsStr);
    if(cDimsSet.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20009, fmt::format("Component Dimensions are not in the right format for dataset with path '{}'. Use comma-separated values (ex: 4x2 would be '4, 2').", datasetPath)}})};
    }

    // Calculate the product of the dataset dimensions and the product of the component dimensions.
    // Since we're already looping over both of these sets of dimensions, let's also create our error message
    // in case the equation does not work and we have to bail.
    std::stringstream stream;

    stream << "-------------------------------------------\n";
    stream << fmt::format("HDF5 File Path: '{}'\n", inputFile);
    stream << fmt::format("HDF5 Dataset Path: '{}'\n", datasetPath);

    size_t hdf5TotalElements = 1;
    stream << "    No. of Dimension(s): " << StringUtilities::number(dims.size()) << "\n";
    stream << "    Dimension Size(s): ";
    std::vector<size_t> cDims;
    std::vector<size_t> tupleDims;
    for(int i = 0; i < dims.size(); i++)
    {
      stream << StringUtilities::number(dims[i]);
      hdf5TotalElements = hdf5TotalElements * dims[i];
      if(i != dims.size() - 1)
      {
        stream << " x ";
      }

      auto cDimIt = cDimsSet.find(dims[i]);
      if(cDimIt != cDimsSet.end())
      {
        cDims.push_back(dims[i]);
        cDimsSet.erase(cDimIt);
      }
      else
      {
        tupleDims.push_back(dims[i]);
      }
    }
    stream << "No. of Component Dimension(s): " << StringUtilities::number(static_cast<uint64>(cDims.size())) << "\n";
    // For now, we will just make sure the component dimnesions entered by the user exist in the dataset from the file
    if(!cDimsSet.empty())
    {
      stream << fmt::format("The dataset with path '{}' cannot be read because the following component dimensions entered don't exist in the dataset's list of dimension sizes: ", inputFile);
      for(const auto& cdim : cDimsSet)
      {
        stream << cdim << " ";
      }
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20010, stream.str()}})};
    }

    // NOTE: When complex implements the “AttributeMatrix” this may become a requirement to check the tuple dimensions.
    stream << "\n";
    stream << "    Total HDF5 Dataset Element Count: " << hdf5TotalElements << "\n";
    // stream << "-------------------------------------------\n";
    // stream << "Current Data Structure Information: \n";
    //
    // stream << fmt::format("Attribute Matrix Path: '{}'\n", pSelectedAttributeMatrixValue.toString());
    //
    // size_t userEnteredTotalElements = 1;
    // std::vector<size_t> amTupleDims = selectedDataGroup->getTupleDimensions();
    // stream << "No. of Attribute Matrix Dimension(s): " << StringUtilities::number(static_cast<uint64>(amTupleDims.size())) << "\n";
    // stream << "Attribute Matrix Dimension(s): ";
    // for(int i = 0; i < amTupleDims.size(); i++)
    //{
    //  userEnteredTotalElements = userEnteredTotalElements * amTupleDims[i];
    //  int d = amTupleDims[i];
    //  stream << StringUtilities::number(d);
    //  if(i != amTupleDims.size() - 1)
    //  {
    //    stream << " x ";
    //  }
    //}
    // stream << "\n";
    //
    // int numOfAMTuples = selectedDataGroup->getNumberOfTuples();
    // stream << fmt::format("Total Attribute Matrix Tuple Count: '{}'\n", StringUtilities::number(numOfAMTuples));
    //
    // stream << "No. of Component Dimension(s): " << StringUtilities::number(static_cast<uint64>(cDims.size())) << "\n";
    // stream << "Component Dimension(s): ";
    //
    // int totalComponents = 1;
    // for(int i = 0; i < cDims.size(); i++)
    //{
    // userEnteredTotalElements = userEnteredTotalElements * cDims[i];
    //  totalComponents = totalComponents * cDims[i];
    //  int d = cDims[i];
    //  stream << StringUtilities::number(d);
    //  if(i != cDims.size() - 1)
    //  {
    //    stream << " x ";
    //  }
    //}
    //
    // stream << "\n";
    // stream << fmt::format("Total Component Count: '{}'\n", StringUtilities::number(totalComponents));
    //
    // stream << "\n";
    //
    // auto totalTuples = hdf5TotalElements / totalComponents;
    // if(hdf5TotalElements != userEnteredTotalElements)
    //{
    //  stream << fmt::format("The dataset with path '{}' cannot be read into attribute matrix '{}' because '{}' "
    //                        "attribute matrix tuples and '{}' components per tuple equals '{}' total elements, and"
    //                        " that does not match the total HDF5 dataset element count of '{}'.\n"
    //                        "'{}' =/= '{}'",
    //                        datasetPath, pSelectedAttributeMatrixValue.getTargetName(), StringUtilities::number(numOfAMTuples), StringUtilities::number(totalComponents),
    //                        StringUtilities::number(numOfAMTuples * totalComponents), StringUtilities::number(static_cast<ulonglong>(hdf5TotalElements)),
    //                        StringUtilities::number(numOfAMTuples * totalComponents), StringUtilities::number(static_cast<ulonglong>(hdf5TotalElements)));
    //
    //
    // return {nonstd::make_unexpected(std::vector<Error>{Error{-20010, stream.str()}})};
    //}

    if(selectedDataGroup->contains(objectName))
    {
      stream.clear();
      stream << "The selected dataset '" << pSelectedAttributeMatrixValue.toString() << "/" << objectName << "' already exists.";
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20011, stream.str()}})};
    }
    else
    {
      DataPath dataArrayPath = pSelectedAttributeMatrixValue.createChildPath(objectName);
      H5::DatasetReader datasetReader(parentId, objectName);
      auto type = datasetReader.getDataType();
      if(!type.errors().empty())
      {
        return {nonstd::make_unexpected(
            std::vector<Error>{Error{-20012, fmt::format("The selected datatset '{}' is not a supported type for importing. Please select a different data set", datasetPath)}})};
      }
      auto action = std::make_unique<CreateArrayAction>(type.value(), tupleDims, cDims, dataArrayPath);
      resultOutputActions.value().actions.push_back(std::move(action));
    }
  } // End For Loop over dataset imoprt info list

  // The sentinel will close the HDF5 File and any groups that were open.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportHDF5Dataset::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pImportHDF5FileValue = filterArgs.value<ImportHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
