#pragma once

#include "ReconstructionFilterTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "NXImageProcessing/Filters/BinaryThresholdImageFilter.hpp"
#include "NXImageProcessing/Filters/ConnectedComponentImageFilter.hpp"
#include "NXImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "NXImageProcessing/Filters/WriteImageFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"
#include "SimplnxCore/Filters/ConvertDataFilter.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <cstring>
#include <filesystem>
#include <random>
#include <span>
#include <system_error>

// Small transport helpers only. Each owning test states its independent numerical expectations.
// NOLINTBEGIN(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while) Catch2 and simplnx assertion macros trigger these checks.
namespace c15_test
{
using namespace nx::core;

class Directory
{
public:
  Directory()
  : m_Path(std::filesystem::path(unit_test::k_BinaryDir.view()) / "test_output" / ("C15Compatibility-" + std::to_string(std::random_device{}())))
  {
    REQUIRE(std::filesystem::create_directories(m_Path));
  }
  ~Directory() noexcept
  {
    std::error_code error;
    std::filesystem::remove_all(m_Path, error);
  }
  Directory(const Directory&) = delete;
  Directory(Directory&&) = delete;
  Directory& operator=(const Directory&) = delete;
  Directory& operator=(Directory&&) = delete;
  const std::filesystem::path& path() const noexcept
  {
    return m_Path;
  }

private:
  std::filesystem::path m_Path;
};

template <class T>
void CheckSaved(const std::filesystem::path& path, const DataPath& arrayPath, const ShapeType& tupleShape, const std::vector<T>& expected)
{
  auto file = HDF5::FileIO::ReadFile(path);
  REQUIRE(file.isValid());
  auto dataset = file.openDataset("DataStructure/" + arrayPath.toString());
  REQUIRE(dataset.getId() > 0);
  REQUIRE(dataset.isValid());
  const auto type = dataset.getDataType();
  SIMPLNX_RESULT_REQUIRE_VALID(type);
  REQUIRE(type.value() == GetDataType<T>());
  ShapeType shape = tupleShape;
  shape.push_back(1);
  REQUIRE(dataset.getDimensions() == shape);
  std::vector<T> actual(expected.size());
  const auto read = dataset.readIntoSpan<T>(nonstd::span<T>(actual.data(), actual.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  REQUIRE(actual == expected);
}

inline void ExecuteChecked(const IFilter& filter, DataStructure& dataStructure, const Arguments& args)
{
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

inline DataPath ConvertToFloat32(DataStructure& dataStructure, const DataPath& input)
{
  const auto filter = Application::Instance()->getFilterList()->createFilter(FilterTraits<ConvertDataFilter>::uuid);
  REQUIRE(filter != nullptr);
  auto args = filter->getDefaultArguments();
  args.insertOrAssign(ConvertDataFilter::k_ScalarType_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(DataType::float32)));
  args.insertOrAssign(ConvertDataFilter::k_ArrayToConvertPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ConvertDataFilter::k_ConvertedArrayName_Key, std::make_any<std::string>("Converted"));
  args.insertOrAssign(ConvertDataFilter::k_DeleteOriginal_Key, std::make_any<bool>(false));
  ExecuteChecked(*filter, dataStructure, args);
  return input.replaceName("Converted");
}

class StoreExpectation
{
public:
  explicit StoreExpectation(IDataStore::StoreType type)
  : m_Type(type)
  {
  }

  void requireExpectedStore(const IDataArray& array) const
  {
    INFO("Array '" << array.getName() << "' expected store type " << static_cast<int>(m_Type));
    REQUIRE(array.getIDataStoreRef().getStoreType() == m_Type);
  }

private:
  IDataStore::StoreType m_Type;
};

template <class T, class Scope>
std::vector<T> Read(DataStructure& dataStructure, const DataPath& path, const ShapeType& shape, const Scope& scope)
{
  const DataArray<T>* arrayPtr = nullptr;
  REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<DataArray<T>>(path));
  REQUIRE(arrayPtr->getDataType() == GetDataType<T>());
  REQUIRE(arrayPtr->getTupleShape() == shape);
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  scope.requireExpectedStore(*arrayPtr);
  const auto& store = arrayPtr->getDataStoreRef();
  if(store.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    REQUIRE(store.getDataFormat() == "HDF5-OOC");
    const auto metadata = store.getRecoveryMetadata();
    const auto backing = metadata.find("OocBackingFilePath");
    REQUIRE(backing != metadata.end());
    REQUIRE(std::filesystem::is_regular_file(backing->second));
  }
  std::vector<T> values(arrayPtr->getSize());
  const auto result = store.copyIntoBuffer(0, nonstd::span<T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  return values;
}

template <class Filter>
Arguments ImageArguments(const Filter& filter, const DataPath& input, const std::string& output)
{
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(Filter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(Filter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(Filter::k_OutputImageArrayName_Key, std::make_any<std::string>(output));
  return args;
}

template <class Filter>
DataPath Pointwise(DataStructure& dataStructure, const DataPath& input, const std::string& output)
{
  const Filter filter;
  const auto args = ImageArguments(filter, input, output);
  ExecuteChecked(filter, dataStructure, args);
  return input.replaceName(output);
}

template <class Filter, class Scope>
void Execute(const Filter& filter, DataStructure& dataStructure, const Arguments& args, Scope& scope)
{
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto result = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
}

// BinaryThreshold has one pointwise implementation and does not select an algorithm.
inline DataPath Select(DataStructure& dataStructure, const DataPath& input, float64 lower, float64 upper, const std::string& name)
{
  const BinaryThresholdImageFilter filter;
  auto args = ImageArguments(filter, input, name);
  args.insertOrAssign(BinaryThresholdImageFilter::k_LowerThreshold_Key, std::make_any<float64>(lower));
  args.insertOrAssign(BinaryThresholdImageFilter::k_UpperThreshold_Key, std::make_any<float64>(upper));
  args.insertOrAssign(BinaryThresholdImageFilter::k_InsideValue_Key, std::make_any<uint8>(1));
  args.insertOrAssign(BinaryThresholdImageFilter::k_OutsideValue_Key, std::make_any<uint8>(0));
  ExecuteChecked(filter, dataStructure, args);
  return input.replaceName(name);
}

inline DataPath Components(DataStructure& dataStructure, const DataPath& input, const std::string& name)
{
  const ConnectedComponentImageFilter filter;
  auto args = ImageArguments(filter, input, name);
  args.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(false));
  args.insertOrAssign(ConnectedComponentImageFilter::k_ProvisionalLabelLimitHandling_Key, std::make_any<ChoicesParameter::ValueType>(0));
  ExecuteChecked(filter, dataStructure, args);
  return input.replaceName(name);
}

inline void SetGeometry(DataStructure& dataStructure)
{
  ImageGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  geometryPtr->setSpacing({1, 2, 3});
  geometryPtr->setOrigin({4, 5, 6});
  geometryPtr->setUnits(IGeometry::LengthUnit::Millimeter);
}

inline void CheckGeometry(DataStructure& dataStructure, const SizeVec3& dimensions)
{
  const ImageGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(DataPath({"Image Geometry"})));
  REQUIRE(geometryPtr->getDimensions() == dimensions);
  REQUIRE(geometryPtr->getSpacing() == FloatVec3{1, 2, 3});
  REQUIRE(geometryPtr->getOrigin() == FloatVec3{4, 5, 6});
  REQUIRE(geometryPtr->getUnits() == IGeometry::LengthUnit::Millimeter);
}

inline Arguments WriteArguments(const DataPath& input, const std::filesystem::path& output)
{
  const WriteImageFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(WriteImageFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(WriteImageFilter::k_ImageArrayPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(WriteImageFilter::k_FileName_Key, std::make_any<std::filesystem::path>(output));
  args.insertOrAssign(WriteImageFilter::k_Plane_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WriteImageFilter::k_TotalIndexDigits_Key, std::make_any<int32>(3));
  args.insertOrAssign(WriteImageFilter::k_LeadingDigitCharacter_Key, std::make_any<std::string>("0"));
  args.insertOrAssign(WriteImageFilter::k_CreateColorTable_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_FlipMode_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(WriteImageFilter::k_AddScaleBar_Key, std::make_any<bool>(false));
  args.insertOrAssign(WriteImageFilter::k_NonfiniteInputHandling_Key, std::make_any<ChoicesParameter::ValueType>(0));
  return args;
}

template <class T>
std::vector<T> Decode(const std::filesystem::path& path, usize width, usize height, usize components = 1)
{
  const auto imageIO = CreateImageIO(path);
  SIMPLNX_RESULT_REQUIRE_VALID(imageIO);
  REQUIRE(imageIO.value() != nullptr);
  const auto metadata = imageIO.value()->readMetadata(path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  REQUIRE(metadata.value().dataType == GetDataType<T>());
  REQUIRE(metadata.value().width == width);
  REQUIRE(metadata.value().height == height);
  REQUIRE(metadata.value().numComponents == components);
  REQUIRE(metadata.value().numPages == 1);
  std::vector<uint8> bytes(width * height * components * sizeof(T));
  const auto read = imageIO.value()->readPixelData(path, std::span<uint8>(bytes));
  SIMPLNX_RESULT_REQUIRE_VALID(read);
  std::vector<T> values(width * height * components);
  std::memcpy(values.data(), bytes.data(), bytes.size());
  return values;
}

inline DataStructure ReadMha(const std::filesystem::path& path)
{
  const ReadMhaFileFilter filter;
  auto args = filter.getDefaultArguments();
  args.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<std::filesystem::path>(path));
  args.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("CellData"));
  args.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("Pixels"));
  DataStructure result;
  const auto read = filter.execute(result, args);
  SIMPLNX_RESULT_REQUIRE_VALID(read.result);
  UnitTest::CheckArraysInheritTupleDims(result);
  return result;
}
// NOLINTEND(bugprone-chained-comparison,cppcoreguidelines-avoid-do-while)
} // namespace c15_test
