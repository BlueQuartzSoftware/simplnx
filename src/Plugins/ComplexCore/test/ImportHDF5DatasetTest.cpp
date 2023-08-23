#include "ComplexCore/Filters/ImportHDF5Dataset.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/ImportHDF5DatasetParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <functional>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
constexpr hsize_t COMPDIMPROD = 72;
constexpr hsize_t TUPLEDIMPROD = 40;
std::string m_FilePath = unit_test::k_BinaryDir.str() + "/ImportHDF5DatasetTest.h5";

// -----------------------------------------------------------------------------
//  Uses Raw Pointers to save data to the data file
// -----------------------------------------------------------------------------
template <typename T, uint8 Dims = 1>
void writePointerArrayDataset(complex::HDF5::GroupWriter& ptrGroupWriter)
{
  std::string dsetName = complex::HDF5::Support::HdfTypeForPrimitiveAsStr<T>();
  std::vector<hsize_t> dims = {};

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

  hsize_t tSize =  std::accumulate(std::begin(dims), std::end(dims), 1, std::multiplies<hsize_t>());
  std::vector<T> data(tSize);
  for(hsize_t i = 0; i < tSize; ++i)
  {
    data[i] = static_cast<T>(i * 5);
  }

  complex::HDF5::DatasetWriter dsetWriter = ptrGroupWriter.createDatasetWriter(dsetName);
  auto result = dsetWriter.writeSpan(dims, nonstd::span<const T>{data});
  COMPLEX_RESULT_REQUIRE_VALID(result);
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

  auto writerResults = complex::HDF5::FileWriter::CreateFile(m_FilePath);
  COMPLEX_RESULT_REQUIRE_VALID(writerResults);
  complex::HDF5::FileWriter fileWriter = std::move(writerResults.value());
  REQUIRE(fileWriter.isValid());

  // Create the Pointer group
  complex::HDF5::GroupWriter ptrGroupWriter = fileWriter.createGroupWriter("Pointer");
  REQUIRE(ptrGroupWriter.isValid());

  writePointer1DArrayDataset<int8_t>(ptrGroupWriter);
  writePointer1DArrayDataset<uint8_t>(ptrGroupWriter);
  writePointer1DArrayDataset<int16_t>(ptrGroupWriter);
  writePointer1DArrayDataset<uint16_t>(ptrGroupWriter);
  writePointer1DArrayDataset<int32_t>(ptrGroupWriter);
  writePointer1DArrayDataset<uint32_t>(ptrGroupWriter);
  writePointer1DArrayDataset<int64_t>(ptrGroupWriter);
  writePointer1DArrayDataset<uint64_t>(ptrGroupWriter);
  writePointer1DArrayDataset<float32>(ptrGroupWriter);
  writePointer1DArrayDataset<float64>(ptrGroupWriter);

  writePointer2DArrayDataset<int8_t>(ptrGroupWriter);
  writePointer2DArrayDataset<uint8_t>(ptrGroupWriter);
  writePointer2DArrayDataset<int16_t>(ptrGroupWriter);
  writePointer2DArrayDataset<uint16_t>(ptrGroupWriter);
  writePointer2DArrayDataset<int32_t>(ptrGroupWriter);
  writePointer2DArrayDataset<uint32_t>(ptrGroupWriter);
  writePointer2DArrayDataset<int64_t>(ptrGroupWriter);
  writePointer2DArrayDataset<uint64_t>(ptrGroupWriter);
  writePointer2DArrayDataset<float32>(ptrGroupWriter);
  writePointer2DArrayDataset<float64>(ptrGroupWriter);

  writePointer3DArrayDataset<int8_t>(ptrGroupWriter);
  writePointer3DArrayDataset<uint8_t>(ptrGroupWriter);
  writePointer3DArrayDataset<int16_t>(ptrGroupWriter);
  writePointer3DArrayDataset<uint16_t>(ptrGroupWriter);
  writePointer3DArrayDataset<int32_t>(ptrGroupWriter);
  writePointer3DArrayDataset<uint32_t>(ptrGroupWriter);
  writePointer3DArrayDataset<int64_t>(ptrGroupWriter);
  writePointer3DArrayDataset<uint64_t>(ptrGroupWriter);
  writePointer3DArrayDataset<float32>(ptrGroupWriter);
  writePointer3DArrayDataset<float64>(ptrGroupWriter);

  writePointer4DArrayDataset<int8_t>(ptrGroupWriter);
  writePointer4DArrayDataset<uint8_t>(ptrGroupWriter);
  writePointer4DArrayDataset<int16_t>(ptrGroupWriter);
  writePointer4DArrayDataset<uint16_t>(ptrGroupWriter);
  writePointer4DArrayDataset<int32_t>(ptrGroupWriter);
  writePointer4DArrayDataset<uint32_t>(ptrGroupWriter);
  writePointer4DArrayDataset<int64_t>(ptrGroupWriter);
  writePointer4DArrayDataset<uint64_t>(ptrGroupWriter);
  writePointer4DArrayDataset<float32>(ptrGroupWriter);
  writePointer4DArrayDataset<float64>(ptrGroupWriter);
}

// -----------------------------------------------------------------------------
void testFilterPreflight(ImportHDF5Dataset& filter)
{
  Arguments args;
  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  std::string levelZeroAMName = Constants::k_LevelZero.str() + "AM";
  AttributeMatrix* levelZeroAttributeMatrix = AttributeMatrix::Create(dataStructure, levelZeroAMName, {COMPDIMPROD * TUPLEDIMPROD});
  std::optional<DataPath> levelZeroPath = {DataPath::FromString(Constants::k_LevelZero.view()).value()};

  ImportHDF5DatasetParameter::ValueType val = {levelZeroPath, "", {}};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  // Check empty file path error
  auto results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check incorrect extension error
  val = {levelZeroPath, "foo.txt", {}};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check non-existent file error
  val = {levelZeroPath, "foo.h5", {}};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Put in the correct file path
  val = {levelZeroPath, m_FilePath, {}};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  // Check no datasets checked error
  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check empty dataset path error
  std::list<ImportHDF5DatasetParameter::DatasetImportInfo> importInfoList;
  ImportHDF5DatasetParameter::DatasetImportInfo importInfo;
  importInfo.componentDimensions = "";
  importInfo.tupleDimensions = "";
  importInfo.dataSetPath = "";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check incorrect dataset path error
  importInfoList.clear();
  importInfo.dataSetPath = "/Foo";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Fill in Dataset Path with a valid path so that we can continue our error checks
  importInfoList.clear();
  std::string typeStr = complex::HDF5::Support::HdfTypeForPrimitiveAsStr<int8_t>();
  importInfo.dataSetPath = "Pointer/Pointer1DArrayDataset<" + typeStr + ">";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  // Check empty component dimensions
  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check incorrect component dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "(abcdg 635w";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check empty tuple dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "12, 6";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check incorrect tuple dimensions
  importInfoList.clear();
  importInfo.tupleDimensions = "(abcdg 635w";
  importInfoList.push_back(importInfo);
  val = {levelZeroPath, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check empty parent attribute matrix/data group
  val = {std::optional<DataPath>{}, m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  REQUIRE(!results.outputActions.valid());

  // Check correct Attribute Matrix parent / dimensions
  importInfoList.clear();
  importInfo.componentDimensions = "1";
  importInfoList.push_back(importInfo);
  val = {DataPath::FromString(levelZeroAMName), m_FilePath, importInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));
  results = filter.preflight(dataStructure, args);
  REQUIRE(results.outputActions.valid());
}

// -----------------------------------------------------------------------------
std::string createVectorString(std::vector<size_t> vec)
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
void DatasetTest(ImportHDF5Dataset& filter, const std::list<ImportHDF5DatasetParameter::DatasetImportInfo>& importInfoList, bool useParentGroup, bool resultsValid)
{
  if(importInfoList.empty())
  {
    return;
  }

  std::string typeStr = complex::HDF5::Support::HdfTypeForPrimitiveAsStr<T>();

  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  std::optional<DataPath> parentGroup{};
  if(useParentGroup)
  {
    parentGroup = {DataPath::FromString(Constants::k_LevelZero.view()).value()};
  }

  std::list<ImportHDF5DatasetParameter::DatasetImportInfo> dsetInfoList = importInfoList;
  for(ImportHDF5DatasetParameter::DatasetImportInfo& info : dsetInfoList)
  {
    info.dataSetPath = StringUtilities::replace(info.dataSetPath, "@TYPE_STRING@", typeStr);
  }

  Arguments args;
  ImportHDF5DatasetParameter::ValueType val = {parentGroup, m_FilePath, dsetInfoList};
  args.insertOrAssign(ImportHDF5Dataset::k_ImportHDF5File_Key.str(), std::make_any<ImportHDF5DatasetParameter::ValueType>(val));

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
    ImportHDF5DatasetParameter::DatasetImportInfo info = dsetInfoList.front();
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
      size_t tDimsProduct = 1;
      for(const auto& tDim : tDims)
      {
        size_t tdim = std::stoi(tDim);
        tDimsProduct = tDimsProduct * tdim;
      }

      std::string cDimsStr = info.componentDimensions;
      std::vector<std::string> tokens = StringUtilities::split(cDimsStr, ',');
      std::vector<size_t> cDims;
      cDims.reserve(tokens.size());
      for(const auto& token : tokens)
      {
        cDims.push_back(std::stoi(token));
      }

      // Calculate the total number of components
      size_t cDimsProduct = 1;
      for(size_t cDim : cDims)
      {
        cDimsProduct = cDimsProduct * cDim;
      }

      std::string dsetPath = info.dataSetPath;
      std::string dsetName = StringUtilities::replace(dsetPath, "/Pointer/", "");
      auto da = dataStructure.getSharedDataAs<DataArray<T>>(DataPath::FromString(Constants::k_LevelZero.str() + "/" + dsetName).value());
      REQUIRE(da != nullptr);
      auto daNumTuples = da->getNumberOfTuples();
      auto daNumComponents = da->getNumberOfComponents();
      size_t totalArrayValues = daNumTuples * daNumComponents;
      REQUIRE(totalArrayValues == tDimsProduct * cDimsProduct);

      for(size_t i = 0; i < tDimsProduct * cDimsProduct; ++i)
      {
        T value = da->at(i);
        REQUIRE(value == static_cast<T>(i * 5));
      }
    }
  }
}

// -----------------------------------------------------------------------------
void testFilterExecute(ImportHDF5Dataset& filter)
{
  //  // ******************* Test Reading Data *************************************

  // Create tuple and component dimensions for all tests
  std::vector<std::vector<size_t>> tDimsVector;
  std::vector<std::vector<size_t>> cDimsVector;

  // Add 1D, 2D, 3D, and 4D tuple and component dimensions that test all 4 possibilities:
  // 1. Tuple dimensions and component dimensions are both valid
  // 2. Tuple dimensions are valid, but component dimensions are invalid
  // 3. Tuple dimensions are invalid, but component dimensions are valid
  // 4. Neither tuple dimensions or component dimensions are valid

  tDimsVector.push_back(std::vector<size_t>(1) = {TUPLEDIMPROD});
  cDimsVector.push_back(std::vector<size_t>(1) = {COMPDIMPROD});

  tDimsVector.push_back(std::vector<size_t>(2) = {10, 4});
  cDimsVector.push_back(std::vector<size_t>(2) = {12, 6});

  tDimsVector.push_back(std::vector<size_t>(3) = {2, 2, 10});
  cDimsVector.push_back(std::vector<size_t>(3) = {4, 3, 6});

  tDimsVector.push_back(std::vector<size_t>(4) = {2, 2, 5, 2});
  cDimsVector.push_back(std::vector<size_t>(4) = {4, 3, 3, 2});

  tDimsVector.push_back(std::vector<size_t>(1) = {TUPLEDIMPROD - 1});
  cDimsVector.push_back(std::vector<size_t>(1) = {COMPDIMPROD - 1});

  tDimsVector.push_back(std::vector<size_t>(2) = {TUPLEDIMPROD - 1, 34});
  cDimsVector.push_back(std::vector<size_t>(2) = {COMPDIMPROD - 1, 56});

  tDimsVector.push_back(std::vector<size_t>(3) = {TUPLEDIMPROD - 1, 23, 654});
  cDimsVector.push_back(std::vector<size_t>(3) = {COMPDIMPROD - 1, 56, 12});

  tDimsVector.push_back(std::vector<size_t>(4) = {TUPLEDIMPROD - 1, 98, 12, 45});
  cDimsVector.push_back(std::vector<size_t>(4) = {COMPDIMPROD - 1, 43, 12, 53});

  // Execute all combinations of tests
  for(const auto& tDims : tDimsVector)
  {
    for(const auto& cDims : cDimsVector)
    {
      size_t amTupleCount = 1;
      for(size_t tDim : tDims)
      {
        amTupleCount *= tDim;
      }

      size_t cDimsProd = 1;
      for(size_t cDim : cDims)
      {
        cDimsProd *= cDim;
      }

      // Figure out our error code based on the dimensions coming in
      bool resultsValid = true;
      if(TUPLEDIMPROD * COMPDIMPROD != amTupleCount * cDimsProd)
      {
        resultsValid = false;
      }

      std::list<ImportHDF5DatasetParameter::DatasetImportInfo> importInfoList;
      ImportHDF5DatasetParameter::DatasetImportInfo info;
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
      DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 2D Array Tests
      info.dataSetPath = dsetPaths[1];
      importInfoList.push_back(info);
      DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 3D Array Tests
      info.dataSetPath = dsetPaths[2];
      importInfoList.push_back(info);
      DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);

      importInfoList.clear();

      // Run 4D Array Tests
      info.dataSetPath = dsetPaths[3];
      importInfoList.push_back(info);
      DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
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

          DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
          DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
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

            DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
            DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
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

      DatasetTest<int8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint8_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint16_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint32_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<int64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<uint64_t>(filter, importInfoList, true, resultsValid);
      DatasetTest<float32>(filter, importInfoList, true, resultsValid);
      DatasetTest<float64>(filter, importInfoList, true, resultsValid);
    }
  }
}
} // namespace
// -----------------------------------------------------------------------------
TEST_CASE("ComplexCore::ImportHDF5Dataset Filter")
{
  {
    writeHDF5File();

    ImportHDF5Dataset filter;
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
