#include "ImportHDF5Dataset.hpp"

#include <set>

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ImportHDF5DatasetParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace H5Support;
using namespace complex;
namespace fs = std::filesystem;

namespace
{
// -----------------------------------------------------------------------------
std::vector<size_t> createDimensionVector(const std::string& cDimsStr)
{
  std::vector<size_t> cDims;
  std::vector<std::string> dimsStrVec = StringUtilities::split(cDimsStr, std::vector<char>{','}, true);
  for(int i = 0; i < dimsStrVec.size(); i++)
  {
    std::string dimsStr = dimsStrVec[i];
    dimsStr = StringUtilities::trimmed(dimsStr);

    try
    {
      int val = std::stoi(dimsStr);
      cDims.push_back(val);
    } catch(std::invalid_argument const& e)
    {
      return std::vector<size_t>();
    } catch(std::out_of_range const& e)
    {
      return std::vector<size_t>();
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

  fs::path inputFilePath(inputFile);
  std::string ext = inputFilePath.extension().string();
  if(ext != ".h5" && ext != ".hdf5" && ext != ".dream3d")
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20002, fmt::format("The selected file '{}' is not an HDF5 file.", inputFilePath.filename().string())}})};
  }

  if(!fs::exists(inputFilePath))
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
    if(datasetPath.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20007, "Cannot import empty dataset path"}})};
    }

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
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20008, fmt::format("Error reading type info from dataset with path '{}'", datasetPath)}})};
    }

    std::string cDimsStr = datasetImportInfo.componentDimensions;
    if(cDimsStr.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20009, fmt::format("The component dimensions are empty for dataset with path '{}'.  Please enter the component dimensions, using comma-separated values (ex: 4x2 would be '4, 2').",
                                    datasetPath)}})};
    }

    std::vector<size_t> cDims = createDimensionVector(cDimsStr);
    if(cDims.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20010, fmt::format("Component Dimensions are not in the right format for dataset with path '{}'. Use comma-separated values (ex: 4x2 would be '4, 2').", datasetPath)}})};
    }

    std::string tDimsStr = datasetImportInfo.tupleDimensions;
    if(tDimsStr.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20011,
                fmt::format("The tuple dimensions are empty for dataset with path '{}'.  Please enter the tuple dimensions, using comma-separated values (ex: 4x2 would be '4, 2').", datasetPath)}})};
    }

    std::vector<size_t> tDims = createDimensionVector(tDimsStr);
    if(tDims.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20012, fmt::format("Tuple Dimensions are not in the right format for dataset with path '{}'. Use comma-separated values (ex: 4x2 would be '4, 2').", datasetPath)}})};
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
    for(int i = 0; i < dims.size(); i++)
    {
      stream << StringUtilities::number(dims[i]);
      hdf5TotalElements = hdf5TotalElements * dims[i];
      if(i != dims.size() - 1)
      {
        stream << " x ";
      }
    }

    // NOTE: When complex implements the “AttributeMatrix” the user entered tuple dimensions should be optional in lieu of an attribute matrix as the selected data path/parent.
    stream << "\n";
    stream << "    Total HDF5 Dataset Element Count: " << hdf5TotalElements << "\n";
    stream << "-------------------------------------------\n";
    // stream << "Current Data Structure Information: \n";
    // stream << fmt::format("Attribute Matrix Path: '{}'\n", pSelectedAttributeMatrixValue.toString());

    // std::vector<size_t> amTupleDims = am->getTupleDimensions();
    // stream << "No. of Attribute Matrix Dimension(s): " << StringUtilities::number(static_cast<uint64>(amTupleDims.size())) << "\n";
    // stream << "Attribute Matrix Dimension(s): ";
    stream << "No. of Tuple Dimension(s): " << StringUtilities::number(static_cast<uint64>(tDims.size())) << "\n";
    stream << "Tuple Dimension(s): ";
    size_t userEnteredTotalElements = 1;
    for(int i = 0; i < tDims.size(); i++)
    {
      userEnteredTotalElements = userEnteredTotalElements * tDims[i];
      int d = tDims[i];
      stream << StringUtilities::number(d);
      if(i != tDims.size() - 1)
      {
        stream << " x ";
      }
    }
    stream << "\n";

    // int numOfAMTuples = am->getNumberOfTuples();
    int numOfTuples = userEnteredTotalElements;
    stream << fmt::format("Total Attribute Matrix Tuple Count: '{}'\n", StringUtilities::number(numOfTuples));

    stream << "No. of Component Dimension(s): " << StringUtilities::number(static_cast<uint64>(cDims.size())) << "\n";
    stream << "Component Dimension(s): ";
    int totalComponents = 1;
    for(int i = 0; i < cDims.size(); i++)
    {
      userEnteredTotalElements = userEnteredTotalElements * cDims[i];
      totalComponents = totalComponents * cDims[i];
      int d = cDims[i];
      stream << StringUtilities::number(d);
      if(i != cDims.size() - 1)
      {
        stream << " x ";
      }
    }

    stream << "\n";
    stream << fmt::format("Total Component Count: '{}'\n", StringUtilities::number(totalComponents));
    stream << "\n";

    if(hdf5TotalElements != userEnteredTotalElements)
    {
      stream << fmt::format("The dataset with path '{}' cannot be read into attribute matrix '{}' because '{}' "
                            "tuples and '{}' components per tuple equals '{}' total elements, and"
                            " that does not match the total HDF5 dataset element count of '{}'.\n"
                            "'{}' =/= '{}'",
                            datasetPath, pSelectedAttributeMatrixValue.getTargetName(), StringUtilities::number(numOfTuples), StringUtilities::number(totalComponents),
                            StringUtilities::number(numOfTuples * totalComponents), StringUtilities::number(hdf5TotalElements), StringUtilities::number(numOfTuples * totalComponents),
                            StringUtilities::number(hdf5TotalElements));
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20013, stream.str()}})};
    }

    if(selectedDataGroup->contains(objectName))
    {
      stream.clear();
      stream << "The selected dataset '" << pSelectedAttributeMatrixValue.toString() << "/" << objectName << "' already exists.";
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20014, stream.str()}})};
    }
    else
    {
      DataPath dataArrayPath = pSelectedAttributeMatrixValue.createChildPath(objectName);
      H5::DatasetReader datasetReader(parentId, objectName);
      auto type = datasetReader.getDataType();
      if(type.invalid())
      {
        return {nonstd::make_unexpected(
            std::vector<Error>{Error{-20015, fmt::format("The selected datatset '{}' is not a supported type for importing. Please select a different data set", datasetPath)}})};
      }
      auto action = std::make_unique<CreateArrayAction>(type.value(), tDims, cDims, dataArrayPath);
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
  auto pImportHDF5FileValue = filterArgs.value<ImportHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = filterArgs.value<DataPath>(k_SelectedAttributeMatrix_Key);
  auto inputFile = pImportHDF5FileValue.first;
  fs::path inputFilePath(inputFile);
  auto datasetImportInfoList = pImportHDF5FileValue.second;
  auto selectedDataGroup = dataStructure.getDataAs<DataGroup>(pSelectedAttributeMatrixValue);

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
    std::string objectName = H5Utilities::getObjectNameFromPath(datasetPath);

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

    // NOTE: When complex implements the “AttributeMatrix” the user entered tuple dimensions should be optional in lieu of an attribute matrix as the selected data path/parent.
    std::string cDimsStr = datasetImportInfo.componentDimensions;
    std::vector<size_t> cDims = createDimensionVector(cDimsStr);
    std::string tDimsStr = datasetImportInfo.tupleDimensions;
    std::vector<size_t> tDims = createDimensionVector(tDimsStr);

    // Read dataset into DREAM.3D structure
    DataPath dataArrayPath = pSelectedAttributeMatrixValue.createChildPath(objectName);
    H5::DatasetReader datasetReader(parentId, objectName);
    auto type = datasetReader.getType();
    switch(type)
    {
    case H5::Type::float32: {
      Float32Array::CreateWithStore<Float32DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::float64: {
      Float64Array::CreateWithStore<Float64DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::int8: {
      Int8Array::CreateWithStore<Int8DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::int16: {
      Int16Array::CreateWithStore<Int16DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::int32: {
      Int32Array::CreateWithStore<Int32DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::int64: {
      Int64Array::CreateWithStore<Int64DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::uint8: {
      UInt8Array::CreateWithStore<UInt8DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::uint16: {
      UInt16Array::CreateWithStore<UInt16DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::uint32: {
      UInt32Array::CreateWithStore<UInt32DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    case H5::Type::uint64: {
      UInt64Array::CreateWithStore<UInt64DataStore>(dataStructure, objectName, tDims, cDims, selectedDataGroup->getId());
      break;
    }
    default:
      return {
          nonstd::make_unexpected(std::vector<Error>{Error{-20012, fmt::format("The selected datatset '{}' is not a supported type for importing. Please select a different data set", datasetPath)}})};
    }
  } // End For Loop over dataset imoprt info list

  // The sentinel will close the HDF5 File and any groups that were open.

  return {};
}
} // namespace complex
