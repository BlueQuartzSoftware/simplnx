#include "Hdf5StackReader.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/format.h>

#include <numeric>
#include <optional>

using namespace nx::core;

namespace
{
constexpr int32 k_FileOpenError = -8200;
constexpr int32 k_NoDatasetsError = -8201;
constexpr int32 k_SliceShapeError = -8203;
constexpr int32 k_SliceDimsMismatch = -8204;
constexpr int32 k_UnsupportedSourceType = -8205;
constexpr int32 k_MontageLookupError = -8206;
constexpr int32 k_GeometryLookupError = -8207;

// -----------------------------------------------------------------------------
// Generates the ordered list of HDF5 dataset paths for a numbered slice stack:
// pathPrefix + zero-padded index (paddingDigits wide) + pathSuffix, for each
// index in [startIndex, endIndex] inclusive. Only used by the "single file,
// multiple datasets" input mode.
std::vector<std::string> GenerateHdf5StackDatasetPaths(const std::string& pathPrefix, const std::string& pathSuffix, uint32 startIndex, uint32 endIndex, uint8 paddingDigits)
{
  std::vector<std::string> paths;
  if(startIndex > endIndex)
  {
    return paths;
  }
  paths.reserve(static_cast<usize>(endIndex - startIndex) + 1);
  for(uint32 index = startIndex; index <= endIndex; ++index)
  {
    paths.push_back(fmt::format("{}{:0{}}{}", pathPrefix, index, paddingDigits, pathSuffix));
  }
  return paths;
}

// -----------------------------------------------------------------------------
// Reads the raw values of one slice dataset (as SrcT) and casts them into the
// destination store, starting at destElementOffset.
template <typename SrcT, typename DestT>
Result<> CastCopySlice(const HDF5::DatasetIO& datasetReader, AbstractDataStore<DestT>& destStore, usize destElementOffset, usize numElements)
{
  std::vector<SrcT> buffer(numElements);
  Result<> readResult = datasetReader.readIntoSpan<SrcT>(nonstd::span<SrcT>(buffer.data(), buffer.size()));
  if(readResult.invalid())
  {
    return readResult;
  }

  for(usize i = 0; i < numElements; i++)
  {
    if constexpr(std::is_same_v<SrcT, DestT>)
    {
      destStore.setValue(destElementOffset + i, buffer[i]);
    }
    else
    {
      destStore.setValue(destElementOffset + i, static_cast<DestT>(buffer[i]));
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
// Dispatches on the HDF5 dataset's native (source) DataType.
template <typename DestT>
Result<> ReadSliceInto(const HDF5::DatasetIO& datasetReader, DataType srcType, AbstractDataStore<DestT>& destStore, usize destElementOffset, usize numElements, const std::string& datasetPath)
{
  switch(srcType)
  {
  case DataType::int8:
    return CastCopySlice<int8, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::uint8:
    return CastCopySlice<uint8, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::int16:
    return CastCopySlice<int16, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::uint16:
    return CastCopySlice<uint16, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::int32:
    return CastCopySlice<int32, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::uint32:
    return CastCopySlice<uint32, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::int64:
    return CastCopySlice<int64, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::uint64:
    return CastCopySlice<uint64, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::float32:
    return CastCopySlice<float32, DestT>(datasetReader, destStore, destElementOffset, numElements);
  case DataType::float64:
    return CastCopySlice<float64, DestT>(datasetReader, destStore, destElementOffset, numElements);
  default:
    return MakeErrorResult(k_UnsupportedSourceType, fmt::format("Dataset '{}' has an unsupported HDF5 data type '{}' for reading.", datasetPath, DataTypeToString(srcType)));
  }
}

// -----------------------------------------------------------------------------
// Dispatches on the destination (user-chosen output) NumericType.
struct ReadSliceDispatchFunctor
{
  template <typename DestT>
  Result<> operator()(DataStructure& dataStructure, const DataPath& arrayPath, usize destTupleOffset, const HDF5::DatasetIO& datasetReader, DataType srcType, usize numElements,
                      const std::string& datasetPath) const
  {
    auto& destArray = dataStructure.getDataRefAs<DataArray<DestT>>(arrayPath);
    AbstractDataStore<DestT>& destStore = destArray.getDataStoreRef();
    usize destElementOffset = destTupleOffset * destStore.getNumberOfComponents();
    return ReadSliceInto<DestT>(datasetReader, srcType, destStore, destElementOffset, numElements, datasetPath);
  }
};
} // namespace

namespace nx::core
{
// -----------------------------------------------------------------------------
std::vector<Hdf5StackSliceSource> GenerateSliceSourcesFromDataStack(const ReadHDF5DataStackParameter::ValueType& dataStackInfo)
{
  std::vector<std::string> datasetPaths = GenerateHdf5StackDatasetPaths(dataStackInfo.pathPrefix, dataStackInfo.pathSuffix, dataStackInfo.startIndex, dataStackInfo.endIndex, dataStackInfo.paddingDigits);

  std::vector<Hdf5StackSliceSource> sources;
  sources.reserve(datasetPaths.size());
  for(auto& datasetPath : datasetPaths)
  {
    sources.push_back(Hdf5StackSliceSource{dataStackInfo.inputPath, std::move(datasetPath)});
  }
  return sources;
}

// -----------------------------------------------------------------------------
std::vector<Hdf5StackSliceSource> GenerateSliceSourcesFromFileList(const ReadHDF5FileListParameter::ValueType& fileListInfo)
{
  std::vector<std::string> filePaths = fileListInfo.generate();

  std::vector<Hdf5StackSliceSource> sources;
  sources.reserve(filePaths.size());
  for(auto& filePath : filePaths)
  {
    sources.push_back(Hdf5StackSliceSource{std::move(filePath), fileListInfo.datasetPath});
  }
  return sources;
}

// -----------------------------------------------------------------------------
Result<Hdf5StackSliceShape> ReadHdf5StackSliceShape(const HDF5::DatasetIO& datasetReader, const std::string& datasetPath)
{
  std::vector<usize> dims = datasetReader.getDimensions();
  if(dims.size() < 2)
  {
    return MakeErrorResult<Hdf5StackSliceShape>(
        k_SliceShapeError, fmt::format("Dataset '{}' has {} dimension(s); at least 2 are required to be treated as a 2D slice (with any remaining dimensions treated as component shape).", datasetPath,
                                       dims.size()));
  }

  Hdf5StackSliceShape shape;
  shape.dimY = dims[0];
  shape.dimX = dims[1];
  shape.componentDims = dims.size() > 2 ? std::vector<usize>(dims.begin() + 2, dims.end()) : std::vector<usize>{1};
  return {shape};
}

// -----------------------------------------------------------------------------
Hdf5StackReader::Hdf5StackReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Hdf5StackReaderInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
Hdf5StackReader::~Hdf5StackReader() noexcept = default;

// -----------------------------------------------------------------------------
Result<> Hdf5StackReader::operator()()
{
  const auto& inputValues = *m_InputValues;
  const std::vector<Hdf5StackSliceSource>& sliceSources = inputValues.sliceSources;

  if(sliceSources.empty())
  {
    return MakeErrorResult(k_NoDatasetsError, "No slices were resolved for this HDF5 stack.");
  }

  DataType destDataType = ConvertNumericTypeToDataType(inputValues.numericType);

  DataPath montagePath({inputValues.montageName});
  GridMontage* gridMontage = nullptr;
  if(inputValues.useMontage)
  {
    gridMontage = m_DataStructure.getDataAs<GridMontage>(montagePath);
    if(gridMontage == nullptr)
    {
      return MakeErrorResult(k_MontageLookupError, fmt::format("Could not find GridMontage at path '{}'", montagePath.toString()));
    }
    gridMontage->resizeTileDims(1, 1, sliceSources.size());
  }

  DataPath singleGeomPath({inputValues.geometryName});
  DataPath singleArrayPath = singleGeomPath.createChildPath(inputValues.matrixName).createChildPath(inputValues.dataArrayName);

  Hdf5StackSliceShape referenceShape;
  usize numTuplesPerSlice = 0;
  usize numElementsPerSlice = 0;

  // Slices may all come from one file (dataset-stack mode, filePath never changes) or one
  // per file (file-list mode, filePath changes every slice) -- only reopen the file when the
  // path actually changes, so the common dataset-stack case still opens the file exactly once.
  std::optional<HDF5::FileIO> currentFile;
  std::string currentFilePath;

  for(usize i = 0; i < sliceSources.size(); i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const Hdf5StackSliceSource& source = sliceSources[i];
    m_MessageHandler.sendMessage(IFilter::Message::Type::Info, fmt::format("Reading slice {}/{}: '{}' :: '{}'", i + 1, sliceSources.size(), source.filePath, source.datasetPath));

    if(!currentFile.has_value() || source.filePath != currentFilePath)
    {
      currentFile.emplace(HDF5::FileIO::ReadFile(source.filePath));
      currentFilePath = source.filePath;
      if(!currentFile->isValid())
      {
        return MakeErrorResult(k_FileOpenError, fmt::format("Could not open HDF5 file '{}'", source.filePath));
      }
    }

    auto datasetReader = currentFile->openDataset(source.datasetPath);

    Result<Hdf5StackSliceShape> shapeResult = ReadHdf5StackSliceShape(datasetReader, source.datasetPath);
    if(shapeResult.invalid())
    {
      return ConvertResult(std::move(shapeResult));
    }
    Hdf5StackSliceShape shape = std::move(shapeResult.value());

    if(i == 0)
    {
      referenceShape = shape;
      numTuplesPerSlice = referenceShape.dimY * referenceShape.dimX;
      numElementsPerSlice = numTuplesPerSlice * std::accumulate(referenceShape.componentDims.begin(), referenceShape.componentDims.end(), static_cast<usize>(1), std::multiplies<usize>());
    }
    else if(shape != referenceShape)
    {
      return MakeErrorResult(k_SliceDimsMismatch,
                             fmt::format("Dataset '{}' in file '{}' has dimensions that do not match the first slice in the stack.", source.datasetPath, source.filePath));
    }

    Result<DataType> srcTypeResult = datasetReader.getDataType();
    if(srcTypeResult.invalid())
    {
      return ConvertResult(std::move(srcTypeResult));
    }
    DataType srcType = srcTypeResult.value();

    if(inputValues.useMontage)
    {
      DataPath geomPath = montagePath.createChildPath(fmt::format("{}_{}", inputValues.geometryName, i));
      DataPath arrayPath = geomPath.createChildPath(inputValues.matrixName).createChildPath(inputValues.dataArrayName);

      Result<> readResult = ExecuteDataFunctionNoBool(ReadSliceDispatchFunctor{}, destDataType, m_DataStructure, arrayPath, usize{0}, datasetReader, srcType, numElementsPerSlice, source.datasetPath);
      if(readResult.invalid())
      {
        return readResult;
      }

      auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(geomPath);
      if(imageGeom == nullptr)
      {
        return MakeErrorResult(k_GeometryLookupError, fmt::format("Could not find ImageGeometry at path '{}'", geomPath.toString()));
      }
      gridMontage->setGeometry(SizeVec3{0, 0, i}, imageGeom);
    }
    else
    {
      usize destTupleOffset = i * numTuplesPerSlice;
      Result<> readResult =
          ExecuteDataFunctionNoBool(ReadSliceDispatchFunctor{}, destDataType, m_DataStructure, singleArrayPath, destTupleOffset, datasetReader, srcType, numElementsPerSlice, source.datasetPath);
      if(readResult.invalid())
      {
        return readResult;
      }
    }
  }

  return {};
}
} // namespace nx::core
