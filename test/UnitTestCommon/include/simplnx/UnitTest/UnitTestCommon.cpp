#include "UnitTestCommon.hpp"

#include "ReaderGeometryTest.hpp"

#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

#include <zlib.h>

#include <array>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace nx::core::UnitTest
{
#ifdef _WIN32
bool VolumeSupportsPosixRename(const fs::path& directory)
{
  std::array<wchar_t, 32768> volumePath{};
  const auto absoluteDirectory = fs::absolute(directory);
  REQUIRE((GetVolumePathNameW(absoluteDirectory.c_str(), volumePath.data(), static_cast<DWORD>(volumePath.size())) != 0));
  DWORD flags = 0;
  REQUIRE((GetVolumeInformationW(volumePath.data(), nullptr, 0, nullptr, nullptr, &flags, nullptr, 0) != 0));
  // FILE_SUPPORTS_POSIX_UNLINK_RENAME, also usable with older SDK headers.
  constexpr DWORD k_SupportsPosixUnlinkRename = 0x00000400;
  return (flags & k_SupportsPosixUnlinkRename) != 0;
}
#endif

void RequireBadReaderMetadata(const IFilter& filter, const Arguments& args, const std::filesystem::path& file, bool spacing)
{
  DataStructure ds;
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  const auto& error = preflight.outputActions.errors().front();
  CHECK(error.code == (spacing ? ImageGeom::k_InvalidSpacing : ImageGeom::k_InvalidOrigin));
  CHECK(error.message.starts_with(fmt::format("Input file '{}' has invalid {} values. Error Message: ", file.string(), spacing ? "spacing" : "origin")));
  CHECK(error.message.ends_with(fmt::format("Enable '{}' to use your own values.", spacing ? "Set Spacing" : "Set Origin")));
}

void RequireDiskBackedStore(const IDataArray& array)
{
  const auto& store = array.getIDataStoreRef();
  REQUIRE((store.getStoreType() == IDataStore::StoreType::OutOfCore));
  REQUIRE((store.getDataFormat() == "HDF5-OOC"));
  const auto metadata = store.getRecoveryMetadata();
  const auto backingPath = metadata.find("OocBackingFilePath");
  REQUIRE((backingPath != metadata.cend()));
  REQUIRE_FALSE(backingPath->second.empty());
  REQUIRE(fs::is_regular_file(fs::path(backingPath->second)));
}

Arguments LoadSIMPLConversionArguments(const fs::path& fixturePath, Uuid expectedFilter)
{
  Arguments args;
  LoadSIMPLConversionPipeline(fixturePath, expectedFilter, args);
  return args;
}

Pipeline LoadSIMPLConversionPipeline(const fs::path& fixturePath, Uuid expectedFilter, Arguments& args)
{
  CAPTURE(fixturePath.string());
  LoadPlugins();
  auto result = Pipeline::FromSIMPLFile(fixturePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto& pipeline = result.value();
  REQUIRE(pipeline.size() == 1);
  const auto* node = dynamic_cast<const PipelineFilter*>(pipeline.at(0));
  REQUIRE(node != nullptr);
  const auto* filter = node->getFilter();
  REQUIRE(filter != nullptr);
  REQUIRE(filter->uuid() == expectedFilter);
  CHECK(node->getComments().empty());
  args = node->getArguments();
  return std::move(result.value());
}

DataStructure LoadDataStructure(const fs::path& filepath)
{
  LoadPlugins();
  REQUIRE(fs::exists(filepath));

  auto result = DREAM3D::LoadDataStructure(filepath);
  if(result.invalid())
  {
    for(const auto& error : result.errors())
    {
      UNSCOPED_INFO(fmt::format("[{}] {}", error.code, error.message));
    }
    FAIL(fmt::format("Failed to load DataStructure from '{}'", filepath.string()));
  }
  return std::move(result.value());
}

TestFileSentinel::TestFileSentinel(std::string testFilesDir, std::string inputArchiveName, std::string expectedTopLevelOutput, bool decompressFiles, bool removeTemp)
: m_TestFilesDir(std::move(testFilesDir))
, m_InputArchiveName(std::move(inputArchiveName))
, m_ExpectedTopLevelOutput(std::move(expectedTopLevelOutput))
, m_Decompress(decompressFiles)
, m_RemoveTemp(removeTemp)
{
  if(m_Decompress)
  {
    const auto errorCode = decompress();
    if(errorCode)
    {
      std::cout << "std::error_code.value(): " << errorCode.value() << std::endl;
      std::cout << "std::error_code.message(): " << errorCode.message() << std::endl;
      //        REQUIRE(errorCode.value() == 0);
    }
  }
}

TestFileSentinel::~TestFileSentinel()
{
  if(m_RemoveTemp)
  {
    std::error_code errorCode;
    fs::remove_all(fmt::format("{}/{}", m_TestFilesDir, m_ExpectedTopLevelOutput), errorCode);
    if(errorCode)
    {
      std::cout << "Removing decompressed data failed: " << errorCode.message() << std::endl;
    }
  }
}

namespace
{
/**
 * @brief Parses an unsigned octal field from a tar header.
 * @param data First field byte.
 * @param length Maximum field length in bytes.
 * @return The parsed value, or 0 for an empty field.
 */
uint64 parseOctal(const char* data, size_t length)
{
  uint64 value = 0;
  for(size_t i = 0; i < length; i++)
  {
    if(data[i] == '\0' || data[i] == ' ')
    {
      break;
    }
    value = value * 8 + static_cast<uint64>(data[i] - '0');
  }
  return value;
}

/**
 * @brief Tests whether a tar header block contains only zero bytes.
 * @param block Header block to test.
 * @return True if all 512 bytes are zero.
 */
bool isZeroBlock(const std::array<char, 512>& block)
{
  for(char c : block)
  {
    if(c != '\0')
    {
      return false;
    }
  }
  return true;
}

/**
 * @brief Validates the checksum in a 512-byte tar header.
 * @param header Header block to validate.
 * @return True if the stored and computed checksums are equal.
 *
 * The checksum uses the unsigned sum of all header bytes. The calculation
 * treats the eight-byte checksum field at offset 148 as spaces.
 */
bool validateChecksum(const std::array<char, 512>& header)
{
  uint64 stored = parseOctal(header.data() + 148, 8);
  uint64 computed = 0;
  for(size_t i = 0; i < 512; i++)
  {
    if(i >= 148 && i < 156)
    {
      computed += ' ';
    }
    else
    {
      computed += static_cast<unsigned char>(header[i]);
    }
  }
  return computed == stored;
}
} // namespace

std::error_code TestFileSentinel::decompress()
{
  const std::string archivePath = fmt::format("{}/{}", m_TestFilesDir, m_InputArchiveName);

  gzFile gz = gzopen(archivePath.c_str(), "rb");
  if(gz == nullptr)
  {
    std::cout << "Failed to open archive: " << archivePath << std::endl;
    return std::make_error_code(std::errc::no_such_file_or_directory);
  }

  constexpr size_t k_BlockSize = 512;
  std::array<char, k_BlockSize> header{};
  std::string gnuLongName;

  while(true)
  {
    {
      int bytesRead = gzread(gz, header.data(), k_BlockSize);
      if(bytesRead == 0)
      {
        break; // The gzip stream reached its end.
      }
      if(bytesRead < 0 || bytesRead != k_BlockSize)
      {
        std::cout << "Failed to read tar header from: " << archivePath << std::endl;
        gzclose(gz);
        return std::make_error_code(std::errc::io_error);
      }
    }

    // This extractor treats the first zero header block as the archive end.
    if(isZeroBlock(header))
    {
      break;
    }

    // Validate each header before its fields control path or size calculations.
    if(!validateChecksum(header))
    {
      std::cout << "Invalid tar header checksum in: " << archivePath << std::endl;
      gzclose(gz);
      return std::make_error_code(std::errc::io_error);
    }

    // The tar prefix starts at byte 345. The entry name starts at byte 0.
    std::string prefix(header.data() + 345, strnlen(header.data() + 345, 155));
    std::string name(header.data(), strnlen(header.data(), 100));
    std::string entryPath = prefix.empty() ? name : (prefix + "/" + name);

    // A preceding GNU long-name entry replaces the fixed-width header name.
    if(!gnuLongName.empty())
    {
      entryPath = std::move(gnuLongName);
      gnuLongName.clear();
    }

    uint64 fileSize = parseOctal(header.data() + 124, 12);
    char typeFlag = header[156];

    // A GNU `L` entry stores the name for the next archive entry.
    if(typeFlag == 'L')
    {
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      std::string longName;
      longName.reserve(fileSize);
      std::array<char, k_BlockSize> dataBuf{};
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive reading GNU long name in: " << archivePath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
        uint64 useful = std::min(fileSize - longName.size(), static_cast<uint64>(k_BlockSize));
        longName.append(dataBuf.data(), useful);
      }
      // The GNU name payload can include one trailing null byte.
      if(!longName.empty() && longName.back() == '\0')
      {
        longName.pop_back();
      }
      gnuLongName = std::move(longName);
      continue;
    }

    // A GNU `K` entry contains a long link name. This extractor skips its payload.
    if(typeFlag == 'K')
    {
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      std::array<char, k_BlockSize> dataBuf{};
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive reading GNU long link in: " << archivePath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
      }
      continue;
    }

    std::string fullPath = fmt::format("{}/{}", m_TestFilesDir, entryPath);

    // Tar type 5 is a directory. Type 0 or null is a regular file.
    if(typeFlag == '5')
    {
      fs::create_directories(fullPath);
    }
    else if(typeFlag == '0' || typeFlag == '\0')
    {
      // Create the parent before this entry writes its regular-file payload.
      fs::path filePath(fullPath);
      fs::create_directories(filePath.parent_path());

      std::ofstream outFile(fullPath, std::ios::binary);
      if(!outFile)
      {
        std::cout << "Failed to create file: " << fullPath << std::endl;
        gzclose(gz);
        return std::make_error_code(std::errc::io_error);
      }

      uint64 remaining = fileSize;
      std::array<char, k_BlockSize> dataBuf{};
      while(remaining > 0)
      {
        int bytesRead = gzread(gz, dataBuf.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive reading: " << entryPath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
        uint64 writeSize = std::min(remaining, static_cast<uint64>(k_BlockSize));
        outFile.write(dataBuf.data(), static_cast<std::streamsize>(writeSize));
        remaining -= writeSize;
      }
    }
    else
    {
      // Skip payload blocks for links and other unsupported entry types.
      uint64 blocks = (fileSize + k_BlockSize - 1) / k_BlockSize;
      for(uint64 i = 0; i < blocks; i++)
      {
        int bytesRead = gzread(gz, header.data(), k_BlockSize);
        if(bytesRead <= 0 || static_cast<size_t>(bytesRead) != k_BlockSize)
        {
          std::cout << "Unexpected end of archive skipping data for: " << entryPath << std::endl;
          gzclose(gz);
          return std::make_error_code(std::errc::io_error);
        }
      }
    }
  }

  gzclose(gz);
  return {};
}

PreferencesSentinel::PreferencesSentinel(nx::core::DataStorageMode mode, int64 largeDataSize)
: PreferencesSentinel(mode, nlohmann::json(largeDataSize))
{
}

PreferencesSentinel::PreferencesSentinel(nx::core::DataStorageMode mode, const nlohmann::json& largeDataSize)
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  const auto validation = nx::core::Preferences::ValidateOocSizeValue(nx::core::Preferences::k_LargeDataSize_Key, largeDataSize);
  if(validation.invalid())
  {
    throw std::invalid_argument(validation.errors().front().message);
  }

  // Save both values before this sentinel changes the process-wide preferences.
  m_HadMode = prefs->contains(nx::core::Preferences::k_DataStorageMode_Key);
  m_OriginalMode = prefs->value(nx::core::Preferences::k_DataStorageMode_Key);
  m_HadSize = prefs->contains(nx::core::Preferences::k_LargeDataSize_Key);
  m_OriginalSize = prefs->value(nx::core::Preferences::k_LargeDataSize_Key);

  // Apply the test values only after the complete prior state is available.
  prefs->setDataStorageMode(mode);
  prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, largeDataSize);
}

PreferencesSentinel::~PreferencesSentinel()
{
  auto* prefs = nx::core::Application::Instance()->getPreferences();

  // Restore only the in-memory values. Unit tests must not write the developer's preferences file.
  // A concurrent save or a terminated test could persist a temporary storage mode.
  // Later test processes would then inherit that incorrect mode.
  if(m_HadSize)
  {
    prefs->setValue(nx::core::Preferences::k_LargeDataSize_Key, m_OriginalSize);
  }
  else
  {
    prefs->removeValue(nx::core::Preferences::k_LargeDataSize_Key);
  }
  if(m_HadMode)
  {
    prefs->setValue(nx::core::Preferences::k_DataStorageMode_Key, m_OriginalMode);
  }
  else
  {
    prefs->removeValue(nx::core::Preferences::k_DataStorageMode_Key);
  }
}

// -----------------------------------------------------------------------------
IFilter::MessageHandler RecordMessages(std::vector<IFilter::Message>& messages)
{
  return IFilter::MessageHandler{[&messages](const IFilter::Message& message) { messages.push_back(message); }};
}

// -----------------------------------------------------------------------------
void RequireAutomaticCreateArrayActions(const Result<OutputActions>& outputActions, usize expectedCount)
{
  REQUIRE(outputActions.valid());
  usize actualCount = 0;
  for(const auto& action : outputActions.value().actions)
  {
    const auto* createArray = dynamic_cast<const CreateArrayAction*>(action.get());
    if(createArray == nullptr)
    {
      continue;
    }
    ++actualCount;
    CHECK(createArray->dataFormat().empty());
  }
  REQUIRE(actualCount == expectedCount);
}

// -----------------------------------------------------------------------------
void RequireAutomaticCreateArrayActions(const Result<OutputActions>& outputActions, const std::vector<DataPath>& expectedPaths)
{
  REQUIRE(outputActions.valid());
  std::vector<DataPath> actualPaths;
  for(const auto& action : outputActions.value().actions)
  {
    const auto* createArray = dynamic_cast<const CreateArrayAction*>(action.get());
    if(createArray == nullptr || std::find(expectedPaths.begin(), expectedPaths.end(), createArray->path()) == expectedPaths.end())
    {
      continue;
    }
    CHECK(createArray->dataFormat().empty());
    actualPaths.push_back(createArray->path());
  }
  CHECK_THAT(actualPaths, Catch::Matchers::UnorderedEquals(expectedPaths));
}

// -----------------------------------------------------------------------------
void LoadPlugins()
{
  static bool pluginsLoaded = []() {
    const Result<> result = Application::GetOrCreateInstance()->loadPlugins(SIMPLNX_BUILD_DIR, true);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    return true;
  }();
  (void)pluginsLoaded;
}

// -----------------------------------------------------------------------------
void WriteTestDataStructure(const DataStructure& dataStructure, const fs::path& filepath)
{
  Pipeline pipeline;
  const Result<> result2 = DREAM3D::WriteFile(filepath, dataStructure, pipeline, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result2);
}

// -----------------------------------------------------------------------------
void CompareImageGeometry(const ImageGeom* exemplarGeom, const ImageGeom* computedGeom, float32 threshold)
{
  REQUIRE(exemplarGeom != nullptr);
  REQUIRE(computedGeom != nullptr);

  const auto exemplarDims = exemplarGeom->getDimensions();
  const auto computedDims = computedGeom->getDimensions();
  REQUIRE(exemplarDims == computedDims);

  const auto exemplarSpacing = exemplarGeom->getSpacing();
  const auto computedSpacing = computedGeom->getSpacing();
  REQUIRE(std::fabs(exemplarSpacing[0] - computedSpacing[0]) <= threshold);
  REQUIRE(std::fabs(exemplarSpacing[1] - computedSpacing[1]) <= threshold);
  REQUIRE(std::fabs(exemplarSpacing[2] - computedSpacing[2]) <= threshold);

  const auto exemplarOrigin = exemplarGeom->getOrigin();
  const auto computedOrigin = computedGeom->getOrigin();
  REQUIRE(std::fabs(exemplarOrigin[0] - computedOrigin[0]) <= threshold);
  REQUIRE(std::fabs(exemplarOrigin[1] - computedOrigin[1]) <= threshold);
  REQUIRE(std::fabs(exemplarOrigin[2] - computedOrigin[2]) <= threshold);
}

// -----------------------------------------------------------------------------
void CompareImageGeometry(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath, float32 threshold)
{
  INFO(fmt::format("Comparing Image Geometries. {} and {}", exemplaryDataPath.toString(), computedPath.toString()));
  const auto* exemplarGeom = dataStructure.getDataAs<ImageGeom>(exemplaryDataPath);
  const auto* computedGeom = dataStructure.getDataAs<ImageGeom>(computedPath);
  CompareImageGeometry(exemplarGeom, computedGeom, threshold);
}

// -----------------------------------------------------------------------------
bool CompareIGeometry(const IGeometry* geom1, const IGeometry* geom2)
{
  REQUIRE(geom1->getGeomType() == geom2->getGeomType());
  REQUIRE(geom1->getSpatialDimensionality() == geom2->getSpatialDimensionality());
  REQUIRE(geom1->getUnitDimensionality() == geom2->getUnitDimensionality());
  REQUIRE(geom1->getNumberOfCells() == geom2->getNumberOfCells());
  REQUIRE(geom1->findAllChildrenOfType<IArray>().size() == geom2->findAllChildrenOfType<IArray>().size());
  REQUIRE(geom1->getParametricCenter() == geom2->getParametricCenter());

  return true;
}

// -----------------------------------------------------------------------------
void CompareStringArrays(const DataStructure& dataStructure, const DataPath& exemplaryDataPath, const DataPath& computedPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(exemplaryDataPath));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(computedPath));

  const auto& exemplaryDataArray = dataStructure.getDataRefAs<StringArray>(exemplaryDataPath);
  const auto& generatedDataArray = dataStructure.getDataRefAs<StringArray>(computedPath);
  REQUIRE(generatedDataArray.getNumberOfTuples() == exemplaryDataArray.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output StringArray: '{}' bad comparison", exemplaryDataPath.toString(), computedPath.toString()));

  usize start = 0;
  usize end = exemplaryDataArray.getSize();
  for(usize i = start; i < end; i++)
  {
    const auto& oldVal = exemplaryDataArray[i];
    const auto& newVal = generatedDataArray[i];
    REQUIRE(oldVal == newVal);
  }
}

// -----------------------------------------------------------------------------
void CompareStringArrays(const StringArray& exemplar, const StringArray& computed)
{
  REQUIRE(exemplar.getNumberOfTuples() == computed.getNumberOfTuples());

  INFO(fmt::format("Input Data Array:'{}'  Output StringArray: '{}' bad comparison", exemplar.getDataPaths()[0].toString(), computed.getDataPaths()[0].toString()));

  constexpr usize start = 0;
  const usize end = exemplar.getSize();
  for(usize i = start; i < end; i++)
  {
    REQUIRE(exemplar[i] == computed[i]);
  }
}

// -----------------------------------------------------------------------------
void CompareDataStructures(const DataStructure& dataStructureA, const DataStructure& dataStructureB, const DataPath& parentGroup)
{
  try
  {
    std::vector<std::string> childrenNamesA;
    std::vector<std::string> childrenNamesB;
    if(parentGroup.empty())
    {
      // std::cout << "DEBUG TEST: dsA size = " << dataStructureA.getSize() << "\tdsB size = " << dataStructureB.getSize();
      INFO(fmt::format("DEBUG TEST: dsA size = {}\tdsB size = {}", dataStructureA.getSize(), dataStructureB.getSize()));
      REQUIRE(dataStructureA.getSize() == dataStructureB.getSize());
      childrenNamesA = dataStructureA.getDataMap().getNames();
      childrenNamesB = dataStructureB.getDataMap().getNames();
    }
    else
    {
      const auto* parentA = dataStructureA.getDataAs<BaseGroup>(parentGroup);
      const auto* parentB = dataStructureB.getDataAs<BaseGroup>(parentGroup);
      REQUIRE(parentA != nullptr);
      REQUIRE(parentB != nullptr);
      const BaseGroup::GroupType parentAGroupType = parentA->getGroupType();
      const BaseGroup::GroupType parentBGroupType = parentB->getGroupType();
      REQUIRE(parentAGroupType == parentBGroupType);
      // std::cout << "DEBUG TEST: ds parentA size = " << parentA->getSize() << "\tds parentB size = " << parentB->getSize();
      INFO(fmt::format("DEBUG TEST: ds parentA size = {}\tds parentB size = {}", parentA->getSize(), parentB->getSize()));
      REQUIRE(parentA->getSize() == parentB->getSize());
      childrenNamesA = parentA->getDataMap().getNames();
      childrenNamesB = parentB->getDataMap().getNames();

      switch(parentAGroupType)
      {
      case nx::core::BaseGroup::GroupType::AttributeMatrix: {
        const auto* attributeMatrixA = dynamic_cast<const AttributeMatrix*>(parentA);
        const auto* attributeMatrixB = dynamic_cast<const AttributeMatrix*>(parentB);
        REQUIRE(attributeMatrixA != nullptr);
        REQUIRE(attributeMatrixB != nullptr);
        REQUIRE(attributeMatrixA->getShape() == attributeMatrixB->getShape());
        break;
      }
      case nx::core::BaseGroup::GroupType::ImageGeom: {
        const auto* geomA = dynamic_cast<const ImageGeom*>(parentA);
        const auto* geomB = dynamic_cast<const ImageGeom*>(parentB);
        CompareImageGeometry(geomA, geomB, UnitTest::EPSILON);
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::RectGridGeom: {
        const auto* geomA = dynamic_cast<const RectGridGeom*>(parentA);
        const auto* geomB = dynamic_cast<const RectGridGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getDimensions() == geomB->getDimensions());
        const auto originA = geomA->getOrigin();
        const auto originB = geomB->getOrigin();
        REQUIRE(originA.valid());
        REQUIRE(originB.valid());
        const auto originAVec = originA.value();
        const auto originBVec = originB.value();
        REQUIRE(std::fabs(originAVec[0] - originBVec[0]) <= UnitTest::EPSILON);
        REQUIRE(std::fabs(originAVec[1] - originBVec[1]) <= UnitTest::EPSILON);
        REQUIRE(std::fabs(originAVec[2] - originBVec[2]) <= UnitTest::EPSILON);
        CompareDataArrays<float32>(geomA->getXBoundsRef(), geomB->getXBoundsRef());
        CompareDataArrays<float32>(geomA->getYBoundsRef(), geomB->getYBoundsRef());
        CompareDataArrays<float32>(geomA->getZBoundsRef(), geomB->getZBoundsRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::HexahedralGeom:
      case nx::core::BaseGroup::GroupType::TetrahedralGeom: {
        const auto* geomA = dynamic_cast<const INodeGeometry3D*>(parentA);
        const auto* geomB = dynamic_cast<const INodeGeometry3D*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getFaces() != nullptr);
        REQUIRE(geomB->getFaces() != nullptr);
        CompareDataArrays<uint64>(geomA->getFacesRef(), geomB->getFacesRef());
        REQUIRE(geomA->getPolyhedra() != nullptr);
        REQUIRE(geomB->getPolyhedra() != nullptr);
        CompareDataArrays<uint64>(geomA->getPolyhedraRef(), geomB->getPolyhedraRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::QuadGeom:
      case nx::core::BaseGroup::GroupType::TriangleGeom: {
        const auto* geomA = dynamic_cast<const INodeGeometry2D*>(parentA);
        const auto* geomB = dynamic_cast<const INodeGeometry2D*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getFaces() != nullptr);
        REQUIRE(geomB->getFaces() != nullptr);
        CompareDataArrays<uint64>(geomA->getFacesRef(), geomB->getFacesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::EdgeGeom: {
        const auto* geomA = dynamic_cast<const EdgeGeom*>(parentA);
        const auto* geomB = dynamic_cast<const EdgeGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getEdges() != nullptr);
        REQUIRE(geomB->getEdges() != nullptr);
        CompareDataArrays<uint64>(geomA->getEdgesRef(), geomB->getEdgesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::VertexGeom: {
        const auto* geomA = dynamic_cast<const VertexGeom*>(parentA);
        const auto* geomB = dynamic_cast<const VertexGeom*>(parentB);
        REQUIRE(geomA != nullptr);
        REQUIRE(geomB != nullptr);
        REQUIRE(geomA->getVertices() != nullptr);
        REQUIRE(geomB->getVertices() != nullptr);
        CompareDataArrays<float32>(geomA->getVerticesRef(), geomB->getVerticesRef());
        REQUIRE(geomA->getUnitDimensionality() == geomB->getUnitDimensionality());
        REQUIRE(geomA->getSpatialDimensionality() == geomB->getSpatialDimensionality());
        REQUIRE(geomA->getUnits() == geomB->getUnits());
        break;
      }
      case nx::core::BaseGroup::GroupType::DataGroup: {
        break;
      }
      default: {
        INFO(fmt::format("Object at path ({}) has unhandled type ({})", parentGroup.toString(), parentA->getTypeName()));
        REQUIRE(false);
        break;
      }
      }
    }

    REQUIRE(childrenNamesA == childrenNamesB);
    for(const auto& childName : childrenNamesA)
    {
      const DataPath childPath = parentGroup.createChildPath(childName);
      CAPTURE(childPath.toString());
      const DataObject* objectA = dataStructureA.getData(childPath);
      const DataObject* objectB = dataStructureB.getData(childPath);
      REQUIRE(objectA != nullptr);
      REQUIRE(objectB != nullptr);

      const DataObject::Type objectADataObjectType = objectA->getDataObjectType();
      REQUIRE(objectADataObjectType == objectB->getDataObjectType());

      switch(objectADataObjectType)
      {
      case nx::core::DataObject::Type::DynamicListArray: {
        // This recursive helper does not compare DynamicListArray objects.
        break;
      }
      case nx::core::DataObject::Type::ScalarData: {
        // This recursive helper reports ScalarData objects without comparing values.
        std::cout << objectA->getTypeName() << ": " << objectA->getName() << std::endl;
        break;
      }
      case nx::core::DataObject::Type::AbstractMontage: {
        // This recursive helper does not compare AbstractMontage objects.
        break;
      }
      case nx::core::DataObject::Type::IDataArray:
      case nx::core::DataObject::Type::DataArray: {
        const auto* dataArrayA = dynamic_cast<const IDataArray*>(objectA);
        const auto* dataArrayB = dynamic_cast<const IDataArray*>(objectB);
        REQUIRE(dataArrayA != nullptr);
        REQUIRE(dataArrayB != nullptr);
        REQUIRE(dataArrayA->getDataType() == dataArrayB->getDataType());
        // std::cout << "DEBUG TEST: data array A DataType = " << DataTypeToString(dataArrayA->getDataType()) << "\tdata array B DataType = " << DataTypeToString(dataArrayB->getDataType());
        INFO(fmt::format("DEBUG TEST: data array A DataType = {}\tdata array B DataType = {}", DataTypeToString(dataArrayA->getDataType()), DataTypeToString(dataArrayB->getDataType())));

        ExecuteDataFunction(CompareArraysFunctor{}, dataArrayA->getDataType(), *dataArrayA, *dataArrayB);

        break;
      }
      case nx::core::DataObject::Type::StringArray: {
        const auto* stringArrayA = dynamic_cast<const StringArray*>(objectA);
        const auto* stringArrayB = dynamic_cast<const StringArray*>(objectB);
        REQUIRE(stringArrayA != nullptr);
        REQUIRE(stringArrayB != nullptr);
        CompareStringArrays(*stringArrayA, *stringArrayB);
        break;
      }
      case nx::core::DataObject::Type::INeighborList:
      case nx::core::DataObject::Type::NeighborList: {
        const auto* neighborlistA = dynamic_cast<const INeighborList*>(objectA);
        const auto* neighborlistB = dynamic_cast<const INeighborList*>(objectB);
        REQUIRE(neighborlistA != nullptr);
        REQUIRE(neighborlistB != nullptr);
        // std::cout << "DEBUG TEST: neighborlist array A DataType = " << DataTypeToString(neighborlistA->getDataType()) << "\tneighborlist B DataType = " <<
        // DataTypeToString(neighborlistB->getDataType());
        INFO(fmt::format("DEBUG TEST: NeighborList A DataType = {}\tNeighborList B DataType = {}", DataTypeToString(neighborlistA->getDataType()), DataTypeToString(neighborlistB->getDataType())));
        REQUIRE(neighborlistA->getDataType() == neighborlistB->getDataType());

        ExecuteDataFunction(CompareNeighborListsFunctor{}, neighborlistA->getDataType(), neighborlistA, neighborlistB);

        break;
      }
      case nx::core::DataObject::Type::VertexGeom:
      case nx::core::DataObject::Type::EdgeGeom:
      case nx::core::DataObject::Type::RectGridGeom:
      case nx::core::DataObject::Type::ImageGeom:
      case nx::core::DataObject::Type::INodeGeometry2D:
      case nx::core::DataObject::Type::QuadGeom:
      case nx::core::DataObject::Type::TriangleGeom:
      case nx::core::DataObject::Type::INodeGeometry3D:
      case nx::core::DataObject::Type::HexahedralGeom:
      case nx::core::DataObject::Type::TetrahedralGeom:
      case nx::core::DataObject::Type::AttributeMatrix:
      case nx::core::DataObject::Type::DataGroup:
      case nx::core::DataObject::Type::BaseGroup: {
        CompareDataStructures(dataStructureA, dataStructureB, childPath);
        break;
      }
      default: {
        auto underlyingDataType = to_underlying(objectADataObjectType);
        std::cout << "Missing DataType: " << underlyingDataType << std::endl;
        INFO(fmt::format("Object at path ({}) has unhandled type ({})", childPath.toString(), underlyingDataType));
        REQUIRE(false);
        break;
      }
      }
    }
  } catch(std::exception& e)
  {
    INFO(fmt::format("Caught exception: {}", e.what()));
    REQUIRE(false);
  }
}

// -----------------------------------------------------------------------------
DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* topLevelGroup = DataGroup::Create(dataStructure, Constants::k_SmallIN100);
  DataGroup* scanData = DataGroup::Create(dataStructure, Constants::k_EbsdScanData, topLevelGroup->getId());

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, scanData->getId());
  REQUIRE(imageGeom->setSpacing({0.25f, 0.55f, 1.86}).valid());
  REQUIRE(imageGeom->setOrigin({0.0f, 20.0f, 66.0f}).valid());
  SizeVec3 imageGeomDims = {40, 60, 80};
  imageGeom->setDimensions(imageGeomDims); // ImageGeom dimensions use {X, Y, Z} order.

  // The DataStructure owns each array. Local raw pointers remain non-owning.
  usize numComponents = 1;
  ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};

  Float32Array* ci_data = CreateTestDataArray<float32>(dataStructure, Constants::k_ConfidenceIndex, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>(dataStructure, Constants::k_FeatureIds, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>(dataStructure, "Phases", tupleShape, {numComponents}, scanData->getId());
  UInt64Array* voxelIndices = CreateTestDataArray<uint64>(dataStructure, "Voxel Indices", tupleShape, {numComponents}, scanData->getId());

  BoolArray* conditionalArray = CreateTestDataArray<bool>(dataStructure, Constants::k_ConditionalArray, tupleShape, {1}, scanData->getId());
  conditionalArray->fill(true);

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8>(dataStructure, "IPF Colors", tupleShape, {numComponents}, scanData->getId());
  Float32Array* euler_data = CreateTestDataArray<float32>(dataStructure, "Euler", tupleShape, {numComponents}, scanData->getId());

  // The ensemble group stores phase-level data, such as the Laue class.
  DataGroup* ensembleGroup = DataGroup::Create(dataStructure, "Phase Data", topLevelGroup->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32>(dataStructure, "Laue Class", {numTuples}, {numComponents}, ensembleGroup->getId());

  VertexGeom* vertexGeom = VertexGeom::Create(dataStructure, Constants::k_VertexGeometry, scanData->getId());
  vertexGeom->setVertices(*euler_data);

  // NeighborList<float32>* neighborList = CreateTestNeighborList<float32>(dataStructure, "Neighbor List", numTuples, scanData->getId());

  return dataStructure;
}

// -----------------------------------------------------------------------------
DataStructure CreateAllPrimitiveTypes(const ShapeType& tupleShape)
{
  DataStructure dataStructure;
  DataGroup* levelZeroGroup = DataGroup::Create(dataStructure, Constants::k_LevelZero);
  // auto levelZeroId = levelZeroGroup->getId();
  DataGroup* levelOneGroup = DataGroup::Create(dataStructure, Constants::k_LevelOne, levelZeroGroup->getId());
  auto levelOneId = levelOneGroup->getId();
  DataGroup* levelTwoGroup = DataGroup::Create(dataStructure, Constants::k_LevelTwo, levelZeroGroup->getId());
  auto levelTwoId = levelTwoGroup->getId();

  //  // Create an Image Geometry grid for the Scan Data
  //  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, levelOneGroup->getId());
  //  SizeVec3 imageGeomDims = {40, 60, 80};
  //  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)
  //  imageGeom->setSpacing({0.25f, 0.55f, 1.86});
  //  imageGeom->setOrigin({0.0f, 20.0f, 66.0f});

  // DataStore<usize>::ShapeType tupleShape = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
  // The first child group contains scalar arrays.
  ShapeType componentShape = {1ULL};

  CreateTestDataArray<int8>(dataStructure, Constants::k_Int8DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint8>(dataStructure, Constants::k_Uint8DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int16>(dataStructure, Constants::k_Int16DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint16>(dataStructure, Constants::k_Uint16DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int32>(dataStructure, Constants::k_Int32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint32>(dataStructure, Constants::k_Uint32DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<int64>(dataStructure, Constants::k_Int64DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<uint64>(dataStructure, Constants::k_Uint64DataSet, tupleShape, componentShape, levelOneId);

  CreateTestDataArray<float32>(dataStructure, Constants::k_Float32DataSet, tupleShape, componentShape, levelOneId);
  CreateTestDataArray<float64>(dataStructure, Constants::k_Float64DataSet, tupleShape, componentShape, levelOneId);

  // The second child group contains three-component arrays.
  componentShape = {3ULL};
  CreateTestDataArray<int8>(dataStructure, Constants::k_Int8DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint8>(dataStructure, Constants::k_Uint8DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int16>(dataStructure, Constants::k_Int16DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint16>(dataStructure, Constants::k_Uint16DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int32>(dataStructure, Constants::k_Int32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint32>(dataStructure, Constants::k_Uint32DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<int64>(dataStructure, Constants::k_Int64DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<uint64>(dataStructure, Constants::k_Uint64DataSet, tupleShape, componentShape, levelTwoId);

  CreateTestDataArray<float32>(dataStructure, Constants::k_Float32DataSet, tupleShape, componentShape, levelTwoId);
  CreateTestDataArray<float64>(dataStructure, Constants::k_Float64DataSet, tupleShape, componentShape, levelTwoId);

  return dataStructure;
}

// -----------------------------------------------------------------------------
void AddImageGeometry(DataStructure& dataStructure, const SizeVec3& imageGeomDims, const FloatVec3& spacing, const FloatVec3& origin, const DataGroup& dataGroup)
{
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, Constants::k_ImageGeometry, dataGroup.getId());
  if(imageGeom == nullptr)
  {
    throw std::runtime_error("UnitTestCommon: Unable to create ImageGeom");
  }
  imageGeom->setDimensions(imageGeomDims);
  REQUIRE(imageGeom->setSpacing(spacing).valid());
  REQUIRE(imageGeom->setOrigin(origin).valid());
}

// -----------------------------------------------------------------------------
void CompareExemplarToGenerateAttributeMatrix(const DataStructure& exemplarDataStructure, const DataPath& exemplarAttributeMatrix, const DataStructure& computedDataStructure,
                                              const DataPath& computedAttributeMatrix, bool allMustMatch)
{
  auto& exemplarAttrMatr = exemplarDataStructure.getDataRefAs<AttributeMatrix>(exemplarAttributeMatrix);
  // std::vector<DataPath> selectedCellArrays;

  // Each exemplar child selects the corresponding computed array by name.
  for(auto& exemplarArrayPath : exemplarAttrMatr)
  {

    DataPath exemplarDataArrayPath = exemplarAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    DataPath computedDataArrayPath = computedAttributeMatrix.createChildPath(exemplarArrayPath.second->getName());
    INFO(fmt::format("Exemplar Array:'{}'  Computed Array: '{}'", exemplarDataArrayPath.toString(), computedDataArrayPath.toString()));

    const auto* exemplarArrayPtr = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);
    const auto* computedArrayPtr = computedDataStructure.getDataAs<IArray>(computedDataArrayPath);

    // Optional mode omits arrays that are absent from either DataStructure.
    if(nullptr == exemplarArrayPtr && !allMustMatch)
    {
      continue;
    }
    if(nullptr == computedArrayPtr && !allMustMatch)
    {
      continue;
    }

    if(allMustMatch)
    {
      REQUIRE(exemplarArrayPtr != nullptr);
      REQUIRE(computedArrayPtr != nullptr);
    }

    DataType type = DataType::int8;
    const IArray::ArrayType arrayType = computedArrayPtr->getArrayType();
    if(arrayType == IArray::ArrayType::DataArray)
    {
      type = exemplarDataStructure.getDataRefAs<IDataArray>(exemplarDataArrayPath).getDataType();
    }
    if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      type = exemplarDataStructure.getDataRefAs<INeighborList>(exemplarDataArrayPath).getDataType();
    }

    switch(type)
    {
    case DataType::boolean: {
      nx::core::UnitTest::CompareArrays<bool>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int8: {
      nx::core::UnitTest::CompareArrays<int8>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int16: {
      nx::core::UnitTest::CompareArrays<int16>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int32: {
      nx::core::UnitTest::CompareArrays<int32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::int64: {
      nx::core::UnitTest::CompareArrays<int64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint8: {
      nx::core::UnitTest::CompareArrays<uint8>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint16: {
      nx::core::UnitTest::CompareArrays<uint16>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint32: {
      nx::core::UnitTest::CompareArrays<uint32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::uint64: {
      nx::core::UnitTest::CompareArrays<uint64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::float32: {
      nx::core::UnitTest::CompareArrays<float32>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    case DataType::float64: {
      nx::core::UnitTest::CompareArrays<float64>(computedArrayPtr, exemplarArrayPtr);
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
}

// -----------------------------------------------------------------------------
void CompareExemplarToGeneratedData(const DataStructure& dataStructure, const DataStructure& exemplarDataStructure, const DataPath& attributeMatrix, const std::string& exemplarDataContainerName)
{
  auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(attributeMatrix);
  std::vector<DataPath> selectedCellArrays;

  // Snapshot the child paths before the comparison starts.
  for(auto& child : cellDataGroup)
  {
    selectedCellArrays.push_back(attributeMatrix.createChildPath(child.second->getName()));
  }

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    const auto* generatedArray = dataStructure.getDataAs<IArray>(cellArrayPath);
    // The exemplar uses a different top-level container but the same child path.
    std::vector<std::string> generatedPathVector = cellArrayPath.getPathVector();
    generatedPathVector[0] = exemplarDataContainerName;
    DataPath exemplarDataArrayPath(generatedPathVector);
    const auto* exemplarArray = exemplarDataStructure.getDataAs<IArray>(exemplarDataArrayPath);

    // An absent exemplar array means that this generated array has no reference.
    if(nullptr == exemplarArray)
    {
      continue;
    }

    DataType type = DataType::int8;
    const IArray::ArrayType arrayType = generatedArray->getArrayType();
    if(arrayType == IArray::ArrayType::DataArray)
    {
      type = dataStructure.getDataRefAs<IDataArray>(cellArrayPath).getDataType();
    }
    if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      type = dataStructure.getDataRefAs<INeighborList>(cellArrayPath).getDataType();
    }

    switch(type)
    {
    case DataType::boolean: {
      nx::core::UnitTest::CompareArrays<bool>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int8: {
      nx::core::UnitTest::CompareArrays<int8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int16: {
      nx::core::UnitTest::CompareArrays<int16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int32: {
      nx::core::UnitTest::CompareArrays<int32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::int64: {
      nx::core::UnitTest::CompareArrays<int64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint8: {
      nx::core::UnitTest::CompareArrays<uint8>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint16: {
      nx::core::UnitTest::CompareArrays<uint16>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint32: {
      nx::core::UnitTest::CompareArrays<uint32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::uint64: {
      nx::core::UnitTest::CompareArrays<uint64>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float32: {
      nx::core::UnitTest::CompareArrays<float32>(generatedArray, exemplarArray);
      break;
    }
    case DataType::float64: {
      nx::core::UnitTest::CompareArrays<float64>(generatedArray, exemplarArray);
      break;
    }
    default: {
      throw std::runtime_error("Invalid DataType");
    }
    }
  }
}

// -----------------------------------------------------------------------------
void CompareAsciiFiles(std::ifstream& computedFile, std::ifstream& exemplarFile, const std::vector<usize>& lineIndicesToSkip)
{
  std::vector<std::string> computedLines;
  std::vector<std::string> exemplarLines;
  for(std::string line; std::getline(computedFile, line);)
  {
    computedLines.push_back(line);
  }
  for(std::string line; std::getline(exemplarFile, line);)
  {
    exemplarLines.push_back(line);
  }

  REQUIRE(computedLines.size() == exemplarLines.size());
  for(usize i = 0; i < computedLines.size(); ++i)
  {
    if(std::find(begin(lineIndicesToSkip), end(lineIndicesToSkip), i) != std::end(lineIndicesToSkip))
    {
      continue;
    }

    REQUIRE(computedLines[i] == exemplarLines[i]);
  }
}

// -----------------------------------------------------------------------------
DataStructure CreateComplexMultiLevelDataGraph()
{
  DataStructure dataStructure;

  // Create the two root groups.
  auto* groupA = DataGroup::Create(dataStructure, Constants::k_GroupAName);
  auto* groupB = DataGroup::Create(dataStructure, Constants::k_GroupBName);

  auto groupAPath = DataPath({groupA->getName()});
  auto groupBPath = DataPath({groupB->getName()});

  // Create level-one groups, including shared group C.
  auto* groupH = DataGroup::Create(dataStructure, Constants::k_GroupHName, groupA->getId());
  auto* groupC = DataGroup::Create(dataStructure, Constants::k_GroupCName, groupA->getId());
  groupB->insert(dataStructure.getSharedData(groupC->getId()));
  auto* groupF = DataGroup::Create(dataStructure, Constants::k_GroupFName, groupB->getId());

  auto groupAHPath = groupAPath.createChildPath(groupH->getName());

  auto groupACPath = groupAPath.createChildPath(groupC->getName());
  auto groupBCPath = groupBPath.createChildPath(groupC->getName());

  auto groupBFPath = groupBPath.createChildPath(groupF->getName());

  // Create level-two groups, including shared group E.
  auto* groupD = DataGroup::Create(dataStructure, Constants::k_GroupDName, groupC->getId());
  auto* groupE = DataGroup::Create(dataStructure, Constants::k_GroupEName, groupC->getId());
  groupB->insert(dataStructure.getSharedData(groupE->getId()));
  groupF->insert(dataStructure.getSharedData(groupE->getId()));
  auto* groupG = DataGroup::Create(dataStructure, Constants::k_GroupGName, groupF->getId());
  auto* arrayN = CreateTestDataArray<int8>(dataStructure, Constants::k_ArrayNName, {1ULL}, {1ULL}, groupH->getId());

  // groupAHPath.createChildPath(arrayN->getName());

  auto groupACDPath = groupACPath.createChildPath(groupD->getName());
  auto groupBCDPath = groupBCPath.createChildPath(groupD->getName());

  auto groupACEPath = groupACPath.createChildPath(groupE->getName());
  auto groupBCEPath = groupBCPath.createChildPath(groupE->getName());
  auto groupBEPath = groupBPath.createChildPath(groupE->getName());
  auto groupBFEPath = groupBFPath.createChildPath(groupE->getName());

  auto groupBFGPath = groupBFPath.createChildPath(groupG->getName());

  // Create arrays that can have paths through more than one parent.
  auto* arrayI = CreateTestDataArray<uint8>(dataStructure, Constants::k_ArrayIName, {1ULL}, {1ULL}, groupD->getId());
  auto* arrayJ = CreateTestDataArray<float32>(dataStructure, Constants::k_ArrayJName, {1ULL}, {1ULL}, groupD->getId());
  groupE->insert(dataStructure.getSharedData(arrayJ->getId()));
  auto* arrayK = CreateTestDataArray<float64>(dataStructure, Constants::k_ArrayKName, {1ULL}, {1ULL}, groupE->getId());
  groupG->insert(dataStructure.getSharedData(arrayK->getId()));
  auto* arrayL = CreateTestDataArray<uint32>(dataStructure, Constants::k_ArrayLName, {1ULL}, {1ULL}, groupG->getId());
  auto* arrayM = CreateTestDataArray<int64>(dataStructure, Constants::k_ArrayMName, {1ULL}, {1ULL}, groupG->getId());

  groupACDPath.createChildPath(arrayI->getName());
  groupBCDPath.createChildPath(arrayI->getName());

  groupBCDPath.createChildPath(arrayJ->getName());
  groupACEPath.createChildPath(arrayJ->getName());
  groupBCEPath.createChildPath(arrayJ->getName());
  groupBEPath.createChildPath(arrayJ->getName());
  groupBFEPath.createChildPath(arrayJ->getName());

  groupACEPath.createChildPath(arrayK->getName());
  groupBCEPath.createChildPath(arrayK->getName());
  groupBEPath.createChildPath(arrayK->getName());
  groupBFEPath.createChildPath(arrayK->getName());

  groupBFGPath.createChildPath(arrayL->getName());

  groupBFGPath.createChildPath(arrayM->getName());

  return dataStructure;
}

// -----------------------------------------------------------------------------
void CheckArraysInheritTupleDims(const DataStructure& dataStructure, const std::vector<DataPath>& ignoredPaths)
{
  std::optional<std::vector<DataPath>> amPathsOpt = GetAllChildDataPathsRecursive(dataStructure, {}, DataObject::Type::AttributeMatrix);
  REQUIRE(amPathsOpt.has_value());
  for(const auto& amPath : amPathsOpt.value())
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(amPath));
    const auto& attrMatrix = dataStructure.getDataRefAs<AttributeMatrix>(amPath);
    std::optional<std::vector<DataPath>> daPathsOpt = GetAllChildArrayDataPaths(dataStructure, amPath, ignoredPaths);
    REQUIRE(daPathsOpt.has_value());
    for(const auto& daPath : daPathsOpt.value())
    {
      INFO(fmt::format("AttributeMatrix: {}  Array: '{}'", amPath.toString(), daPath.toString()));
      const auto& arr = dataStructure.getDataAs<IArray>(daPath);
      REQUIRE(attrMatrix.getShape() == arr->getTupleShape());
    }
  }
}

} // namespace nx::core::UnitTest

namespace nx::core::UnitTest::Cropping
{
// -----------------------------------------------------------------------------
std::string BoolToString(bool v)
{
  return v ? "True" : "False";
}

// -----------------------------------------------------------------------------
std::string CropTypeToString(CropGeometryParameter::CropValues::TypeEnum t)
{
  using T = CropGeometryParameter::CropValues::TypeEnum;
  switch(t)
  {
  case T::NoCropping:
    return "NoCropping";
  case T::VoxelSubvolume:
    return "VoxelSubvolume";
  case T::PhysicalSubvolume:
    return "PhysicalSubvolume";
  }
  return "Unknown";
}

// -----------------------------------------------------------------------------
std::vector<CropGeometryParameter::ValueType> GenerateAllCropValues(const AxisBoundsChoices& C, bool is2D)
{
  std::vector<CropGeometryParameter::ValueType> out;

  // The first value represents the no-cropping case.
  {
    CropGeometryParameter::ValueType cv;
    cv.type = CropGeometryParameter::CropValues::TypeEnum::NoCropping;
    cv.cropX = false;
    cv.cropY = false;
    cv.cropZ = false;
    cv.is2D = is2D;
    out.push_back(cv);
  }

  // Two-dimensional cases use only X and Y combinations. Three-dimensional
  // cases use each nonempty combination of X, Y, and Z.
  const std::vector<std::tuple<bool, bool, bool>> flagOrder = is2D ? std::vector<std::tuple<bool, bool, bool>>{{false, true, false}, {true, false, false}, {true, true, false}} :
                                                                     std::vector<std::tuple<bool, bool, bool>>{{false, false, true}, {false, true, false}, {false, true, true}, {true, false, false},
                                                                                                               {true, false, true},  {true, true, false},  {true, true, true}};

  // Generate the voxel-bounds combinations first.
  for(const auto& [cx, cy, cz] : flagOrder)
  {
    std::vector<std::optional<SizeVec2>> xOpts = cx ? std::vector<std::optional<SizeVec2>>(C.voxelX.begin(), C.voxelX.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    std::vector<std::optional<SizeVec2>> yOpts = cy ? std::vector<std::optional<SizeVec2>>(C.voxelY.begin(), C.voxelY.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    std::vector<std::optional<SizeVec2>> zOpts = (!is2D && cz) ? std::vector<std::optional<SizeVec2>>(C.voxelZ.begin(), C.voxelZ.end()) : std::vector<std::optional<SizeVec2>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = false;
          cv.is2D = is2D;

          if(xb)
          {
            cv.xBoundVoxels = *xb;
          }
          if(yb)
          {
            cv.yBoundVoxels = *yb;
          }

          // Two-dimensional crop values never set a Z bound.
          if(!is2D && zb)
          {
            cv.zBoundVoxels = *zb;
            cv.cropZ = cz;
          }

          out.push_back(cv);
        }
      }
    }
  }

  // Generate the physical-bounds combinations after the voxel combinations.
  for(const auto& [cx, cy, cz] : flagOrder)
  {
    std::vector<std::optional<FloatVec2Type>> xOpts = cx ? std::vector<std::optional<FloatVec2Type>>(C.physX.begin(), C.physX.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    std::vector<std::optional<FloatVec2Type>> yOpts = cy ? std::vector<std::optional<FloatVec2Type>>(C.physY.begin(), C.physY.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    std::vector<std::optional<FloatVec2Type>> zOpts =
        (!is2D && cz) ? std::vector<std::optional<FloatVec2Type>>(C.physZ.begin(), C.physZ.end()) : std::vector<std::optional<FloatVec2Type>>{std::nullopt};

    for(const auto& xb : xOpts)
    {
      for(const auto& yb : yOpts)
      {
        for(const auto& zb : zOpts)
        {
          CropGeometryParameter::ValueType cv;
          cv.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
          cv.cropX = cx;
          cv.cropY = cy;
          cv.cropZ = false;
          cv.is2D = is2D;

          if(xb)
          {
            cv.xBoundPhysical = *xb;
          }
          if(yb)
          {
            cv.yBoundPhysical = *yb;
          }

          // Two-dimensional crop values never set a Z bound.
          if(!is2D && zb)
          {
            cv.zBoundPhysical = *zb;
            cv.cropZ = cz;
          }

          out.push_back(cv);
        }
      }
    }
  }

  return out;
}

} // namespace nx::core::UnitTest::Cropping

namespace SmallIn100
{
// -----------------------------------------------------------------------------
void ExecuteMultiThresholdObjects(DataStructure& dataStructure, const FilterList& filterList, bool useBoolOutputType)
{
  constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds_object";
  constexpr StringLiteral k_CreatedDataPath_Key = "output_data_array_name";
  constexpr StringLiteral k_CreatedMaskType_Key = "created_mask_type";

  INFO(fmt::format("Error creating Filter '{}'  ", k_MultiThresholdObjectsFilterHandle.getFilterName()));

  auto filter = filterList.createFilter(k_MultiThresholdObjectsFilterHandle);
  REQUIRE(nullptr != filter);

  Arguments args;

  ArrayThresholdSet arrayThresholdset;
  ArrayThresholdSet::CollectionType thresholds;

  std::shared_ptr<ArrayThreshold> ciThreshold = std::make_shared<ArrayThreshold>();
  ciThreshold->setArrayPath(nx::core::Constants::k_ConfidenceIndexArrayPath);
  ciThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  ciThreshold->setComparisonValue(0.1);
  thresholds.push_back(ciThreshold);

  std::shared_ptr<ArrayThreshold> iqThreshold = std::make_shared<ArrayThreshold>();
  iqThreshold->setArrayPath(nx::core::Constants::k_ImageQualityArrayPath);
  iqThreshold->setComparisonType(ArrayThreshold::ComparisonType::GreaterThan);
  iqThreshold->setComparisonValue(120.0);
  thresholds.push_back(iqThreshold);

  arrayThresholdset.setArrayThresholds(thresholds);

  args.insertOrAssign(k_ArrayThresholds_Key, std::make_any<ArrayThresholdsParameter::ValueType>(arrayThresholdset));
  args.insertOrAssign(k_CreatedDataPath_Key, std::make_any<std::string>(nx::core::Constants::k_Mask));
  if(useBoolOutputType)
  {
    args.insertOrAssign(k_CreatedMaskType_Key, std::make_any<DataTypeParameter::ValueType>(DataType::boolean));
  }

  // Preflight verifies that the exemplar contains the required threshold arrays.
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execution creates the compatibility mask and must preserve tuple shapes.
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}

// -----------------------------------------------------------------------------
void ExecuteIdentifySample(DataStructure& dataStructure, const FilterList& filterList)
{
  INFO(fmt::format("Error creating Filter '{}'  ", k_IdentifySampleFilterHandle.getFilterName()));
  auto filter = filterList.createFilter(k_IdentifySampleFilterHandle);
  REQUIRE(nullptr != filter);

  // These keys select the inputs used by the legacy Small IN100 workflow.
  constexpr StringLiteral k_FillHoles_Key = "fill_holes";
  constexpr StringLiteral k_ImageGeom_Key = "input_image_geometry_path";
  constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";

  Arguments args;
  args.insertOrAssign(k_FillHoles_Key, std::make_any<BoolParameter::ValueType>(false));
  args.insertOrAssign(k_ImageGeom_Key, std::make_any<GeometrySelectionParameter::ValueType>(nx::core::Constants::k_DataContainerPath));
  args.insertOrAssign(k_MaskArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(nx::core::Constants::k_MaskArrayPath));

  // Preflight verifies the legacy image geometry and mask paths.
  auto preflightResult = filter->preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execution updates the mask and must preserve tuple shapes.
  auto executeResult = filter->execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure, k_TupleCheckIgnoredPaths);
}

} // namespace SmallIn100
