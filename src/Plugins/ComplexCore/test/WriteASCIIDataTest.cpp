#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/WriteASCIIDataFilter.hpp"

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace complex;

/**
 * @brief The WriteASCIIDataTest class
 */
namespace
{
constexpr int32 k_NumOfDataArrays = 3;      // used for generation
constexpr int32 k_NumOfTuples = 10;         // used for generation
constexpr int32 k_NumComponents = 2;        // used for generation
constexpr uint64 k_OutputStyleElements = 2; // pull enum # of elements
constexpr uint64 k_DelimiterElements = 5;   // pull enum # of elements
constexpr uint64 k_MultipleFiles = 0;       // enum representation
constexpr uint64 k_SingleFile = 1;          // enum representation
constexpr uint64 k_TabDelimiter = 4;        // enum representation
} // namespace

// -----------------------------------------------------------------------------
template <class T>
class RunASCIIDataTest
{
public:
  // ctor
  RunASCIIDataTest(DataStructure& dataStructure)
  : m_DataStructure(dataStructure)
  {
  }
  // virtual dtor
  ~RunASCIIDataTest() = default;

  void execute()
  {
    setMemb(); // set up member variables according to template type (if switch)

    // double for loop for each choice possibility
    for(uint64 fValue = 0; fValue < k_OutputStyleElements; fValue++)
    {
      for(uint64 del = 0; del < k_DelimiterElements; del++)
      {
        Test(fValue, del);
      }
    }
  }

private:
  DataStructure& m_DataStructure;
  fs::path m_ExemplarsPath = fs::path("");
  fs::path m_TestOutput = fs::path("");
  std::vector<T> m_FillValue = std::vector<T>{};
  std::string m_SingleFileName = "";

  void setMemb()
  {
    if constexpr(std::is_same<T, int8>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int8"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "int8"));
      setFillValue(-65, -63, -61);
    }
    else if constexpr(std::is_same<T, int32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "int32"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "int32"));
      setFillValue(-6500, -6498, -6496);
    }
    else if constexpr(std::is_same<T, uint8>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint8"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "uint8"));
      setFillValue(65, 67, 69);
    }
    else if constexpr(std::is_same<T, uint32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "uint32"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "uint32"));
      setFillValue(6500, 6502, 6504);
    }
    else if constexpr(std::is_same<T, float32>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "float32"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "float32"));
      setFillValue(8.00, 67.001, 69.000001);
    }
    else if constexpr(std::is_same<T, float64>::value)
    {
      m_ExemplarsPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir.view(), "float64"));
      m_TestOutput = fs::path(fmt::format("{}/{}", unit_test::k_BinaryTestOutputDir, "float64"));
      setFillValue(65.00, 67.54335100, 69.0000015436767341);
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
    std::hash<std::string> str_hash;

    fs::path writtenFilePath = fs::path(m_TestOutput.string() + "/" + selectedArray.getName() + ".txt");
    if(!fs::exists(writtenFilePath))
    {
      writtenFilePath = fs::path(m_TestOutput.string() + "/" + m_SingleFileName + ".txt");
    }
    REQUIRE(fs::exists(writtenFilePath));
    fs::path exemplarFilePath = fs::path(m_ExemplarsPath.string() + "/" + selectedArray.getName() + ".txt");
    if(!fs::exists(exemplarFilePath))
    {
      exemplarFilePath = fs::path(m_ExemplarsPath.string() + "/" + m_SingleFileName + ".txt");
    }
    REQUIRE(fs::exists(exemplarFilePath));
    REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
  }

  void Test(uint64 fileType, uint64 delimiter)
  {
    // create data set
    m_DataStructure.clear();
    // create DataArrays and store in vector to pass as an args
    for(int32 index = 0; index < k_NumOfDataArrays; index++)
    {
      using DataStoreType = DataStore<T>;
      using ArrayType = DataArray<T>;

      ArrayType* dataArray = ArrayType::template CreateWithStore<DataStoreType>(m_DataStructure, std::to_string(fileType) + "_" + std::to_string(delimiter) + "_" + "array_" + std::to_string(index),
                                                                                {static_cast<usize>(k_NumOfTuples)}, {static_cast<usize>(k_NumComponents)});
      dataArray->fill(m_FillValue[index]);
    }
    std::vector<DataPath> daps1 = m_DataStructure.getAllDataPaths();

    // Instantiate the filter and an Arguments Object
    WriteASCIIDataFilter filter;
    Arguments args;

    if(fileType == 1) // single file
    {
      m_SingleFileName = std::to_string(fileType) + "_" + std::to_string(delimiter) + "_" + "array";
    }
    // Create default Parameters for the filter.
    args.insertOrAssign(WriteASCIIDataFilter::k_OutputStyle_Key, std::make_any<ChoicesParameter::ValueType>(fileType)); // uint64 0 and 1
    args.insertOrAssign(WriteASCIIDataFilter::k_OutputPath_Key, std::make_any<fs::path>(fs::path(m_TestOutput.string() + "/" + m_SingleFileName + ".txt")));
    args.insertOrAssign(WriteASCIIDataFilter::k_OutputDir_Key, std::make_any<fs::path>(m_TestOutput));
    args.insertOrAssign(WriteASCIIDataFilter::k_FileExtension_Key, std::make_any<std::string>(".txt"));
    args.insertOrAssign(WriteASCIIDataFilter::k_MaxValPerLine_Key, std::make_any<int32>(0));
    args.insertOrAssign(WriteASCIIDataFilter::k_Delimiter_Key, std::make_any<ChoicesParameter::ValueType>(delimiter)); // uint64 0 - 4 inclusive
    args.insertOrAssign(WriteASCIIDataFilter::k_Includes_Key, std::make_any<ChoicesParameter::ValueType>(1));
    args.insertOrAssign(WriteASCIIDataFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(daps1));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(m_DataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

    // read the file(s) back in
    if(fileType == k_MultipleFiles)
    {
      for(int32 i = 0; i < daps1.size(); i++)
      {
        CompareResults(m_DataStructure.getDataRefAs<IDataArray>(daps1[i]));
      }
    }
    else if(fileType == k_SingleFile)
    {
      CompareResults(m_DataStructure.getDataRefAs<IDataArray>(daps1[0]));
    }
  }
};

TEST_CASE("ComplexCore::WriteASCIIData: Valid filter execution")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ascii_data_exemplars.tar.gz", "ascii_data_exemplars",
                                                             complex::unit_test::k_BinaryTestOutputDir);

  DataStructure dataStructure;

  RunASCIIDataTest<int8>(dataStructure).execute();
  RunASCIIDataTest<int32>(dataStructure).execute();
  RunASCIIDataTest<uint8>(dataStructure).execute();
  RunASCIIDataTest<uint32>(dataStructure).execute();
  RunASCIIDataTest<float32>(dataStructure).execute();
  RunASCIIDataTest<float64>(dataStructure).execute();
} // end of test case
