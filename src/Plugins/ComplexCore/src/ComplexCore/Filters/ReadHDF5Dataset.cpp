#include "ReadHDF5Dataset.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ImportHDF5DatasetParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <nonstd/span.hpp>

using namespace complex;
namespace fs = std::filesystem;

namespace
{
// -----------------------------------------------------------------------------
std::vector<size_t> createDimensionVector(const std::string& cDimsStr)
{
  std::vector<size_t> cDims;
  std::vector<std::string> dimsStrVec = StringUtilities::split(cDimsStr, std::vector<char>{','}, true);
  for(const auto& stringValue : dimsStrVec)
  {
    std::string dimsStr = StringUtilities::trimmed(stringValue);

    try
    {
      int val = std::stoi(dimsStr);
      cDims.push_back(val);
    } catch(std::invalid_argument const& e)
    {
      return {};
    } catch(std::out_of_range const& e)
    {
      return {};
    }
  }

  return cDims;
}

template <typename T>
Result<> fillDataStore(DataArray<T>& dataArray, const DataPath& dataArrayPath, const complex::HDF5::DatasetReader& datasetReader)
{
  using StoreType = DataStore<T>;
  StoreType& dataStore = dataArray.template getIDataStoreRefAs<StoreType>();
  if(!datasetReader.readIntoSpan<T>(dataStore.createSpan()))
  {
    return {MakeErrorResult(-21002, fmt::format("Error reading dataset '{}' with '{}' total elements into data store for data array '{}' with '{}' total elements ('{}' tuples and '{}' components)",
                                                dataArrayPath.getTargetName(), datasetReader.getNumElements(), dataArrayPath.toString(), dataArray.getSize(), dataArray.getNumberOfTuples(),
                                                dataArray.getNumberOfComponents()))};
  }

  return {};
}

template <typename T>
Result<> fillOocDataStore(DataArray<T>& dataArray, const DataPath& dataArrayPath, const complex::HDF5::DatasetReader& datasetReader)
{
  if(Memory::GetTotalMemory() <= dataArray.getSize() * sizeof(T))
  {
    return MakeErrorResult(-21004, fmt::format("Error reading dataset '{}' with '{}' total elements. Not enough memory to import data.", dataArray.getName(), datasetReader.getNumElements()));
  }

  auto& absDataStore = dataArray.getDataStoreRef();
  std::vector<T> data(absDataStore.getSize());
  nonstd::span<T> span{data.data(), data.size()};
  if(!datasetReader.readIntoSpan<T>(span))
  {
    return {MakeErrorResult(-21003, fmt::format("Error reading dataset '{}' with '{}' total elements into data store for data array '{}' with '{}' total elements ('{}' tuples and '{}' components)",
                                                dataArrayPath.getTargetName(), datasetReader.getNumElements(), dataArrayPath.toString(), dataArray.getSize(), dataArray.getNumberOfTuples(),
                                                dataArray.getNumberOfComponents()))};
  }
  std::copy(data.begin(), data.end(), absDataStore.begin());

  return {};
}

template <typename T>
Result<> fillDataArray(DataStructure& dataStructure, const DataPath& dataArrayPath, const complex::HDF5::DatasetReader& datasetReader)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
  if(dataArray.getDataFormat().empty())
  {
    return fillDataStore(dataArray, dataArrayPath, datasetReader);
  }
  else
  {
    return fillOocDataStore(dataArray, dataArrayPath, datasetReader);
  }
}
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ReadHDF5Dataset::name() const
{
  return FilterTraits<ReadHDF5Dataset>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadHDF5Dataset::className() const
{
  return FilterTraits<ReadHDF5Dataset>::className;
}

//------------------------------------------------------------------------------
Uuid ReadHDF5Dataset::uuid() const
{
  return FilterTraits<ReadHDF5Dataset>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadHDF5Dataset::humanName() const
{
  return "Read HDF5 Dataset";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadHDF5Dataset::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadHDF5Dataset::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ImportHDF5DatasetParameter>(k_ImportHDF5File_Key, "Select HDF5 File", "The HDF5 file data to import", ImportHDF5DatasetParameter::ValueType{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadHDF5Dataset::clone() const
{
  return std::make_unique<ReadHDF5Dataset>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadHDF5Dataset::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto pImportHDF5FileValue = filterArgs.value<ImportHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = pImportHDF5FileValue.parent;
  auto inputFile = pImportHDF5FileValue.inputFile;
  auto datasetImportInfoList = pImportHDF5FileValue.datasets;

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(inputFile.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20001, "The HDF5 file path is empty.  Please select an HDF5 file."}})};
  }

  fs::path inputFilePath(inputFile);
  std::string ext = inputFilePath.extension().string();

  if(!fs::exists(inputFilePath))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20003, fmt::format("The selected file '{}' does not exist.", inputFilePath.filename().string())}})};
  }

  if(datasetImportInfoList.empty())
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20004, "No dataset has been checked.  Please check a dataset."}})};
  }

  if(pSelectedAttributeMatrixValue.has_value())
  {
    if(dataStructure.getDataAs<DataGroup>(pSelectedAttributeMatrixValue.value()) == nullptr && dataStructure.getDataAs<AttributeMatrix>(pSelectedAttributeMatrixValue.value()) == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{
          Error{-20005, fmt::format("The selected data path '{}' does not exist. Make sure you are selecting a DataGroup or AttributeMatrix.", pSelectedAttributeMatrixValue.value().toString())}})};
    }
  }

  const AttributeMatrix* parentAM = pSelectedAttributeMatrixValue.has_value() ? dataStructure.getDataAs<AttributeMatrix>(pSelectedAttributeMatrixValue.value()) : nullptr;

  int err = 0;
  complex::HDF5::FileReader h5FileReader(inputFilePath);
  hid_t fileId = h5FileReader.getId();
  if(fileId < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-20006, fmt::format("Error Reading HDF5 file: '{}'", inputFile)}})};
  }

  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    std::string datasetPath = datasetImportInfo.dataSetPath;
    if(datasetPath.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20007, "Cannot import an empty dataset path"}})};
    }

    // Read dataset into DREAM.3D structure
    complex::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(datasetPath);
    std::vector<hsize_t> dims = datasetReader.getDimensions();
    std::string objectName = datasetReader.getName();

    if(dims.empty())
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20008, fmt::format("Error reading dimensions from dataset with path '{}'", datasetPath)}})};
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

    std::vector<size_t> tDims;
    if(parentAM == nullptr)
    {
      std::string tDimsStr = datasetImportInfo.tupleDimensions;
      if(tDimsStr.empty())
      {
        return {nonstd::make_unexpected(std::vector<Error>{
            Error{-20011, fmt::format("The tuple dimensions are empty for dataset with path '{}'.  Please enter the tuple dimensions, using comma-separated values (ex: 4x2 would be '4, 2').",
                                      datasetPath)}})};
      }

      tDims = createDimensionVector(tDimsStr);
      if(tDims.empty())
      {
        return {nonstd::make_unexpected(std::vector<Error>{
            Error{-20012, fmt::format("Tuple Dimensions are not in the right format for dataset with path '{}'. Use comma-separated values (ex: 4x2 would be '4, 2').", datasetPath)}})};
      }
    }
    else
    {
      tDims = parentAM->getShape();
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

    // NOTE: When complex implements the AttributeMatrix, the user entered tuple dimensions should be optional in lieu of an attribute matrix as the selected data path/parent.
    stream << "\n";
    stream << "    Total HDF5 Dataset Element Count: " << hdf5TotalElements << "\n";
    stream << "-------------------------------------------\n";
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
      stream << fmt::format("The dataset with path '{}' cannot be read in because '{}' "
                            "tuples and '{}' components per tuple equals '{}' total elements, and"
                            " that does not match the total HDF5 dataset element count of '{}'.\n"
                            "'{}' =/= '{}'",
                            datasetPath, StringUtilities::number(numOfTuples), StringUtilities::number(totalComponents), StringUtilities::number(numOfTuples * totalComponents),
                            StringUtilities::number(hdf5TotalElements), StringUtilities::number(numOfTuples * totalComponents), StringUtilities::number(hdf5TotalElements));
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20013, stream.str()}})};
    }

    DataPath dataArrayPath = pSelectedAttributeMatrixValue.has_value() ? pSelectedAttributeMatrixValue.value().createChildPath(objectName) : DataPath::FromString(objectName).value();

    if(dataStructure.getId(dataArrayPath).has_value())
    {
      stream.clear();
      stream << "The selected dataset '" << pSelectedAttributeMatrixValue.value().toString() << "/" << objectName << "' already exists.";
      return {nonstd::make_unexpected(std::vector<Error>{Error{-20014, stream.str()}})};
    }
    else
    {
      Result<HDF5::Type> type = datasetReader.getDataType();
      if(type.invalid())
      {
        return {
            nonstd::make_unexpected(std::vector<Error>{Error{-20015, fmt::format("The selected datatset '{}' with type '{}' is not a supported type for importing. Please select a different data set",
                                                                                 datasetPath, fmt::underlying(datasetReader.getType()))}})};
      }
      DataType dataType = complex::HDF5::toCommonType(type.value()).value();
      auto action = std::make_unique<CreateArrayAction>(dataType, tDims, cDims, dataArrayPath);
      resultOutputActions.value().appendAction(std::move(action));
    }
  } // End For Loop over dataset imoprt info list

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadHDF5Dataset::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  auto pImportHDF5FileValue = filterArgs.value<ImportHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = pImportHDF5FileValue.parent;
  auto inputFile = pImportHDF5FileValue.inputFile;
  fs::path inputFilePath(inputFile);
  auto datasetImportInfoList = pImportHDF5FileValue.datasets;

  complex::HDF5::FileReader h5FileReader(inputFilePath);
  hid_t fileId = h5FileReader.getId();
  if(fileId < 0)
  {
    return {MakeErrorResult(-21000, fmt::format("Error Reading HDF5 file: '{}'", inputFile))};
  }

  std::map<std::string, hid_t> openedParentPathsMap;
  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    std::string datasetPath = datasetImportInfo.dataSetPath;
    complex::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(datasetPath);
    std::string objectName = datasetReader.getName();

    // Read dataset into DREAM.3D structure
    DataPath dataArrayPath = pSelectedAttributeMatrixValue.has_value() ? pSelectedAttributeMatrixValue.value().createChildPath(objectName) : DataPath::FromString(objectName).value();
    Result<> fillArrayResults;
    auto type = datasetReader.getType();
    switch(type)
    {
    case complex::HDF5::Type::float32: {
      fillArrayResults = fillDataArray<float32>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::float64: {
      fillArrayResults = fillDataArray<float64>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::int8: {
      fillArrayResults = fillDataArray<int8>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::int16: {
      fillArrayResults = fillDataArray<int16>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::int32: {
      fillArrayResults = fillDataArray<int32>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::int64: {
      fillArrayResults = fillDataArray<int64>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::uint8: {
      fillArrayResults = fillDataArray<uint8>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::uint16: {
      fillArrayResults = fillDataArray<uint16>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::uint32: {
      fillArrayResults = fillDataArray<uint32>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    case complex::HDF5::Type::uint64: {
      fillArrayResults = fillDataArray<uint64>(dataStructure, dataArrayPath, datasetReader);
      break;
    }
    default: {
      return {MakeErrorResult(-21001,
                              fmt::format("The selected datatset '{}' with type '{}' is not a supported type for importing. Please select a different data set", datasetPath, fmt::underlying(type)))};
    }
    }
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  } // End For Loop over dataset imoprt info list

  return {};
}
} // namespace complex
