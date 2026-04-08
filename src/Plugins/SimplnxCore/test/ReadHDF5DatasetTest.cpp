#include "SimplnxCore/Filters/ReadHDF5DatasetFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <functional>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr hsize_t COMPDIMPROD = 72;
constexpr hsize_t TUPLEDIMPROD = 40;
std::string m_FilePath = unit_test::k_BinaryDir.str() + "/ImportHDF5DatasetTest.h5";

// -----------------------------------------------------------------------------
//  Uses Raw Pointers to save data to the data file
// -----------------------------------------------------------------------------
template <typename T, uint8 Dims = 1>
void writePointerArrayDataset(nx::core::HDF5::GroupIO& ptrGroupWriter)
{
  std::string dsetName = nx::core::HDF5::Support::HdfTypeForPrimitiveAsStr<T>();
  std::vector<usize> dims = {};

  if constexpr(Dims == 1)
  {
    dsetName = "Pointer1DArrayDataset<" + dsetName + ">";
    dims = {COMPDIMPROD * TUPLEDIMPROD};
  }
  if constexpr(Dims == 2)
  {
    dsetName = "Pointer2DArrayDataset<" + dsetName + ">";
    dims = {10, (COMPDIMPROD * TUPLEDIMPROD) / 10};
  }
  if constexpr(Dims == 3)
  {
    dsetName = "Pointer3DArrayDataset<" + dsetName + ">";
    dims = {10, 8, (COMPDIMPROD * TUPLEDIMPROD) / 10 / 8};
  }
  if constexpr(Dims == 4)
  {
    dsetName = "Pointer4DArrayDataset<" + dsetName + ">";
    dims = {10, 8, 6, (COMPDIMPROD * TUPLEDIMPROD) / 10 / 8 / 6};
  }

  hsize_t tSize = std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<hsize_t>());
  std::vector<T> data(tSize);
  for(hsize_t i = 0; i < tSize; ++i)
  {
    data[i] = static_cast<T>(i * 5);
  }

  auto dsetWriter = ptrGroupWriter.createDataset(dsetName);
  auto result = dsetWriter.writeSpan(dims, nonstd::span<const T>{data});
  SIMPLNX_RESULT_REQUIRE_VALID(result);
}

template <typename T>
constexpr auto writePointer1DArrayDataset = &writePointerArrayDataset<T, 1>;

template <typename T>
constexpr auto writePointer2DArrayDataset = &writePointerArrayDataset<T, 2>;

template <typename T>
constexpr auto writePointer3DArrayDataset = &writePointerArrayDataset<T, 3>;

template <typename T>
constexpr auto writePointer4DArrayDataset = &writePointerArrayDataset<T, 4>;

// -----------------------------------------------------------------------------
void writeHDF5File()
{
  if(fs::exists(m_FilePath))
  {
    if(!fs::remove(m_FilePath))
    {
      REQUIRE(0 == 1);
    }
  }

  nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(m_FilePath);
  REQUIRE(fileWriter.isValid());

  // Create the Pointer group
  auto ptrGroupWriter = fileWriter.createGroup("Pointer");

  writePointer1DArrayDataset<int8>(ptrGroupWriter);
  writePointer1DArrayDataset<uint8>(ptrGroupWriter);
  writePointer1DArrayDataset<int16>(ptrGroupWriter);
  writePointer1DArrayDataset<uint16>(ptrGroupWriter);
  writePointer1DArrayDataset<int32>(ptrGroupWriter);
  writePointer1DArrayDataset<uint32>(ptrGroupWriter);
  writePointer1DArrayDataset<int64>(ptrGroupWriter);
  writePointer1DArrayDataset<uint64>(ptrGroupWriter);
  writePointer1DArrayDataset<float32>(ptrGroupWriter);
  writePointer1DArrayDataset<float64>(ptrGroupWriter);

  writePointer2DArrayDataset<int8>(ptrGroupWriter);
  writePointer2DArrayDataset<uint8>(ptrGroupWriter);
  writePointer2DArrayDataset<int16>(ptrGroupWriter);
  writePointer2DArrayDataset<uint16>(ptrGroupWriter);
  writePointer2DArrayDataset<int32>(ptrGroupWriter);
  writePointer2DArrayDataset<uint32>(ptrGroupWriter);
  writePointer2DArrayDataset<int64>(ptrGroupWriter);
  writePointer2DArrayDataset<uint64>(ptrGroupWriter);
  writePointer2DArrayDataset<float32>(ptrGroupWriter);
  writePointer2DArrayDataset<float64>(ptrGroupWriter);

  writePointer3DArrayDataset<int8>(ptrGroupWriter);
  writePointer3DArrayDataset<uint8>(ptrGroupWriter);
  writePointer3DArrayDataset<int16>(ptrGroupWriter);
  writePointer3DArrayDataset<uint16>(ptrGroupWriter);
  writePointer3DArrayDataset<int32>(ptrGroupWriter);
  writePointer3DArrayDataset<uint32>(ptrGroupWriter);
  writePointer3DArrayDataset<int64>(ptrGroupWriter);
  writePointer3DArrayDataset<uint64>(ptrGroupWriter);
  writePointer3DArrayDataset<float32>(ptrGroupWriter);
  writePointer3DArrayDataset<float64>(ptrGroupWriter);

  writePointer4DArrayDataset<int8>(ptrGroupWriter);
  writePointer4DArrayDataset<uint8>(ptrGroupWriter);
  writePointer4DArrayDataset<int16>(ptrGroupWriter);
  writePointer4DArrayDataset<uint16>(ptrGroupWriter);
  writePointer4DArrayDataset<int32>(ptrGroupWriter);
  writePointer4DArrayDataset<uint32>(ptrGroupWriter);
  writePointer4DArrayDataset<int64>(ptrGroupWriter);
  writePointer4DArrayDataset<uint64>(ptrGroupWriter);
  writePointer4DArrayDataset<float32>(ptrGroupWriter);
  writePointer4DArrayDataset<float64>(ptrGroupWriter);
}

// -----------------------------------------------------------------------------
void testFilterPreflight(ReadHDF5DatasetFilter& filter)
{
  Arguments args;
  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  std::string levelZeroAMName = Constants::k_LevelZero.str() + "AM";
  AttributeMatrix* levelZeroAttributeMatrix = AttributeMatrix::Create(dataStructure, levelZeroAMName, {COMPDIMPROD * TUPLEDIMPROD});
  std::optional<DataPath> levelZeroPath = {DataPath::FromString(Constants::k_LevelZero.view()).value()};

  ReadHDF5DatasetParameter::ValueType val = {levelZeroPath, "", {}};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  // Check empty file path error
  auto results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check incorrect extension error
  val = {levelZeroPath, "foo.txt", {}};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check non-existent file error
  val = {levelZeroPath, "foo.h5", {}};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Put in the correct file path
  val = {levelZeroPath, m_FilePath, {}};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  // Check no datasets checked error
  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check empty dataset path error
  std::list<ReadHDF5DatasetParameter::DatasetImportInfo> importInfoList;
  ReadHDF5DatasetParameter::DatasetImportInfo importInfo;
  importInfo.componentDimensions = "";
  importInfo.tupleDimensions = "";
  importInfo.dataSetPath = "";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check incorrect dataset path error
  importInfoList.clear();
  importInfo.dataSetPath = "/Foo";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Fill in Dataset Path with a valid path so that we can continue our error checks
  importInfoList.clear();
  std::string typeStr = nx::core::HDF5::Support::HdfTypeForPrimitiveAsStr<int8>();
  importInfo.dataSetPath = "Pointer/Pointer1DArrayDataset<" + typeStr + ">";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  // Check empty component dimensions
  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check incorrect component dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "(abcdg 635w";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check empty tuple dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "12, 6";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check incorrect tuple dimensions
  importInfoList.clear();
  importInfo.tupleDimensions = "(abcdg 635w";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check empty parent attribute matrix/data group
  val = {std::optional<DataPath>{}, m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(results.outputActions)

  // Check correct Attribute Matrix parent / dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "1";
  importInfo.tupleDimensions = "2880";
  importInfoList.push_back(importInfo);
  val = {DataPath::FromString(levelZeroAMName), m_FilePath, importInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(results.outputActions)
}

// -----------------------------------------------------------------------------
std::string createVectorString(const ShapeType& vec)
{
  std::string str = "(";
  for(int i = 0; i < vec.size(); i++)
  {
    str.append(StringUtilities::number(vec[i]));
    if(i < vec.size() - 1)
    {
      str.append(",");
    }
  }
  str.append(")");

  return str;
}

// -----------------------------------------------------------------------------
template <typename T>
void DatasetTest(ReadHDF5DatasetFilter& filter, const std::list<ReadHDF5DatasetParameter::DatasetImportInfo>& importInfoList, bool useParentGroup, bool resultsValid)
{
  if(importInfoList.empty())
  {
    return;
  }

  std::string typeStr = nx::core::HDF5::Support::HdfTypeForPrimitiveAsStr<T>();

  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  std::optional<DataPath> parentGroup{};
  if(useParentGroup)
  {
    parentGroup = {DataPath::FromString(Constants::k_LevelZero.view()).value()};
  }

  std::list<ReadHDF5DatasetParameter::DatasetImportInfo> dsetInfoList = importInfoList;
  for(ReadHDF5DatasetParameter::DatasetImportInfo& info : dsetInfoList)
  {
    info.dataSetPath = StringUtilities::replace(info.dataSetPath, "@TYPE_STRING@", typeStr);
  }

  Arguments args;
  ReadHDF5DatasetParameter::ValueType val = {parentGroup, m_FilePath, dsetInfoList};
  args.insertOrAssign(ReadHDF5DatasetFilter::k_ImportHDF5File_Key.str(), std::make_any<ReadHDF5DatasetParameter::ValueType>(val));

  // Execute Dataset Test
  if(dsetInfoList.size() > 1)
  {
    std::string statusMessage = "Starting Multiple Dataset Test: ";
    std::string dsetPathsStr = "";
    std::string cDimsVectorStr = "";
    std::string tDimsStr = "";
    for(const auto& info : dsetInfoList)
    {
      dsetPathsStr.append(info.dataSetPath + "\n");
      std::string cDimsStr = info.componentDimensions;
      cDimsVectorStr.append(cDimsStr + "\n");
      tDimsStr.append(info.tupleDimensions + "\n");
    }

    statusMessage.append("Dataset Paths = \n" + dsetPathsStr);
    statusMessage.append("tDims = " + tDimsStr + "\n");
    statusMessage.append("cDims = \n" + cDimsVectorStr);
  }
  else
  {
    ReadHDF5DatasetParameter::DatasetImportInfo info = dsetInfoList.front();
  }

  auto result = filter.execute(dataStructure, args);
  REQUIRE(result.result.valid() == resultsValid);

  // If we got through without errors, validate the results
  if(resultsValid)
  {
    for(const auto& info : dsetInfoList)
    {
      // Calculate the total number of tuples
      std::string tDimsStr = info.tupleDimensions;
      std::vector<std::string> tDims = StringUtilities::split(tDimsStr, ',');
      usize tDimsProduct = 1;
      for(const auto& tDim : tDims)
      {
        usize tdim = std::stoi(tDim);
        tDimsProduct = tDimsProduct * tdim;
      }

      std::string cDimsStr = info.componentDimensions;
      std::vector<std::string> tokens = StringUtilities::split(cDimsStr, ',');
      ShapeType cDims;
      cDims.reserve(tokens.size());
      for(const auto& token : tokens)
      {
        cDims.push_back(std::stoi(token));
      }

      // Calculate the total number of components
      usize cDimsProduct = 1;
      for(usize cDim : cDims)
      {
        cDimsProduct = cDimsProduct * cDim;
      }

      std::string dsetPath = info.dataSetPath;
      std::string dsetName = StringUtilities::replace(dsetPath, "/Pointer/", "");
      auto dataArrayPath = DataPath::FromString(Constants::k_LevelZero.str() + "/" + dsetName).value();
      auto da = dataStructure.getSharedDataAs<DataArray<T>>(dataArrayPath);
      REQUIRE(da != nullptr);
      auto daNumTuples = da->getNumberOfTuples();
      auto daNumComponents = da->getNumberOfComponents();
      usize totalArrayValues = daNumTuples * daNumComponents;
      REQUIRE(totalArrayValues == tDimsProduct * cDimsProduct);

      // Bulk-read into local buffer to avoid per-element OOC overhead
      std::vector<T> buf(totalArrayValues);
      da->getDataStoreRef().copyIntoBuffer(0, nonstd::span<T>(buf.data(), totalArrayValues));
      for(usize i = 0; i < totalArrayValues; ++i)
      {
        T value = buf[i];
        REQUIRE(value == static_cast<T>(i * 5));
      }
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
void testFilterExecute(ReadHDF5DatasetFilter& filter)
{
  //  // ******************* Test Reading Data *************************************

  // Create tuple and component dimensions for all tests
  std::vector<ShapeType> tDimsVector;
  std::vector<ShapeType> cDimsVector;

  // Add 1D, 2D, 3D, and 4D tuple and component dimensions that test all 4 possibilities:
  // 1. Tuple dimensions and component dimensions are both valid
  // 2. Tuple dimensions are valid, but component dimensions are invalid
  // 3. Tuple dimensions are invalid, but component dimensions are valid
  // 4. Neither tuple dimensions or component dimensions are valid

  tDimsVector.push_back(ShapeType{TUPLEDIMPROD});
  cDimsVector.push_back(ShapeType{COMPDIMPROD});

  tDimsVector.push_back(ShapeType{10, 4});
  cDimsVector.push_back(ShapeType{12, 6});

  tDimsVector.push_back(ShapeType{2, 2, 10});
  cDimsVector.push_back(ShapeType{4, 3, 6});

  tDimsVector.push_back(ShapeType{2, 2, 5, 2});
  cDimsVector.push_back(ShapeType{4, 3, 3, 2});

  tDimsVector.push_back(ShapeType{TUPLEDIMPROD - 1});
  cDimsVector.push_back(ShapeType{COMPDIMPROD - 1});

  tDimsVector.push_back(ShapeType{TUPLEDIMPROD - 1, 34});
  cDimsVector.push_back(ShapeType{COMPDIMPROD - 1, 56});

  tDimsVector.push_back(ShapeType{TUPLEDIMPROD - 1, 23, 654});
  cDimsVector.push_back(ShapeType{COMPDIMPROD - 1, 56, 12});

  tDimsVector.push_back(ShapeType{TUPLEDIMPROD - 1, 98, 12, 45});
  cDimsVector.push_back(ShapeType{COMPDIMPROD - 1, 43, 12, 53});

  // Execute all combinations of tests
  for(const auto& tDims : tDimsVector)
  {
    for(const auto& cDims : cDimsVector)
    {
      usize amTupleCount = 1;
      for(usize tDim : tDims)
      {
        amTupleCount *= tDim;
      }

      usize cDimsProd = 1;
      for(usize cDim : cDims)
      {
        cDimsProd *= cDim;
      }

      // Figure out our error code based on the dimensions coming in
      bool resultsValid = true;
      if(TUPLEDIMPROD * COMPDIMPROD != amTupleCount * cDimsProd)
      {
        resultsValid = false;
      }

      std::list<ReadHDF5DatasetParameter::DatasetImportInfo> importInfoList;
      ReadHDF5DatasetParameter::DatasetImportInfo info;
      info.componentDimensions = fmt::format("{}", fmt::join(cDims, ", "));
      info.tupleDimensions = fmt::format("{}", fmt::join(tDims, ", "));

      std::vector<std::string> dsetPaths;
      dsetPaths.emplace_back("/Pointer/Pointer1DArrayDataset<@TYPE_STRING@>");
      dsetPaths.emplace_back("/Pointer/Pointer2DArrayDataset<@TYPE_STRING@>");
      dsetPaths.emplace_back("/Pointer/Pointer3DArrayDataset<@TYPE_STRING@>");
      dsetPaths.emplace_back("/Pointer/Pointer4DArrayDataset<@TYPE_STRING@>");

      // Run 1D Array Tests
      info.dataSetPath = dsetPaths[0];
      importInfoList.push_back(info);
      DatasetTest<int8>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 2D Array Tests
      info.dataSetPath = dsetPaths[1];
      importInfoList.push_back(info);
      DatasetTest<int8>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 3D Array Tests
      info.dataSetPath = dsetPaths[2];
      importInfoList.push_back(info);
      DatasetTest<int8>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 4D Array Tests
      info.dataSetPath = dsetPaths[3];
      importInfoList.push_back(info);
      DatasetTest<int8>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Test every possible set of 2 datasets
      for(int a = 0; a < dsetPaths.size(); a++)
      {
        for(int b = a + 1; b < dsetPaths.size(); b++)
        {
          info.dataSetPath = dsetPaths[a];
          importInfoList.push_back(info);
          info.dataSetPath = dsetPaths[b];
          importInfoList.push_back(info);

          DatasetTest<int8>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
          DatasetTest<int16>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
          DatasetTest<int32>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
          DatasetTest<int64>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
          DatasetTest<float32>(filter, importInfoList, true, resultsValid);
          DatasetTest<float64>(filter, importInfoList, true, resultsValid);

          importInfoList.clear();
        }
      }

      // Test every possible set of 3 datasets
      for(int a = 0; a < dsetPaths.size(); a++)
      {
        for(int b = a + 1; b < dsetPaths.size(); b++)
        {
          for(int c = b + 1; c < dsetPaths.size(); c++)
          {
            info.dataSetPath = dsetPaths[a];
            importInfoList.push_back(info);
            info.dataSetPath = dsetPaths[b];
            importInfoList.push_back(info);
            info.dataSetPath = dsetPaths[c];
            importInfoList.push_back(info);

            DatasetTest<int8>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
            DatasetTest<int16>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
            DatasetTest<int32>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
            DatasetTest<int64>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
            DatasetTest<float32>(filter, importInfoList, true, resultsValid);
            DatasetTest<float64>(filter, importInfoList, true, resultsValid);

            importInfoList.clear();
          }
        }
      }

      importInfoList.clear();

      // Test the set of 4 datasets
      info.dataSetPath = dsetPaths[0];
      importInfoList.push_back(info);
      info.dataSetPath = dsetPaths[1];
      importInfoList.push_back(info);
      info.dataSetPath = dsetPaths[2];
      importInfoList.push_back(info);
      info.dataSetPath = dsetPaths[3];
      importInfoList.push_back(info);

      DatasetTest<int8>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);
    }
  }
}
} // namespace
// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ReadHDF5DatasetFilter Filter")
{
  UnitTest::LoadPlugins();

  {
    writeHDF5File();

    ReadHDF5DatasetFilter filter;
    testFilterPreflight(filter);
    testFilterExecute(filter);
  }

  if(fs::exists(m_FilePath))
  {
    if(!fs::remove(m_FilePath))
    {
      REQUIRE(0 == 1);
    }
  }
}

TEST_CASE("SimplnxCore::ReadHDF5DatasetFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadHDF5DatasetFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadHDF5DatasetFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadHDF5DatasetFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadHDF5DatasetFilter>::uuid);

      // Note: Complex SIMPL parameter conversions may produce warnings
      // pipelineFilter->getComments() may not be empty for filters with custom converters

      const Arguments args = pipelineFilter->getArguments();
    }
  }
}
