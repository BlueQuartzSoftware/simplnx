#include "SimplnxCore/Filters/Hdf5StackReaderFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DataStackParameter.hpp"
#include "simplnx/Parameters/ReadHDF5FileListParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <numeric>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr ChoicesParameter::ValueType k_SingleFileMode = 0;
constexpr ChoicesParameter::ValueType k_MultipleFilesMode = 1;

constexpr int32 k_EmptyParameterError = -123;
constexpr int32 k_SliceShapeError = -8203;
constexpr int32 k_SliceDimsMismatch = -8223;
constexpr int32 k_DataStackFileMissing = -787;
constexpr int32 k_DataStackBadIndexRange = -788;
constexpr int32 k_DataStackDatasetMissing = -790;
constexpr int32 k_FileListDatasetMissing = -4107;

const std::string k_MontageName = "Montage";
const std::string k_GeometryName = "Geometry";
const std::string k_MatrixName = "Cell Data";
const std::string k_ArrayName = "Stack Data";

const fs::path k_TestOutputDir = fs::path(fmt::format("{}", unit_test::k_BinaryTestOutputDir)) / "Hdf5StackReaderTest";

// -----------------------------------------------------------------------------
// Creates (or recreates) an empty scratch directory for a single test case.
fs::path MakeCleanDir(const std::string& name)
{
  fs::path dir = k_TestOutputDir / name;
  if(fs::exists(dir))
  {
    fs::remove_all(dir);
  }
  fs::create_directories(dir);
  return dir;
}

// -----------------------------------------------------------------------------
usize Product(const std::vector<usize>& dims)
{
  return std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>());
}

// -----------------------------------------------------------------------------
// Deterministic, per-slice-unique value for element `elementIndex` of slice `sliceIndex`.
// Kept small so it fits in uint8 for the test sizes used here.
template <typename T>
T SliceValue(usize sliceIndex, usize elementIndex)
{
  return static_cast<T>(sliceIndex * 64 + elementIndex);
}

// -----------------------------------------------------------------------------
template <typename T>
void WriteSliceDataset(HDF5::GroupIO& parentGroup, const std::string& name, const std::vector<usize>& dims, usize sliceIndex)
{
  std::vector<T> values(Product(dims));
  for(usize i = 0; i < values.size(); i++)
  {
    values[i] = SliceValue<T>(sliceIndex, i);
  }
  auto datasetWriter = parentGroup.createDataset(name);
  Result<> writeResult = datasetWriter.writeSpan(dims, nonstd::span<const T>{values});
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
}

// -----------------------------------------------------------------------------
// Writes `sliceDims.size()` datasets named "Slices/slice_000", "Slices/slice_001", ...
// into a single HDF5 file. Each entry of `sliceDims` is the HDF5 shape of that slice.
template <typename T>
void WriteSingleFileStack(const fs::path& filePath, const std::vector<std::vector<usize>>& sliceDims)
{
  HDF5::FileIO fileWriter = HDF5::FileIO::WriteFile(filePath);
  REQUIRE(fileWriter.isValid());
  auto slicesGroup = fileWriter.createGroup("Slices");
  REQUIRE(slicesGroup.isValid());
  for(usize i = 0; i < sliceDims.size(); i++)
  {
    WriteSliceDataset<T>(slicesGroup, fmt::format("slice_{:03}", i), sliceDims[i], i);
  }
}

// -----------------------------------------------------------------------------
// Writes one HDF5 file per slice ("slice_00.h5", "slice_01.h5", ...) into `dir`,
// each holding its slice at "Data/Image".
template <typename T>
void WriteMultiFileStack(const fs::path& dir, usize numFiles, const std::vector<usize>& dims)
{
  for(usize i = 0; i < numFiles; i++)
  {
    HDF5::FileIO fileWriter = HDF5::FileIO::WriteFile(dir / fmt::format("slice_{:02}.h5", i));
    REQUIRE(fileWriter.isValid());
    auto dataGroup = fileWriter.createGroup("Data");
    REQUIRE(dataGroup.isValid());
    WriteSliceDataset<T>(dataGroup, "Image", dims, i);
  }
}

// -----------------------------------------------------------------------------
ReadHDF5DataStackParameter::ValueType MakeDataStackValue(const fs::path& filePath, uint32 startIndex, uint32 endIndex)
{
  ReadHDF5DataStackParameter::ValueType value;
  value.inputPath = filePath.string();
  value.pathPrefix = "Slices/slice_";
  value.pathSuffix = "";
  value.paddingDigits = 3;
  value.startIndex = startIndex;
  value.endIndex = endIndex;
  return value;
}

// -----------------------------------------------------------------------------
ReadHDF5FileListParameter::ValueType MakeFileListValue(const fs::path& dir, int32 startIndex, int32 endIndex, ReadHDF5FileListParameter::Ordering ordering)
{
  ReadHDF5FileListParameter::ValueType value;
  value.inputPath = dir.string();
  value.filePrefix = "slice_";
  value.fileSuffix = "";
  value.fileExtension = ".h5";
  value.startIndex = startIndex;
  value.endIndex = endIndex;
  value.incrementIndex = 1;
  value.paddingDigits = 2;
  value.ordering = ordering;
  value.datasetPath = "Data/Image";
  return value;
}

// -----------------------------------------------------------------------------
Arguments MakeArgs(ChoicesParameter::ValueType inputMode, const ReadHDF5DataStackParameter::ValueType& dataStack, const ReadHDF5FileListParameter::ValueType& fileList, bool useMontage,
                   NumericType numericType)
{
  Arguments args;
  args.insertOrAssign(Hdf5StackReaderFilter::k_InputModeKey, std::make_any<ChoicesParameter::ValueType>(inputMode));
  args.insertOrAssign(Hdf5StackReaderFilter::k_H5DataStack_Key, std::make_any<ReadHDF5DataStackParameter::ValueType>(dataStack));
  args.insertOrAssign(Hdf5StackReaderFilter::k_H5FileList_Key, std::make_any<ReadHDF5FileListParameter::ValueType>(fileList));
  args.insertOrAssign(Hdf5StackReaderFilter::k_MontageName_Key, std::make_any<std::string>(k_MontageName));
  args.insertOrAssign(Hdf5StackReaderFilter::k_GeometryName_Key, std::make_any<std::string>(k_GeometryName));
  args.insertOrAssign(Hdf5StackReaderFilter::k_CellMatrixName_Key, std::make_any<std::string>(k_MatrixName));
  args.insertOrAssign(Hdf5StackReaderFilter::k_OutputDataName_Key, std::make_any<std::string>(k_ArrayName));
  args.insertOrAssign(Hdf5StackReaderFilter::k_NumericType_Key, std::make_any<NumericType>(numericType));
  args.insertOrAssign(Hdf5StackReaderFilter::k_GeomOutputType_Key, std::make_any<bool>(useMontage));
  return args;
}

// -----------------------------------------------------------------------------
Arguments MakeSingleFileArgs(const ReadHDF5DataStackParameter::ValueType& dataStack, bool useMontage = false, NumericType numericType = NumericType::int32)
{
  return MakeArgs(k_SingleFileMode, dataStack, ReadHDF5FileListParameter::ValueType{}, useMontage, numericType);
}

// -----------------------------------------------------------------------------
Arguments MakeFileListArgs(const ReadHDF5FileListParameter::ValueType& fileList, bool useMontage = false, NumericType numericType = NumericType::int32)
{
  ReadHDF5DataStackParameter::ValueType emptyDataStack{};
  return MakeArgs(k_MultipleFilesMode, emptyDataStack, fileList, useMontage, numericType);
}

// -----------------------------------------------------------------------------
bool HasErrorCode(const std::vector<Error>& errors, int32 code)
{
  return std::any_of(errors.cbegin(), errors.cend(), [code](const Error& error) { return error.code == code; });
}

// -----------------------------------------------------------------------------
// Checks the single stacked geometry/array created in non-montage mode. `sliceOrder[z]`
// is the source slice index expected to land at Z index `z`.
template <typename SrcT, typename DestT>
void VerifyStackedOutput(const DataStructure& dataStructure, usize dimY, usize dimX, const std::vector<usize>& componentDims, const std::vector<usize>& sliceOrder)
{
  const usize numSlices = sliceOrder.size();
  const DataPath geomPath({k_GeometryName});
  const DataPath arrayPath = geomPath.createChildPath(k_MatrixName).createChildPath(k_ArrayName);

  const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(geomPath);
  REQUIRE(imageGeom != nullptr);
  REQUIRE(imageGeom->getDimensions() == SizeVec3{dimX, dimY, numSlices});

  const auto* dataArray = dataStructure.getDataAs<DataArray<DestT>>(arrayPath);
  REQUIRE(dataArray != nullptr);
  REQUIRE(dataArray->getTupleShape() == std::vector<usize>{numSlices, dimY, dimX});
  REQUIRE(dataArray->getComponentShape() == componentDims);

  const auto& store = dataArray->getDataStoreRef();
  const usize elementsPerSlice = dimY * dimX * Product(componentDims);
  REQUIRE(store.getSize() == elementsPerSlice * numSlices);
  for(usize z = 0; z < numSlices; z++)
  {
    for(usize e = 0; e < elementsPerSlice; e++)
    {
      const auto expected = static_cast<DestT>(SliceValue<SrcT>(sliceOrder[z], e));
      REQUIRE(store.getValue(z * elementsPerSlice + e) == expected);
    }
  }
}

// -----------------------------------------------------------------------------
// Checks the GridMontage and its per-slice geometries/arrays created in montage mode.
template <typename SrcT, typename DestT>
void VerifyMontageOutput(const DataStructure& dataStructure, usize dimY, usize dimX, const std::vector<usize>& componentDims, usize numSlices)
{
  const DataPath montagePath({k_MontageName});
  const auto* montage = dataStructure.getDataAs<GridMontage>(montagePath);
  REQUIRE(montage != nullptr);
  REQUIRE(montage->getGridSize() == SizeVec3{1, 1, numSlices});

  // No single stacked geometry should be created in montage mode
  REQUIRE(dataStructure.getDataAs<ImageGeom>(DataPath({k_GeometryName})) == nullptr);

  const usize elementsPerSlice = dimY * dimX * Product(componentDims);
  for(usize i = 0; i < numSlices; i++)
  {
    const DataPath geomPath = montagePath.createChildPath(fmt::format("{}_{}", k_GeometryName, i));
    const auto* imageGeom = dataStructure.getDataAs<ImageGeom>(geomPath);
    REQUIRE(imageGeom != nullptr);
    REQUIRE(imageGeom->getDimensions() == SizeVec3{dimX, dimY, 1});
    REQUIRE(montage->getGeometry(SizeVec3{0, 0, i}) == imageGeom);

    const auto* dataArray = dataStructure.getDataAs<DataArray<DestT>>(geomPath.createChildPath(k_MatrixName).createChildPath(k_ArrayName));
    REQUIRE(dataArray != nullptr);
    REQUIRE(dataArray->getTupleShape() == std::vector<usize>{1, dimY, dimX});
    REQUIRE(dataArray->getComponentShape() == componentDims);

    const auto& store = dataArray->getDataStoreRef();
    REQUIRE(store.getSize() == elementsPerSlice);
    for(usize e = 0; e < elementsPerSlice; e++)
    {
      REQUIRE(store.getValue(e) == static_cast<DestT>(SliceValue<SrcT>(i, e)));
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::Hdf5StackReaderFilter: Single File Stack", "[SimplnxCore][Hdf5StackReaderFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = MakeCleanDir("SingleFileStack");
  const fs::path filePath = dir / "stack.h5";
  constexpr usize k_DimY = 4;
  constexpr usize k_DimX = 5;
  constexpr usize k_NumSlices = 3;

  Hdf5StackReaderFilter filter;

  SECTION("Scalar int32 slices into int32 array")
  {
    WriteSingleFileStack<int32>(filePath, std::vector<std::vector<usize>>(k_NumSlices, {k_DimY, k_DimX}));

    DataStructure dataStructure;
    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, k_NumSlices - 1), false, NumericType::int32);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyStackedOutput<int32, int32>(dataStructure, k_DimY, k_DimX, {1}, {0, 1, 2});
  }

  SECTION("Multi-component uint8 slices cast into float32 array")
  {
    WriteSingleFileStack<uint8>(filePath, std::vector<std::vector<usize>>(k_NumSlices, {k_DimY, k_DimX, 3}));

    DataStructure dataStructure;
    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, k_NumSlices - 1), false, NumericType::float32);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyStackedOutput<uint8, float32>(dataStructure, k_DimY, k_DimX, {3}, {0, 1, 2});
  }

  SECTION("Sub-range of the dataset indices")
  {
    WriteSingleFileStack<int32>(filePath, std::vector<std::vector<usize>>(k_NumSlices, {k_DimY, k_DimX}));

    DataStructure dataStructure;
    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 1, 2), false, NumericType::int32);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyStackedOutput<int32, int32>(dataStructure, k_DimY, k_DimX, {1}, {1, 2});
  }

  SECTION("Montage output")
  {
    WriteSingleFileStack<int16>(filePath, std::vector<std::vector<usize>>(k_NumSlices, {k_DimY, k_DimX, 2}));

    DataStructure dataStructure;
    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, k_NumSlices - 1), true, NumericType::int64);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyMontageOutput<int16, int64>(dataStructure, k_DimY, k_DimX, {2}, k_NumSlices);
  }

  fs::remove_all(dir);
}

// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::Hdf5StackReaderFilter: Multiple File Stack", "[SimplnxCore][Hdf5StackReaderFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = MakeCleanDir("MultipleFileStack");
  constexpr usize k_DimY = 3;
  constexpr usize k_DimX = 6;
  constexpr usize k_NumFiles = 3;

  WriteMultiFileStack<float64>(dir, k_NumFiles, {k_DimY, k_DimX});

  Hdf5StackReaderFilter filter;

  SECTION("Low to high ordering")
  {
    DataStructure dataStructure;
    Arguments args = MakeFileListArgs(MakeFileListValue(dir, 0, k_NumFiles - 1, ReadHDF5FileListParameter::Ordering::LowToHigh), false, NumericType::float64);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyStackedOutput<float64, float64>(dataStructure, k_DimY, k_DimX, {1}, {0, 1, 2});
  }

  SECTION("High to low ordering reverses the slice order")
  {
    DataStructure dataStructure;
    Arguments args = MakeFileListArgs(MakeFileListValue(dir, 0, k_NumFiles - 1, ReadHDF5FileListParameter::Ordering::HighToLow), false, NumericType::float64);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyStackedOutput<float64, float64>(dataStructure, k_DimY, k_DimX, {1}, {2, 1, 0});
  }

  SECTION("Montage output")
  {
    DataStructure dataStructure;
    Arguments args = MakeFileListArgs(MakeFileListValue(dir, 0, k_NumFiles - 1, ReadHDF5FileListParameter::Ordering::LowToHigh), true, NumericType::float32);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    VerifyMontageOutput<float64, float32>(dataStructure, k_DimY, k_DimX, {1}, k_NumFiles);
  }

  fs::remove_all(dir);
}

// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::Hdf5StackReaderFilter: Invalid Parameters", "[SimplnxCore][Hdf5StackReaderFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = MakeCleanDir("InvalidParameters");
  const fs::path filePath = dir / "stack.h5";
  WriteSingleFileStack<int32>(filePath, std::vector<std::vector<usize>>(2, {4, 5}));

  Hdf5StackReaderFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, 1));

  SECTION("Empty geometry name")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_GeometryName_Key, std::make_any<std::string>(""));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_EmptyParameterError));
  }

  SECTION("Empty cell attribute matrix name")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_CellMatrixName_Key, std::make_any<std::string>(""));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_EmptyParameterError));
  }

  SECTION("Empty data array name")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_OutputDataName_Key, std::make_any<std::string>(""));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_EmptyParameterError));
  }

  SECTION("Empty montage name is an error only when creating a montage")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_MontageName_Key, std::make_any<std::string>(""));

    auto noMontageResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(noMontageResult.outputActions);

    args.insertOrAssign(Hdf5StackReaderFilter::k_GeomOutputType_Key, std::make_any<bool>(true));
    auto montageResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(montageResult.outputActions);
    REQUIRE(HasErrorCode(montageResult.outputActions.errors(), k_EmptyParameterError));
  }

  SECTION("HDF5 file does not exist")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_H5DataStack_Key, std::make_any<ReadHDF5DataStackParameter::ValueType>(MakeDataStackValue(dir / "missing.h5", 0, 1)));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_DataStackFileMissing));
  }

  SECTION("Start index greater than end index")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_H5DataStack_Key, std::make_any<ReadHDF5DataStackParameter::ValueType>(MakeDataStackValue(filePath, 1, 0)));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_DataStackBadIndexRange));
  }

  SECTION("Dataset index range exceeds the datasets in the file")
  {
    args.insertOrAssign(Hdf5StackReaderFilter::k_H5DataStack_Key, std::make_any<ReadHDF5DataStackParameter::ValueType>(MakeDataStackValue(filePath, 0, 2)));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_DataStackDatasetMissing));
  }

  SECTION("Dataset missing from one file in a file list")
  {
    const fs::path fileListDir = dir / "file_list";
    fs::create_directories(fileListDir);
    WriteMultiFileStack<int32>(fileListDir, 2, {4, 5});
    {
      // Third file exists but holds its data under a different dataset path
      HDF5::FileIO fileWriter = HDF5::FileIO::WriteFile(fileListDir / "slice_02.h5");
      REQUIRE(fileWriter.isValid());
      WriteSliceDataset<int32>(fileWriter, "WrongName", {4, 5}, 2);
    }

    Arguments fileListArgs = MakeFileListArgs(MakeFileListValue(fileListDir, 0, 2, ReadHDF5FileListParameter::Ordering::LowToHigh));
    auto preflightResult = filter.preflight(dataStructure, fileListArgs);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_FileListDatasetMissing));
  }

  fs::remove_all(dir);
}

// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::Hdf5StackReaderFilter: Invalid Slice Shapes", "[SimplnxCore][Hdf5StackReaderFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = MakeCleanDir("InvalidSliceShapes");
  const fs::path filePath = dir / "stack.h5";

  Hdf5StackReaderFilter filter;
  DataStructure dataStructure;

  SECTION("1D dataset cannot be treated as a slice")
  {
    WriteSingleFileStack<int32>(filePath, {{20}, {20}});

    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, 1));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_SliceShapeError));
  }

  SECTION("Slice XY dimensions do not match")
  {
    WriteSingleFileStack<int32>(filePath, {{4, 5}, {4, 6}});

    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, 1));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_SliceDimsMismatch));

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  }

  SECTION("Slice component dimensions do not match")
  {
    WriteSingleFileStack<int32>(filePath, {{4, 5, 3}, {4, 5}});

    Arguments args = MakeSingleFileArgs(MakeDataStackValue(filePath, 0, 1));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_SliceDimsMismatch));
  }

  SECTION("Slice dimensions do not match across a file list")
  {
    const fs::path fileListDir = dir / "file_list";
    fs::create_directories(fileListDir);
    WriteMultiFileStack<int32>(fileListDir, 2, {4, 5});
    {
      HDF5::FileIO fileWriter = HDF5::FileIO::WriteFile(fileListDir / "slice_02.h5");
      REQUIRE(fileWriter.isValid());
      auto dataGroup = fileWriter.createGroup("Data");
      WriteSliceDataset<int32>(dataGroup, "Image", {5, 4}, 2);
    }

    Arguments args = MakeFileListArgs(MakeFileListValue(fileListDir, 0, 2, ReadHDF5FileListParameter::Ordering::LowToHigh));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(HasErrorCode(preflightResult.outputActions.errors(), k_SliceDimsMismatch));
  }

  fs::remove_all(dir);
}
