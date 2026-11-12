#include "ReadHDF5DatasetFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/ReadHDF5Dataset.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5DataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <limits>
#include <numeric>

using namespace nx::core;
namespace fs = std::filesystem;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadHDF5DatasetFilter::name() const
{
  return FilterTraits<ReadHDF5DatasetFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadHDF5DatasetFilter::className() const
{
  return FilterTraits<ReadHDF5DatasetFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadHDF5DatasetFilter::uuid() const
{
  return FilterTraits<ReadHDF5DatasetFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadHDF5DatasetFilter::humanName() const
{
  return "Read HDF5 Dataset";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadHDF5DatasetFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadHDF5DatasetFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ReadHDF5DatasetParameter>(k_ImportHDF5File_Key, "Select HDF5 File", "The HDF5 file data to import", ReadHDF5DatasetParameter::ValueType{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadHDF5DatasetFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadHDF5DatasetFilter::clone() const
{
  return std::make_unique<ReadHDF5DatasetFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadHDF5DatasetFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pImportHDF5FileValue = filterArgs.value<ReadHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);
  auto pSelectedAttributeMatrixValue = pImportHDF5FileValue.parent;
  auto inputFile = pImportHDF5FileValue.inputFile;
  auto datasetImportInfoList = pImportHDF5FileValue.datasets;

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(inputFile.empty())
  {
    return MakePreflightErrorResult(-20001, "The HDF5 file path is empty.  Please select an HDF5 file.");
  }

  fs::path inputFilePath(inputFile);
  std::string ext = inputFilePath.extension().string();

  if(!fs::exists(inputFilePath))
  {
    return MakePreflightErrorResult(-20003, fmt::format("The selected file '{}' does not exist.", inputFilePath.filename().string()));
  }

  if(datasetImportInfoList.empty())
  {
    return MakePreflightErrorResult(-20004, "No dataset has been checked.  Please check a dataset.");
  }

  if(pSelectedAttributeMatrixValue.has_value())
  {
    if(dataStructure.getDataAs<DataGroup>(pSelectedAttributeMatrixValue.value()) == nullptr && dataStructure.getDataAs<AttributeMatrix>(pSelectedAttributeMatrixValue.value()) == nullptr)
    {
      return MakePreflightErrorResult(
          -20005, fmt::format("The selected data path '{}' does not exist. Make sure you are selecting a DataGroup or AttributeMatrix.", pSelectedAttributeMatrixValue.value().toString()));
    }
  }

  const AttributeMatrix* parentAM = pSelectedAttributeMatrixValue.has_value() ? dataStructure.getDataAs<AttributeMatrix>(pSelectedAttributeMatrixValue.value()) : nullptr;

  int err = 0;
  auto h5FileReader = nx::core::HDF5::FileIO::ReadFile(inputFilePath);

  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    std::string datasetPath = datasetImportInfo.dataSetPath;
    if(datasetPath.empty())
    {
      return MakePreflightErrorResult(-20007, "Cannot import an empty dataset path");
    }

    // Read dataset into DREAM.3D structure
    auto datasetReader = h5FileReader.openDataset(datasetPath);

    std::vector<usize> dims = datasetReader.getDimensions();
    std::string objectName = datasetReader.getName();

    if(dims.empty())
    {
      return MakePreflightErrorResult(-20008, fmt::format("Error reading dimensions from dataset with path '{}'", datasetPath));
    }

    std::string cDimsStr = datasetImportInfo.componentDimensions;
    if(cDimsStr.empty())
    {
      return MakePreflightErrorResult(
          -20009,
          fmt::format("The component dimensions are empty for dataset with path '{}'.  Please enter the component dimensions, using comma-separated values (ex: 4x2 would be '4, 2').", datasetPath));
    }

    auto componentDimensions = ConvertDimensions(cDimsStr, fmt::format("Component for dataset '{}'", datasetPath), ZeroDimensionPolicy::Reject, -20010);
    if(componentDimensions.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(componentDimensions))};
    }
    ShapeType cDims = std::move(componentDimensions.value());

    ShapeType tDims;
    if(parentAM == nullptr)
    {
      std::string tDimsStr = datasetImportInfo.tupleDimensions;
      if(tDimsStr.empty())
      {
        return MakePreflightErrorResult(
            -20011, fmt::format("The tuple dimensions are empty for dataset with path '{}'.  Please enter the tuple dimensions, using comma-separated values (ex: 4x2 would be '4, 2').", datasetPath));
      }

      auto tupleDimensions = ConvertDimensions(tDimsStr, fmt::format("Tuple for dataset '{}'", datasetPath), ZeroDimensionPolicy::Allow, -20012);
      if(tupleDimensions.invalid())
      {
        return {ConvertInvalidResult<OutputActions>(std::move(tupleDimensions))};
      }
      tDims = std::move(tupleDimensions.value());
    }
    else
    {
      tDims = parentAM->getShape();
    }

    auto tupleCount = ComputeDimensionProduct(tDims, fmt::format("Tuple for dataset '{}'", datasetPath));
    if(tupleCount.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(tupleCount))};
    }
    auto componentCount = ComputeDimensionProduct(cDims, fmt::format("Component for dataset '{}'", datasetPath));
    if(componentCount.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(componentCount))};
    }
    const usize numOfTuples = tupleCount.value();
    const usize totalComponents = componentCount.value();
    if(numOfTuples != 0 && totalComponents > std::numeric_limits<usize>::max() / numOfTuples)
    {
      return MakePreflightErrorResult(DimensionConversion::k_ElementCountOverflow, fmt::format("Tuple count {} times component count {} for dataset '{}' exceeds {} elements. Use smaller dimensions.",
                                                                                               numOfTuples, totalComponents, datasetPath, std::numeric_limits<usize>::max()));
    }
    const usize userEnteredTotalElements = numOfTuples * totalComponents;
    // The file reader supplies the dataset dimensions; user shape validation does not change that contract.
    const usize hdf5TotalElements = std::accumulate(dims.begin(), dims.end(), usize{1}, std::multiplies<>());
    std::stringstream stream;
    stream << fmt::format("HDF5 File Path: '{}'\nHDF5 Dataset Path: '{}'\n", inputFile, datasetPath);
    stream << fmt::format("HDF5 dimensions: [{}], element count: {}\n", StringUtilities::formatDimensions(dims), hdf5TotalElements);
    stream << fmt::format("Tuple dimensions: [{}], tuple count: {}\n", StringUtilities::formatDimensions(tDims), numOfTuples);
    stream << fmt::format("Component dimensions: [{}], component count: {}\n", StringUtilities::formatDimensions(cDims), totalComponents);

    if(hdf5TotalElements != userEnteredTotalElements)
    {
      stream << fmt::format("The dataset with path '{}' cannot be read in because '{}' "
                            "tuples and '{}' components per tuple equals '{}' total elements, and"
                            " that does not match the total HDF5 dataset element count of '{}'.\n"
                            "'{}' =/= '{}'",
                            datasetPath, StringUtilities::number(numOfTuples), StringUtilities::number(totalComponents), StringUtilities::number(numOfTuples * totalComponents),
                            StringUtilities::number(hdf5TotalElements), StringUtilities::number(numOfTuples * totalComponents), StringUtilities::number(hdf5TotalElements));
      return MakePreflightErrorResult(-20013, stream.str());
    }

    // Avoid including "/" as an object name within AttributeMatrix.
    // if(pSelectedAttributeMatrixValue.has_value() && objectName.find('/') != std::string::npos)
    //{
    //  objectName = objectName.substr(objectName.find_last_of('/') + 1);
    //}
    DataPath dataArrayPath = pSelectedAttributeMatrixValue.has_value() ? pSelectedAttributeMatrixValue.value().createChildPath(objectName) : DataPath::FromString(objectName).value();

    if(dataStructure.getId(dataArrayPath).has_value())
    {
      stream.clear();
      stream << "The selected dataset '" << pSelectedAttributeMatrixValue.value().toString() << "/" << objectName << "' already exists.";
      return MakePreflightErrorResult(-20014, stream.str());
    }

    Result<DataType> typeResult = datasetReader.getDataType();
    if(typeResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(typeResult))};
    }
    DataType dataType = typeResult.value();
    auto action = std::make_unique<CreateArrayAction>(dataType, tDims, cDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(action));

  } // End For Loop over dataset import info list

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadHDF5DatasetFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadHDF5DatasetInputValues inputValues;
  inputValues.ImportHdf5Object = filterArgs.value<ReadHDF5DatasetParameter::ValueType>(k_ImportHDF5File_Key);

  return ReadHDF5Dataset(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_HDF5FilePathKey = "HDF5FilePath";
constexpr StringLiteral k_ImportInfoArrayKey = "Dataset Import Info Array";
constexpr StringLiteral k_SelectedAttributeMatrixKey = "SelectedAttributeMatrix";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadHDF5DatasetFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadHDF5DatasetFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::ImportHDF5DatasetFilterParameterConverter>(args, json, SIMPL::k_ImportInfoArrayKey, SIMPL::k_HDF5FilePathKey,
                                                                                                                    SIMPL::k_SelectedAttributeMatrixKey, k_ImportHDF5File_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
