#include "Hdf5StackReaderFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/Hdf5StackReader.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGridMontageAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumericTypeParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DataStackParameter.hpp"
#include "simplnx/Parameters/ReadHDF5FileListParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <optional>

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyParameterError = -123;
constexpr ChoicesParameter::ValueType k_SingleFileInputMode = 0;
constexpr int32 k_FileOpenError = -8220;
constexpr int32 k_NoDatasetsError = -8221;
constexpr int32 k_SliceDimsMismatch = -8223;

// -----------------------------------------------------------------------------
// Resolves the ordered list of (file, dataset) slice sources for whichever
// input parameter is active for the given input mode.
std::vector<Hdf5StackSliceSource> ResolveSliceSources(const Arguments& filterArgs, ChoicesParameter::ValueType inputMode)
{
  if(inputMode == k_SingleFileInputMode)
  {
    auto h5DataStack = filterArgs.value<ReadHDF5DataStackParameter::ValueType>(Hdf5StackReaderFilter::k_H5DataStack_Key);
    return GenerateSliceSourcesFromDataStack(h5DataStack);
  }
  auto h5FileList = filterArgs.value<ReadHDF5FileListParameter::ValueType>(Hdf5StackReaderFilter::k_H5FileList_Key);
  return GenerateSliceSourcesFromFileList(h5FileList);
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string Hdf5StackReaderFilter::name() const
{
  return FilterTraits<Hdf5StackReaderFilter>::name;
}

//------------------------------------------------------------------------------
std::string Hdf5StackReaderFilter::className() const
{
  return FilterTraits<Hdf5StackReaderFilter>::className;
}

//------------------------------------------------------------------------------
Uuid Hdf5StackReaderFilter::uuid() const
{
  return FilterTraits<Hdf5StackReaderFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string Hdf5StackReaderFilter::humanName() const
{
  return "HDF5 Stack Reader";
}

//------------------------------------------------------------------------------
std::vector<std::string> Hdf5StackReaderFilter::defaultTags() const
{
  return {className(), "HDF5", "Data Structure", "Stack", "Reader"};
}

//------------------------------------------------------------------------------
Parameters Hdf5StackReaderFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"HDF5 Stack"});
  ChoicesParameter::Choices inputModeChoices({"Single File (Multiple Datasets)", "Multiple Files (Same Dataset)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_InputModeKey, "Input Mode", "Whether to read multiple datasets out of a single HDF5 file, or the same dataset path out of multiple HDF5 files.", k_SingleFileInputMode, inputModeChoices));
  params.insert(std::make_unique<ReadHDF5DataStackParameter>(k_H5DataStack_Key, "Read HDF5 Data Stack", "HDF5 datasets to read, all from a single HDF5 file.", ReadHDF5DataStackParameter::ValueType()));
  params.insert(
      std::make_unique<ReadHDF5FileListParameter>(k_H5FileList_Key, "Read HDF5 File List", "HDF5 files to read, all using the same dataset path.", ReadHDF5FileListParameter::ValueType()));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Created Montage Name", "Montage storing individual slice data", "[Stack Montage]"));
  params.insert(std::make_unique<StringParameter>(k_GeometryName_Key, "Created Geometry Name", "Geometry storing the slice or stack data", "[Stack Geometry]"));
  params.insert(std::make_unique<StringParameter>(k_CellMatrixName_Key, "Created Cell Attribute Matrix", "Cell AttributeMatrix to use for each created geometry", "Cell Data"));
  params.insert(std::make_unique<StringParameter>(k_OutputDataName_Key, "Created Data Array", "Imported HDF5 stack data array.", "[Stack Array]"));
  params.insert(std::make_unique<NumericTypeParameter>(k_NumericType_Key, "Output Numeric Type", "Numeric Type of data to create", NumericType::int32));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_GeomOutputType_Key, "Create Montage", "Determines what type of output the filter should use.", false));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_GeomOutputType_Key, k_MontageName_Key, true);
  params.linkParameters(k_InputModeKey, k_H5DataStack_Key, k_SingleFileInputMode);
  params.linkParameters(k_InputModeKey, k_H5FileList_Key, ChoicesParameter::ValueType{1});

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType Hdf5StackReaderFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer Hdf5StackReaderFilter::clone() const
{
  return std::make_unique<Hdf5StackReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult Hdf5StackReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto montageName = filterArgs.value<std::string>(k_MontageName_Key);
  auto geometryName = filterArgs.value<std::string>(k_GeometryName_Key);
  auto matrixName = filterArgs.value<std::string>(k_CellMatrixName_Key);
  auto outputArrayName = filterArgs.value<std::string>(k_OutputDataName_Key);
  auto numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  auto useMontage = filterArgs.value<BoolParameter::ValueType>(k_GeomOutputType_Key);
  auto inputMode = filterArgs.value<ChoicesParameter::ValueType>(k_InputModeKey);

  nx::core::Result<OutputActions> resultOutputActions;

  // Only check the montage name if a montage is created
  if(useMontage && montageName.empty())
  {
    return MakePreflightErrorResult(k_EmptyParameterError, fmt::format("{}: Created Montage Name cannot be empty.", humanName()));
  }
  if(geometryName.empty())
  {
    return MakePreflightErrorResult(k_EmptyParameterError, fmt::format("{}: Created Geometry Name cannot be empty.", humanName()));
  }
  if(matrixName.empty())
  {
    return MakePreflightErrorResult(k_EmptyParameterError, fmt::format("{}: Created Cell Attribute Matrix Name cannot be empty.", humanName()));
  }
  if(outputArrayName.empty())
  {
    return MakePreflightErrorResult(k_EmptyParameterError, fmt::format("{}: Created Data Array Name cannot be empty.", humanName()));
  }

  // Resolve the ordered list of (file, dataset) slice sources from whichever input parameter
  // is active, then read the real dimensions of each slice so the created actions use accurate
  // tuple/component dims instead of placeholders. The active parameter's own validate() has
  // already confirmed its files/datasets exist, but preflight still needs to open them to read
  // the actual per-slice shapes.
  std::vector<Hdf5StackSliceSource> sliceSources = ResolveSliceSources(filterArgs, inputMode);
  if(sliceSources.empty())
  {
    return MakePreflightErrorResult(k_NoDatasetsError, fmt::format("{}: No slices were resolved for this HDF5 stack.", humanName()));
  }

  Hdf5StackSliceShape referenceShape;
  std::optional<HDF5::FileIO> currentFile;
  std::string currentFilePath;

  for(usize i = 0; i < sliceSources.size(); i++)
  {
    const Hdf5StackSliceSource& source = sliceSources[i];

    if(!currentFile.has_value() || source.filePath != currentFilePath)
    {
      currentFile.emplace(HDF5::FileIO::ReadFile(source.filePath));
      currentFilePath = source.filePath;
      if(!currentFile->isValid())
      {
        return MakePreflightErrorResult(k_FileOpenError, fmt::format("{}: Could not open HDF5 file '{}'", humanName(), source.filePath));
      }
    }

    auto datasetReader = currentFile->openDataset(source.datasetPath);

    Result<Hdf5StackSliceShape> shapeResult = ReadHdf5StackSliceShape(datasetReader, source.datasetPath);
    if(shapeResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(shapeResult))};
    }

    if(i == 0)
    {
      referenceShape = shapeResult.value();
    }
    else if(shapeResult.value() != referenceShape)
    {
      return MakePreflightErrorResult(
          k_SliceDimsMismatch, fmt::format("{}: Dataset '{}' in file '{}' has dimensions that do not match the first slice in the stack.", humanName(), source.datasetPath, source.filePath));
    }
  }

  const usize numSlices = sliceSources.size();
  DataType arrayDataType = ConvertNumericTypeToDataType(numericType);
  CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};

  if(useMontage)
  {
    CreateGridMontageAction::DimensionType montageDims = {1, 1, numSlices};
    resultOutputActions.value().appendAction(std::make_unique<CreateGridMontageAction>(DataPath({montageName}), montageDims, origin, spacing));

    for(usize i = 0; i < numSlices; i++)
    {
      DataPath geomPath = DataPath({montageName, fmt::format("{}_{}", geometryName, i)});
      CreateImageGeometryAction::DimensionType geomDims = {referenceShape.dimX, referenceShape.dimY, 1};
      std::vector<usize> tupleDims = {1, referenceShape.dimY, referenceShape.dimX};

      resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(geomPath, geomDims, origin, spacing, matrixName));

      DataPath arrayPath = geomPath.createChildPath(matrixName).createChildPath(outputArrayName);
      resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(arrayDataType, tupleDims, referenceShape.componentDims, arrayPath));
    }
  }
  else
  {
    DataPath geomPath({geometryName});
    CreateImageGeometryAction::DimensionType geomDims = {referenceShape.dimX, referenceShape.dimY, numSlices};
    std::vector<usize> tupleDims = {numSlices, referenceShape.dimY, referenceShape.dimX};

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(geomPath, geomDims, origin, spacing, matrixName));

    DataPath arrayPath = geomPath.createChildPath(matrixName).createChildPath(outputArrayName);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(arrayDataType, tupleDims, referenceShape.componentDims, arrayPath));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> Hdf5StackReaderFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto useMontage = filterArgs.value<BoolParameter::ValueType>(k_GeomOutputType_Key);
  auto inputMode = filterArgs.value<ChoicesParameter::ValueType>(k_InputModeKey);

  Hdf5StackReaderInputValues inputValues;
  inputValues.sliceSources = ResolveSliceSources(filterArgs, inputMode);
  inputValues.montageName = filterArgs.value<std::string>(k_MontageName_Key);
  inputValues.geometryName = filterArgs.value<std::string>(k_GeometryName_Key);
  inputValues.matrixName = filterArgs.value<std::string>(k_CellMatrixName_Key);
  inputValues.dataArrayName = filterArgs.value<std::string>(k_OutputDataName_Key);
  inputValues.numericType = filterArgs.value<NumericType>(k_NumericType_Key);
  inputValues.useMontage = useMontage;

  return Hdf5StackReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
