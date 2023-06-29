#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/WriteBinaryDataFilter.hpp"

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <fmt/core.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace complex;

/**
 * @brief The WriteASCIIDataTest class
 */
namespace
{
const fs::path k_TestOutput = fs::path(fmt::format("{}", unit_test::k_BinaryTestOutputDir));
constexpr int32 k_NumOfDataArrays = 3;    // used for generation
constexpr int32 k_NumOfTuples = 10;       // used for generation
constexpr int32 k_NumComponents = 16;     // used for generation
constexpr uint64 k_EndianessElements = 2; // pull enum # of elements
constexpr uint64 k_MultipleFiles = 0;     // enum representation
constexpr uint64 k_SingleFile = 1;        // enum representation
} // namespace

// -----------------------------------------------------------------------------
template <class T>
class RunBinaryTest
{
public:
  // ctor
  RunBinaryTest(DataStructure& dataStructure)
  : m_DataStructure(dataStructure)
  {
  }
  // virtual dtor
  ~RunBinaryTest() = default;

  void execute()
  {
    setMemb(); // set up member variables according to template type (glorified if switch)

    // double for loop for each choice possibility
    for(uint64 fValue = 0; fValue < k_EndianessElements; fValue++)
    {
      Test(fValue);
    }
  }

private:
  DataStructure& m_DataStructure;
  fs::path m_ExemplarsPath = fs::path("");
  std::vector<T> m_FillValue = std::vector<T>{};

  void setMemb()
  {
    if(std::is_same<T, int8>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int8"));
      setFillValue(-65, -63, -61);
    }
    else if(std::is_same<T, int16>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int16"));
      setFillValue(-650, -648, -646);
    }
    else if(std::is_same<T, int32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int32"));
      setFillValue(-6500, -6498, -6496);
    }
    else if(std::is_same<T, int64>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int64"));
      setFillValue(-65000, -64998, -64996);
    }
    else if(std::is_same<T, uint8>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint8"));
      setFillValue(65, 67, 69);
    }
    else if(std::is_same<T, uint16>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint16"));
      setFillValue(650, 652, 654);
    }
    else if(std::is_same<T, uint32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint32"));
      setFillValue(6500, 6502, 6504);
    }
    else if(std::is_same<T, uint64>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint64"));
      setFillValue(65000, 65002, 65004);
    }
    else if(std::is_same<T, float32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "float32"));
      setFillValue(65.001, 67.001, 69.001);
    }
    else if(std::is_same<T, float64>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/export_files_test/write_binary_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "float64"));
      setFillValue(65.000001, 67.000001, 69.000001);
    }
  }

  void setFillValue(T a, T b, T c)
  {
    m_FillValue = std::vector<T>{a, b, c};
  }

  std::vector<char> readIn(fs::path filePath)
  {
    std::ifstream file(filePath.string(), std::ios_base::binary);

    if(file)
    {
      // get file size
      file.seekg(0, std::ios::end);
      std::streampos length = file.tellg();
      file.seekg(0, std::ios::beg);

      // read whole file into a vector
      std::vector<char> contents(length); // act as a buffer
      file.read(contents.data(), length);

      // build string from psuedo-buffer
      return contents;
    }
    return {};
  }

  void CompareResults(IDataArray& selectedArray) // compare hash of both file strings
  {
    fs::path writtenFilePath = fs::path(k_TestOutput.string() + "/" + selectedArray.getName() + ".bin");
    REQUIRE(fs::exists(writtenFilePath));
    std::string exemplarStr = selectedArray.getName();
    exemplarStr.replace(exemplarStr.find("array"), 5, "exemplar");
    fs::path exemplarFilePath = fs::path(m_ExemplarsPath.string() + "/" + exemplarStr + ".bin");
    REQUIRE(fs::exists(exemplarFilePath));
    REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
  }

  void Test(uint64 endianess)
  {
    // create data set
    m_DataStructure.clear();
    // create DataArrays and store in vector to pass as an args
    for(int32 index = 0; index < k_NumOfDataArrays; index++)
    {
      using DataStoreType = DataStore<T>;
      using ArrayType = DataArray<T>;

      ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(m_DataStructure, std::to_string(endianess) + "_" + "array_" + std::to_string(index),
                                                                                {static_cast<usize>(k_NumOfTuples)}, {static_cast<usize>(k_NumComponents)});
      dataArray->fill(m_FillValue[index]);
    }
    std::vector<DataPath> daps1 = m_DataStructure.getAllDataPaths();

    // Instantiate the filter and an Arguments Object
    WriteBinaryDataFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(WriteBinaryDataFilter::k_Endianess_Key, std::make_any<ChoicesParameter::ValueType>(endianess)); // uint64 0 and 1
    args.insertOrAssign(WriteBinaryDataFilter::k_OutputPath_Key, std::make_any<fs::path>(k_TestOutput));
    args.insertOrAssign(WriteBinaryDataFilter::k_FileExtension_Key, std::make_any<std::string>(".bin"));
    args.insertOrAssign(WriteBinaryDataFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(daps1));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    // read the file(s) back in
    for(int32 i = 0; i < daps1.size(); i++)
    {
      CompareResults(m_DataStructure.getDataRefAs<IDataArray>(daps1[i]));
    }
  }
};

TEST_CASE("ComplexCore::WriteBinaryData: Valid filter execution")
{
  const std::string kDataInputArchive = "export_files_test.tar.gz";
  const std::string kExpectedOutputTopLevel = "export_files_test";
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, kDataInputArchive, kExpectedOutputTopLevel,
                                                             complex::unit_test::k_BinaryTestOutputDir);

  DataStructure dataStructure;
  DataStructure& dsRef = dataStructure;

  RunBinaryTest<int8>(dsRef).execute();
  RunBinaryTest<int16>(dsRef).execute();
  RunBinaryTest<int32>(dsRef).execute();
  RunBinaryTest<int64>(dsRef).execute();
  RunBinaryTest<uint8>(dsRef).execute();
  RunBinaryTest<uint16>(dsRef).execute();
  RunBinaryTest<uint32>(dsRef).execute();
  RunBinaryTest<uint64>(dsRef).execute();
  RunBinaryTest<float32>(dsRef).execute();
  RunBinaryTest<float64>(dsRef).execute();
} // end of test case

// For this test we are going to use the Windows path of "A:/" which historically
// was for the floppy disk drives. I've not seen anyone use A or B for drive letters
// is a LONG time, so I'm going to just assume that the test machines are setup
// like a normal "modern" Windows system and start their drive lettering at "C".
//
// Not sure how to replicate this on unix as there is a decent probability that
// a test bot might actually have write access to the root "/" of the Unix OS. This
// definitely would NOT happen macOS. Linux is iffy.
TEST_CASE("ComplexCore::WriteBinaryData:Invalid Filter Execution")
{
  // We are just going to generate a big number so that we can use that in the output
  // file path. This tests the creation of intermediate directories that the filter
  // would be responsible to create.
  const uint64_t millisFromEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  // create data set
  DataStructure dataStructure;
  // create DataArrays and store in vector to pass as an args
  for(int32 index = 0; index < k_NumOfDataArrays; index++)
  {
    using DataStoreType = DataStore<int32>;
    using ArrayType = DataArray<int32>;

    ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(dataStructure, std::to_string(0) + "_" + "array_" + std::to_string(index), {static_cast<usize>(k_NumOfTuples)},
                                                                              {static_cast<usize>(k_NumComponents)});
    dataArray->fill(123123);
  }
  std::vector<DataPath> daps1 = dataStructure.getAllDataPaths();

  // Instantiate the filter and an Arguments Object
  WriteBinaryDataFilter filter;
  Arguments args;

  // These paths are meant to fail. A: doesn't probably exist on most main stream Windows computers
  // Most Unix users don't have write privs on "/". If they do then this test fails and we fix this test
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER)
  std::string invalidPath = fmt::format("A:/{}", millisFromEpoch);
#else
  std::string invalidPath = fmt::format("/{}", millisFromEpoch);
#endif

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteBinaryDataFilter::k_Endianess_Key, std::make_any<ChoicesParameter::ValueType>(0)); // uint64 0 and 1
  args.insertOrAssign(WriteBinaryDataFilter::k_OutputPath_Key, std::make_any<fs::path>(invalidPath));
  args.insertOrAssign(WriteBinaryDataFilter::k_FileExtension_Key, std::make_any<std::string>(".bin"));
  args.insertOrAssign(WriteBinaryDataFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(daps1));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
}
