#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/EmptyListStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataStoreFormatResolver.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataArrayIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataIOManager.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIOInternal.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <H5Dpublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>
#include <algorithm>
#include <atomic>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
// These constants define paths used by the loading API scenarios.
constexpr StringLiteral k_GroupName = "TopGroup";
constexpr StringLiteral k_SmallAttrMatName = "SmallAM";
constexpr StringLiteral k_LargeAttrMatName = "LargeAM";
constexpr StringLiteral k_SmallArrayName = "SmallArray";
constexpr StringLiteral k_LargeArrayName = "LargeArray";

constexpr usize k_SmallArraySize = 10;
constexpr usize k_LargeArraySize = 100;

// These paths select arrays and their ancestor containers.
const DataPath k_GroupPath({k_GroupName});
const DataPath k_SmallAMPath({k_GroupName, k_SmallAttrMatName});
const DataPath k_LargeAMPath({k_GroupName, k_LargeAttrMatName});
const DataPath k_SmallArrayPath({k_GroupName, k_SmallAttrMatName, k_SmallArrayName});
const DataPath k_LargeArrayPath({k_GroupName, k_LargeAttrMatName, k_LargeArrayName});

// These paths select the multi-group prune fixture.
constexpr StringLiteral k_GroupAName = "GroupA";
constexpr StringLiteral k_GroupBName = "GroupB";
constexpr StringLiteral k_AttrMatAName = "AttrMatA";
constexpr StringLiteral k_AttrMatBName = "AttrMatB";
constexpr StringLiteral k_ArrayA1Name = "ArrayA1";
constexpr StringLiteral k_ArrayA2Name = "ArrayA2";
constexpr StringLiteral k_ArrayB1Name = "ArrayB1";
constexpr StringLiteral k_ArrayB2Name = "ArrayB2";
constexpr usize k_PruneArraySize = 20;

const DataPath k_GroupAPath({k_GroupAName});
const DataPath k_AttrMatAPath({k_GroupAName, k_AttrMatAName});
const DataPath k_ArrayA1Path({k_GroupAName, k_AttrMatAName, k_ArrayA1Name});
const DataPath k_ArrayA2Path({k_GroupAName, k_AttrMatAName, k_ArrayA2Name});
const DataPath k_GroupBPath({k_GroupBName});
const DataPath k_AttrMatBPath({k_GroupBName, k_AttrMatBName});
const DataPath k_ArrayB1Path({k_GroupBName, k_AttrMatBName, k_ArrayB1Name});
const DataPath k_ArrayB2Path({k_GroupBName, k_AttrMatBName, k_ArrayB2Name});

// These paths select numeric and nonnumeric sibling arrays from one AttributeMatrix.
constexpr StringLiteral k_NonnumericGroupName = "NonnumericGroup";
constexpr StringLiteral k_NonnumericAttrMatName = "NonnumericAM";
constexpr StringLiteral k_NonnumericNumericArrayName = "Numeric";
constexpr StringLiteral k_NonnumericRecoverySiblingName = "RecoverySibling";
constexpr StringLiteral k_NonnumericStringArrayName = "Strings";
constexpr StringLiteral k_NonnumericNeighborListName = "Neighbors";
constexpr usize k_NonnumericTupleCount = 3;

const DataPath k_NonnumericGroupPath({k_NonnumericGroupName});
const DataPath k_NonnumericAttrMatPath({k_NonnumericGroupName, k_NonnumericAttrMatName});
const DataPath k_NonnumericNumericArrayPath({k_NonnumericGroupName, k_NonnumericAttrMatName, k_NonnumericNumericArrayName});
const DataPath k_NonnumericRecoverySiblingPath({k_NonnumericGroupName, k_NonnumericAttrMatName, k_NonnumericRecoverySiblingName});
const DataPath k_NonnumericStringArrayPath({k_NonnumericGroupName, k_NonnumericAttrMatName, k_NonnumericStringArrayName});
const DataPath k_NonnumericNeighborListPath({k_NonnumericGroupName, k_NonnumericAttrMatName, k_NonnumericNeighborListName});

std::atomic_size_t g_ExcludedMaterializations = 0;

class ImportSelectionFloatArrayIO : public HDF5::Float32ArrayIO
{
public:
  Result<> finishImportingData(DataStructure& dataStructure, const DataPath& dataPath, const group_reader_type& parentGroupReader) const override
  {
    if(dataPath == k_LargeArrayPath)
    {
      ++g_ExcludedMaterializations;
    }
    return HDF5::Float32ArrayIO::finishImportingData(dataStructure, dataPath, parentGroupReader);
  }
};

struct ScopedImportSelectionManager
{
  DataIOCollection& collection;
  std::shared_ptr<IDataIOManager> original;

  explicit ScopedImportSelectionManager(DataIOCollection& ioCollection)
  : collection(ioCollection)
  , original(collection.getManager("HDF5"))
  {
    REQUIRE(original != nullptr);
    auto replacement = std::make_shared<HDF5::DataIOManager>();
    replacement->addFactory<ImportSelectionFloatArrayIO>();
    auto result = collection.addIOManager(replacement);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  ~ScopedImportSelectionManager()
  {
    (void)collection.addIOManager(original);
  }

  ScopedImportSelectionManager(const ScopedImportSelectionManager&) = delete;
  ScopedImportSelectionManager(ScopedImportSelectionManager&&) = delete;
  ScopedImportSelectionManager& operator=(const ScopedImportSelectionManager&) = delete;
  ScopedImportSelectionManager& operator=(ScopedImportSelectionManager&&) = delete;
};

/**
 * @brief Replaces optional import finalizers with core-only managers.
 *
 * This guard forces the public loader's direct in-memory route.
 * It restores each replaced manager during stack
 * unwinding.
 */
class CoreImportOnlyManager : public IDataIOManager
{
public:
  explicit CoreImportOnlyManager(std::string formatName)
  : m_FormatName(std::move(formatName))
  {
  }

  std::string formatName() const override
  {
    return m_FormatName;
  }

private:
  std::string m_FormatName;
};

struct ScopedCoreImportManagers
{
  DataIOCollection& collection;
  std::vector<std::pair<std::string, std::shared_ptr<IDataIOManager>>> originals;

  explicit ScopedCoreImportManagers(DataIOCollection& ioCollection)
  : collection(ioCollection)
  {
    // Snapshot before replacement because DataIOCollection owns a mutable map.
    // Replacing a manager while a range-for loop references that map can change
    // the current entry's shared pointer.
    for(const auto& [formatName, manager] : collection)
    {
      if(manager->finalizesImport())
      {
        originals.emplace_back(formatName, manager);
      }
    }

    for(const auto& original : originals)
    {
      auto result = collection.addIOManager(std::make_shared<CoreImportOnlyManager>(original.first));
      if(result.invalid())
      {
        restore();
        SIMPLNX_RESULT_REQUIRE_VALID(result);
      }
    }
  }

  ~ScopedCoreImportManagers()
  {
    restore();
  }

  void restore()
  {
    for(const auto& original : originals)
    {
      (void)collection.addIOManager(original.second);
    }
    originals.clear();
  }

  ScopedCoreImportManagers(const ScopedCoreImportManagers&) = delete;
  ScopedCoreImportManagers(ScopedCoreImportManagers&&) = delete;
  ScopedCoreImportManagers& operator=(const ScopedCoreImportManagers&) = delete;
  ScopedCoreImportManagers& operator=(ScopedCoreImportManagers&&) = delete;
};

/**
 * @brief Returns a distinct warning before core materializes deferred placeholders.
 */
class WarningImportFinalizerManager : public HDF5::DataIOManager
{
public:
  bool finalizesImport() const override
  {
    return true;
  }

  Result<> onImportFinalize(DataStructure&, const std::vector<DataPath>&, const HDF5::FileIO&) override
  {
    return MakeWarningVoidResult(-89201, "Synthetic import-finalizer warning");
  }
};

struct ScopedWarningImportFinalizerManager
{
  DataIOCollection& collection;
  std::shared_ptr<IDataIOManager> original;

  explicit ScopedWarningImportFinalizerManager(DataIOCollection& ioCollection)
  : collection(ioCollection)
  , original(collection.getManager("HDF5"))
  {
    REQUIRE(original != nullptr);
    auto result = collection.addIOManager(std::make_shared<WarningImportFinalizerManager>());
    if(result.invalid())
    {
      restore();
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
  }

  ~ScopedWarningImportFinalizerManager()
  {
    restore();
  }

  void restore()
  {
    (void)collection.addIOManager(original);
  }

  ScopedWarningImportFinalizerManager(const ScopedWarningImportFinalizerManager&) = delete;
  ScopedWarningImportFinalizerManager(ScopedWarningImportFinalizerManager&&) = delete;
  ScopedWarningImportFinalizerManager& operator=(const ScopedWarningImportFinalizerManager&) = delete;
  ScopedWarningImportFinalizerManager& operator=(ScopedWarningImportFinalizerManager&&) = delete;
};

class RecordingResolver : public IDataStoreFormatResolver
{
public:
  mutable std::vector<DataPath> paths;
  mutable std::vector<DataType> types;
  mutable std::vector<uint64> bytes;
  std::vector<DataPath> requiredContextPaths;
  mutable bool contextWasComplete = true;
  std::string failureMessage;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, const DataPath& path, DataType numericType, uint64 logicalBytes) const override
  {
    paths.push_back(path);
    types.push_back(numericType);
    bytes.push_back(logicalBytes);
    for(const auto& requiredPath : requiredContextPaths)
    {
      contextWasComplete = contextWasComplete && dataStructure.containsData(requiredPath);
    }
    if(!failureMessage.empty())
    {
      throw std::runtime_error(failureMessage);
    }
    return {};
  }
};

class ThrowingCountingResolver : public IDataStoreFormatResolver
{
public:
  mutable usize calls = 0;

  std::string resolveFormat([[maybe_unused]] const DataStructure& dataStructure, [[maybe_unused]] const DataPath& path, [[maybe_unused]] DataType numericType,
                            [[maybe_unused]] uint64 logicalBytes) const override
  {
    ++calls;
    throw std::runtime_error("materialization must not replay storage policy");
  }
};

class RecordingImportFinalizer : public IDataIOManager
{
public:
  explicit RecordingImportFinalizer(std::string formatName)
  : m_FormatName(std::move(formatName))
  {
  }

  std::string formatName() const override
  {
    return m_FormatName;
  }

  bool finalizesImport() const override
  {
    return m_Finalizes;
  }

  Result<> onImportFinalize([[maybe_unused]] DataStructure& dataStructure, const std::vector<DataPath>& paths, [[maybe_unused]] const HDF5::FileIO& fileReader) override
  {
    calls.push_back(paths);
    return {};
  }

  void disable()
  {
    m_Finalizes = false;
  }

  std::vector<std::vector<DataPath>> calls;

private:
  std::string m_FormatName;
  bool m_Finalizes = true;
};

struct ScopedRecordingImportFinalizer
{
  DataIOCollection& collection;
  std::shared_ptr<IDataIOManager> original;
  std::shared_ptr<RecordingImportFinalizer> recorder;

  explicit ScopedRecordingImportFinalizer(DataIOCollection& ioCollection)
  : collection(ioCollection)
  {
    for(const auto& [name, manager] : collection)
    {
      if(manager->finalizesImport())
      {
        original = manager;
        break;
      }
    }
    const std::string formatName = original == nullptr ? "Task4-Recording-Finalizer" : original->formatName();
    recorder = std::make_shared<RecordingImportFinalizer>(formatName);
    REQUIRE(collection.addIOManager(recorder).valid());
  }

  ~ScopedRecordingImportFinalizer()
  {
    if(original != nullptr)
    {
      (void)collection.addIOManager(original);
    }
    else
    {
      recorder->disable();
    }
  }

  ScopedRecordingImportFinalizer(const ScopedRecordingImportFinalizer&) = delete;
  ScopedRecordingImportFinalizer(ScopedRecordingImportFinalizer&&) = delete;
  ScopedRecordingImportFinalizer& operator=(const ScopedRecordingImportFinalizer&) = delete;
  ScopedRecordingImportFinalizer& operator=(ScopedRecordingImportFinalizer&&) = delete;
};

// The helpers create and clean temporary DREAM3D files.

/**
 * @struct ScopedTempFile
 * @brief Removes one temporary file when the guard is destroyed.
 *
 * Ensures test-output files are cleaned up even when a REQUIRE assertion
 * throws and skips the remaining test body.
 */
struct ScopedTempFile
{
  explicit ScopedTempFile(fs::path p)
  : path(std::move(p))
  {
  }
  ~ScopedTempFile()
  {
    std::error_code ec;
    fs::remove(path, ec);
  }
  fs::path path;
};

/**
 * @brief Returns the binary test-output directory.
 * @return Directory for temporary DREAM3D files.
 */
fs::path GetTestOutputDir()
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view());
}

/**
 * @brief Creates a two-array DataStructure for loading tests.
 * @return A hierarchy with independent small and large AttributeMatrices.
 *
 * Separate AttributeMatrices permit different tuple dimensions. SmallArray
 * values use `i * 3`; LargeArray values use `i * 1.5`.
 */
DataStructure CreateSimpleTestDataStructure()
{
  DataStructure ds;

  auto* group = DataGroup::Create(ds, k_GroupName);
  REQUIRE(group != nullptr);

  // SmallAM holds SmallArray (10 tuples)
  auto* smallAM = AttributeMatrix::Create(ds, k_SmallAttrMatName, {k_SmallArraySize}, group->getId());
  REQUIRE(smallAM != nullptr);

  auto smallStore = std::make_unique<DataStore<int32>>(std::vector<usize>{k_SmallArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    smallStore->setValue(i, static_cast<int32>(i * 3));
  }
  auto* smallArray = DataArray<int32>::Create(ds, k_SmallArrayName, std::move(smallStore), smallAM->getId());
  REQUIRE(smallArray != nullptr);

  // LargeAM holds LargeArray (100 tuples)
  auto* largeAM = AttributeMatrix::Create(ds, k_LargeAttrMatName, {k_LargeArraySize}, group->getId());
  REQUIRE(largeAM != nullptr);

  auto largeStore = std::make_unique<DataStore<float32>>(std::vector<usize>{k_LargeArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_LargeArraySize; ++i)
  {
    largeStore->setValue(i, static_cast<float32>(i) * 1.5f);
  }
  auto* largeArray = DataArray<float32>::Create(ds, k_LargeArrayName, std::move(largeStore), largeAM->getId());
  REQUIRE(largeArray != nullptr);

  return ds;
}

/**
 * @brief Writes a DataStructure to a temporary DREAM3D file.
 * @param ds DataStructure to write.
 * @param fileName Output file name.
 * @return The written file path.
 */
fs::path WriteTestFile(const DataStructure& ds, const std::string& fileName)
{
  fs::path outputPath = GetTestOutputDir() / fileName;
  Result<> writeResult = DREAM3D::WriteFile(outputPath, ds);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(fs::exists(outputPath));
  return outputPath;
}

fs::path CreateMalformedCurrentArrayFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_CurrentFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult);
  auto dataStructureGroup = fileWriter.createGroup(Constants::k_DataStructureTag);
  REQUIRE(dataStructureGroup.isValid());
  auto writeScalarAttributeResult = dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, DataObject::IdType{2});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult);

  auto dataset = dataStructureGroup.createDataset("MalformedArray");
  const std::vector<int32> values = {1, 2, 3};
  auto sizeResult = dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(sizeResult);
  auto writeStringAttributeResult2 = dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<int32>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult2);
  auto writeScalarAttributeResult2 = dataset.writeScalarAttribute(Constants::k_ObjectIdTag, DataObject::IdType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult2);
  auto writeScalarAttributeResult3 = dataset.writeScalarAttribute(Constants::k_ImportableTag, int32{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult3);
  auto writeVectorAttributeResult = dataset.writeVectorAttribute(IOConstants::k_TupleShapeTag, ShapeType{3});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult);
  return outputPath;
}

fs::path CreateCorruptCurrentArrayFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult3 = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_CurrentFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult3);
  auto dataStructureGroup = fileWriter.createGroup(Constants::k_DataStructureTag);
  REQUIRE(dataStructureGroup.isValid());
  auto writeScalarAttributeResult4 = dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, DataObject::IdType{2});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult4);

  auto dataset = dataStructureGroup.createDataset("CorruptShapeArray");
  const std::vector<int32> values = {1, 2, 3};
  auto sizeResult2 = dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(sizeResult2);
  auto writeStringAttributeResult4 = dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<int32>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult4);
  auto writeScalarAttributeResult5 = dataset.writeScalarAttribute(Constants::k_ObjectIdTag, DataObject::IdType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult5);
  auto writeScalarAttributeResult6 = dataset.writeScalarAttribute(Constants::k_ImportableTag, int32{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult6);
  auto writeVectorAttributeResult2 = dataset.writeVectorAttribute(IOConstants::k_TupleShapeTag, ShapeType{3});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult2);
  auto writeStringAttributeResult5 = dataset.writeStringAttribute(IOConstants::k_ComponentShapeTag.str(), "not-a-numeric-shape");
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult5);
  return outputPath;
}

fs::path CreateMissingImportableCurrentArrayFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  REQUIRE(fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_CurrentFileVersion.str()).valid());
  auto dataStructureGroup = fileWriter.createGroup(Constants::k_DataStructureTag);
  REQUIRE(dataStructureGroup.isValid());
  REQUIRE(dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, DataObject::IdType{2}).valid());

  auto dataset = dataStructureGroup.createDataset("MissingImportableArray");
  const std::vector<int32> values = {1, 2, 3};
  REQUIRE(dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size())).valid());
  REQUIRE(dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<int32>::GetTypeName()).valid());
  REQUIRE(dataset.writeScalarAttribute(Constants::k_ObjectIdTag, DataObject::IdType{1}).valid());
  REQUIRE(dataset.writeVectorAttribute(IOConstants::k_TupleShapeTag, ShapeType{3}).valid());
  REQUIRE(dataset.writeVectorAttribute(IOConstants::k_ComponentShapeTag, ShapeType{1}).valid());
  return outputPath;
}

fs::path CreateOverflowCurrentArrayFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult6 = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_CurrentFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult6);
  auto dataStructureGroup = fileWriter.createGroup(Constants::k_DataStructureTag);
  REQUIRE(dataStructureGroup.isValid());
  auto writeScalarAttributeResult7 = dataStructureGroup.writeScalarAttribute(Constants::k_NextIdTag, DataObject::IdType{2});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult7);

  const hid_t groupId = dataStructureGroup.getId();
  {
    std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
    const hsize_t zero = 0;
    const hid_t dataspaceId = H5Screate_simple(1, &zero, nullptr);
    REQUIRE(dataspaceId >= 0);
    const hid_t datasetId = H5Dcreate2(groupId, "OverflowArray", H5T_NATIVE_INT32, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(datasetId >= 0);
    REQUIRE(H5Dclose(datasetId) >= 0);
    REQUIRE(H5Sclose(dataspaceId) >= 0);
  }

  auto dataset = dataStructureGroup.openDataset("OverflowArray");
  auto writeStringAttributeResult7 = dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<int32>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult7);
  auto writeScalarAttributeResult8 = dataset.writeScalarAttribute(Constants::k_ObjectIdTag, DataObject::IdType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult8);
  auto writeScalarAttributeResult9 = dataset.writeScalarAttribute(Constants::k_ImportableTag, int32{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeScalarAttributeResult9);
  auto writeVectorAttributeResult3 = dataset.writeVectorAttribute(IOConstants::k_TupleShapeTag, ShapeType{std::numeric_limits<usize>::max(), 2});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult3);
  auto writeVectorAttributeResult4 = dataset.writeVectorAttribute(IOConstants::k_ComponentShapeTag, ShapeType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult4);
  return outputPath;
}

template <typename T>
void WriteLegacyArray(HDF5::GroupIO& group, const std::string& name, const ShapeType& tupleShape, const ShapeType& componentShape, const std::vector<T>& values)
{
  auto dataset = group.createDataset(name);
  ShapeType physicalShape = tupleShape;
  physicalShape.insert(physicalShape.end(), componentShape.begin(), componentShape.end());
  auto dataResult = dataset.writeSpan<T>(physicalShape, nonstd::span<const T>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(dataResult);
  auto writeStringAttributeResult8 = dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<T>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult8);
  auto writeVectorAttributeResult5 = dataset.writeVectorAttribute("TupleDimensions", tupleShape);
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult5);
  auto writeVectorAttributeResult6 = dataset.writeVectorAttribute("ComponentDimensions", componentShape);
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult6);
}

HDF5::GroupIO CreateLegacyGeometryGroup(HDF5::GroupIO& containers, const std::string& containerName, const std::string& geometryType)
{
  auto container = containers.createGroup(containerName);
  auto geometry = container.createGroup("_SIMPL_GEOMETRY");
  REQUIRE(geometry.isValid());
  auto writeStringAttributeResult9 = geometry.writeStringAttribute("GeometryTypeName", geometryType);
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult9);
  auto assertionResult = geometry.writeScalarAttribute<int32>("SpatialDimensionality", 3);
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult);
  auto assertionResult2 = geometry.writeScalarAttribute<int32>("UnitDimensionality", 3);
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult2);
  return geometry;
}

fs::path CreateLegacyGeometryFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult10 = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_LegacyFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult10);
  auto containers = fileWriter.createGroup("DataContainers");
  const std::vector<float32> vertices = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F, 11.0F};

  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyVertex", "VertexGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {4}, {3}, vertices);
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyEdge", "EdgeGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {4}, {3}, vertices);
    WriteLegacyArray<uint64>(geometry, "SharedEdgeList", {2}, {2}, {0, 1, 2, 3});
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyTriangle", "TriangleGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {4}, {3}, vertices);
    WriteLegacyArray<uint64>(geometry, "SharedTriList", {1}, {3}, {0, 1, 2});
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyQuad", "QuadrilateralGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {4}, {3}, vertices);
    WriteLegacyArray<uint64>(geometry, "SharedQuadList", {1}, {4}, {0, 1, 2, 3});
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyTetra", "TetrahedralGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {4}, {3}, vertices);
    WriteLegacyArray<uint64>(geometry, "SharedTetList", {1}, {4}, {0, 1, 2, 3});
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyHex", "HexahedralGeometry");
    WriteLegacyArray(geometry, "SharedVertexList", {8}, {3}, std::vector<float32>(24, 1.0F));
    WriteLegacyArray<uint64>(geometry, "SharedHexList", {1}, {8}, {0, 1, 2, 3, 4, 5, 6, 7});
  }
  {
    auto geometry = CreateLegacyGeometryGroup(containers, "LegacyRect", "RectGridGeometry");
    auto dimensions = geometry.createDataset("DIMENSIONS");
    const std::vector<int64> dims = {2, 2, 2};
    auto sizeResult3 = dimensions.writeSpan<int64>({dims.size()}, nonstd::span<const int64>(dims.data(), dims.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(sizeResult3);
    WriteLegacyArray<float32>(geometry, "xBounds", {3}, {1}, {0.0F, 1.0F, 2.0F});
    WriteLegacyArray<float32>(geometry, "yBounds", {3}, {1}, {0.0F, 1.0F, 2.0F});
    WriteLegacyArray<float32>(geometry, "zBounds", {3}, {1}, {0.0F, 1.0F, 2.0F});
  }
  return outputPath;
}

fs::path CreateMalformedLegacyArrayFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult11 = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_LegacyFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult11);
  auto containers = fileWriter.createGroup("DataContainers");
  auto container = containers.createGroup("MalformedContainer");
  auto attributeMatrix = container.createGroup("MalformedAM");
  auto writeVectorAttributeResult7 = attributeMatrix.writeVectorAttribute("TupleDimensions", ShapeType{3});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult7);
  auto assertionResult3 = attributeMatrix.writeScalarAttribute<uint32>("AttributeMatrixType", 13);
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult3);
  auto dataset = attributeMatrix.createDataset("MalformedLegacyArray");
  const std::vector<int32> values = {1, 2, 3};
  auto sizeResult4 = dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(sizeResult4);
  auto writeStringAttributeResult12 = dataset.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<int32>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult12);
  auto writeVectorAttributeResult8 = dataset.writeVectorAttribute("TupleDimensions", ShapeType{3});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult8);
  return outputPath;
}

fs::path CreateLegacyStatisticsFile(const std::string& fileName)
{
  const fs::path outputPath = GetTestOutputDir() / fileName;
  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  REQUIRE(fileWriter.isValid());
  auto writeStringAttributeResult13 = fileWriter.writeStringAttribute("FileVersion", DREAM3D::k_LegacyFileVersion.str());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult13);
  auto containers = fileWriter.createGroup("DataContainers");
  auto container = containers.createGroup("StatsContainer");
  auto attributeMatrix = container.createGroup("StatsAM");
  auto writeVectorAttributeResult9 = attributeMatrix.writeVectorAttribute("TupleDimensions", ShapeType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult9);
  auto assertionResult4 = attributeMatrix.writeScalarAttribute<uint32>("AttributeMatrixType", 13);
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult4);
  auto statistics = attributeMatrix.createGroup("StatisticsSource");
  auto writeStringAttributeResult14 = statistics.writeStringAttribute(Constants::k_ObjectTypeTag, "Statistics");
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult14);
  auto values = statistics.createDataset("Values");
  const std::vector<float32> datasetValues = {1.0F};
  auto dataResult2 = values.writeSpan<float32>({1, 1}, nonstd::span<const float32>(datasetValues.data(), datasetValues.size()));
  SIMPLNX_RESULT_REQUIRE_VALID(dataResult2);
  auto writeStringAttributeResult15 = values.writeStringAttribute(Constants::k_ObjectTypeTag, DataArray<float32>::GetTypeName());
  SIMPLNX_RESULT_REQUIRE_VALID(writeStringAttributeResult15);
  auto writeVectorAttributeResult10 = values.writeVectorAttribute("TupleDimensions", ShapeType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult10);
  auto writeVectorAttributeResult11 = values.writeVectorAttribute("ComponentDimensions", ShapeType{1});
  SIMPLNX_RESULT_REQUIRE_VALID(writeVectorAttributeResult11);
  auto assertionResult5 = values.writeVectorAttribute<float32>("Mean", {4.0F, 5.0F, 6.0F});
  SIMPLNX_RESULT_REQUIRE_VALID(assertionResult5);
  return outputPath;
}

/**
 * @brief Creates two groups with distinct integer and float arrays for pruning.
 * @return A populated multi-group DataStructure.
 */
DataStructure CreateMultiGroupTestDataStructure()
{
  DataStructure ds;

  auto* groupA = DataGroup::Create(ds, k_GroupAName);
  REQUIRE(groupA != nullptr);
  auto* attrMatA = AttributeMatrix::Create(ds, k_AttrMatAName, {k_PruneArraySize}, groupA->getId());
  REQUIRE(attrMatA != nullptr);

  auto storeA1 = std::make_unique<DataStore<int32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeA1->setValue(i, static_cast<int32>(i));
  }
  auto* arrayA1 = DataArray<int32>::Create(ds, k_ArrayA1Name, std::move(storeA1), attrMatA->getId());
  REQUIRE(arrayA1 != nullptr);

  auto storeA2 = std::make_unique<DataStore<int32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<int32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeA2->setValue(i, static_cast<int32>(i * 2));
  }
  auto* arrayA2 = DataArray<int32>::Create(ds, k_ArrayA2Name, std::move(storeA2), attrMatA->getId());
  REQUIRE(arrayA2 != nullptr);

  auto* groupB = DataGroup::Create(ds, k_GroupBName);
  REQUIRE(groupB != nullptr);
  auto* attrMatB = AttributeMatrix::Create(ds, k_AttrMatBName, {k_PruneArraySize}, groupB->getId());
  REQUIRE(attrMatB != nullptr);

  auto storeB1 = std::make_unique<DataStore<float32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeB1->setValue(i, static_cast<float32>(i) * 0.5f);
  }
  auto* arrayB1 = DataArray<float32>::Create(ds, k_ArrayB1Name, std::move(storeB1), attrMatB->getId());
  REQUIRE(arrayB1 != nullptr);

  auto storeB2 = std::make_unique<DataStore<float32>>(std::vector<usize>{k_PruneArraySize}, std::vector<usize>{1}, static_cast<float32>(0));
  for(usize i = 0; i < k_PruneArraySize; ++i)
  {
    storeB2->setValue(i, static_cast<float32>(i) * 0.25f);
  }
  auto* arrayB2 = DataArray<float32>::Create(ds, k_ArrayB2Name, std::move(storeB2), attrMatB->getId());
  REQUIRE(arrayB2 != nullptr);

  return ds;
}

/**
 * @brief Creates numeric, StringArray, and NeighborList siblings for selective import tests.
 * @return DataStructure with values that distinguish empty and nonempty nonnumeric tuples.
 */
DataStructure CreateNonnumericSelectionTestDataStructure()
{
  DataStructure ds;

  auto* group = DataGroup::Create(ds, k_NonnumericGroupName);
  REQUIRE(group != nullptr);
  auto* attrMat = AttributeMatrix::Create(ds, k_NonnumericAttrMatName, {k_NonnumericTupleCount}, group->getId());
  REQUIRE(attrMat != nullptr);

  auto numericStore = std::make_unique<DataStore<int32>>(std::vector<usize>{k_NonnumericTupleCount}, std::vector<usize>{1}, 0);
  numericStore->setValue(0, 2);
  numericStore->setValue(1, 4);
  numericStore->setValue(2, 6);
  REQUIRE(DataArray<int32>::Create(ds, k_NonnumericNumericArrayName, std::move(numericStore), attrMat->getId()) != nullptr);

  auto recoverySiblingStore = std::make_unique<DataStore<int32>>(std::vector<usize>{k_NonnumericTupleCount}, std::vector<usize>{1}, 0);
  recoverySiblingStore->setValue(0, 3);
  recoverySiblingStore->setValue(1, 6);
  recoverySiblingStore->setValue(2, 9);
  REQUIRE(DataArray<int32>::Create(ds, k_NonnumericRecoverySiblingName, std::move(recoverySiblingStore), attrMat->getId()) != nullptr);

  REQUIRE(StringArray::CreateWithValues(ds, k_NonnumericStringArrayName, {k_NonnumericTupleCount}, {"alpha", "", "omega"}, attrMat->getId()) != nullptr);

  auto* neighborList = NeighborList<int32>::Create(ds, k_NonnumericNeighborListName, {k_NonnumericTupleCount}, attrMat->getId());
  REQUIRE(neighborList != nullptr);
  neighborList->setList(0, std::vector<int32>{});
  neighborList->setList(1, std::vector<int32>{7, 11});
  neighborList->setList(2, std::vector<int32>{13});

  return ds;
}

/**
 * @brief Changes a numeric dataset's component shape while retaining its physical values.
 * @param filePath Identifies the test DREAM3D file.
 *
 * Creates a physical-count mismatch for a
 * recovery placeholder.
 * The valid sibling proves isolated recovery.
 */
void SetRecoveryPlaceholderComponentShape(const fs::path& filePath)
{
  auto fileWriter = HDF5::FileIO::AppendFile(filePath);
  REQUIRE(fileWriter.isValid());
  auto dataStructureGroup = fileWriter.openGroup("DataStructure");
  REQUIRE(dataStructureGroup.isValid());
  auto group = dataStructureGroup.openGroup(k_NonnumericGroupName.str());
  REQUIRE(group.isValid());
  auto attrMat = group.openGroup(k_NonnumericAttrMatName.str());
  REQUIRE(attrMat.isValid());
  auto numericDataset = attrMat.openDataset(k_NonnumericNumericArrayName.str());
  REQUIRE(numericDataset.getId() > 0);
  auto shapeResult = numericDataset.writeVectorAttribute<usize>(IOConstants::k_ComponentShapeTag.str(), {2});
  SIMPLNX_RESULT_REQUIRE_VALID(shapeResult);
}

/**
 * @brief Returns true when a result contains the requested warning code.
 * @tparam T Specifies the result value type.
 * @param result Provides warnings to inspect.
 * @param code Identifies the
 * warning code.
 * @return True when result contains code.
 */
template <typename T>
bool ContainsWarning(const Result<T>& result, int32 code)
{
  return std::any_of(result.warnings().cbegin(), result.warnings().cend(), [code](const Warning& warning) { return warning.code == code; });
}
} // namespace

// =============================================================================
// Test Scenarios
// =============================================================================

TEST_CASE("Dream3dLoadingApi: LoadDataStructure loads all arrays")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_LoadAll.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructure(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  // Both arrays must exist
  auto* smallArray = ds.getDataAs<Int32Array>(smallPath);
  REQUIRE(smallArray != nullptr);
  REQUIRE(smallArray->getNumberOfTuples() == k_SmallArraySize);

  auto* largeArray = ds.getDataAs<Float32Array>(largePath);
  REQUIRE(largeArray != nullptr);
  REQUIRE(largeArray->getNumberOfTuples() == k_LargeArraySize);

  // Verify SmallArray values
  const auto& smallStore = smallArray->getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }

  // Verify LargeArray values
  const auto& largeStore = largeArray->getDataStoreRef();
  for(usize i = 0; i < k_LargeArraySize; ++i)
  {
    CHECK(largeStore[i] == Approx(static_cast<float32>(i) * 1.5f));
  }
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArrays loads only requested arrays")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_Selective.dream3d");

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  Result<DataStructure> result = DREAM3D::LoadDataStructureArrays(filePath, {smallPath});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // SmallArray must exist with correct data
  auto* smallArray = ds.getDataAs<Int32Array>(smallPath);
  REQUIRE(smallArray != nullptr);
  REQUIRE(smallArray->getNumberOfTuples() == k_SmallArraySize);

  const auto& smallStore = smallArray->getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }

  // LargeArray must NOT exist
  auto* largeArray = ds.getDataAs<Float32Array>(largePath);
  CHECK(largeArray == nullptr);

  // Ancestor containers must exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_SmallAMPath) != nullptr);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArraysMetadata loads only requested metadata")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_SelectiveMeta.dream3d");

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  Result<DataStructure> result = DREAM3D::LoadDataStructureArraysMetadata(filePath, {smallPath});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // SmallArray must exist
  auto* smallArray = ds.getDataAs<IDataArray>(smallPath);
  REQUIRE(smallArray != nullptr);

  // SmallArray store must be Empty (no data loaded)
  CHECK(smallArray->getStoreType() == IDataStore::StoreType::Empty);

  // LargeArray must NOT exist
  auto* largeArray = ds.getDataAs<IDataArray>(largePath);
  CHECK(largeArray == nullptr);

  // Ancestor containers must exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_SmallAMPath) != nullptr);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureMetadata loads all metadata")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_AllMeta.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructureMetadata(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;
  const auto& largePath = k_LargeArrayPath;

  // Both arrays must exist
  auto* smallArray = ds.getDataAs<IDataArray>(smallPath);
  REQUIRE(smallArray != nullptr);
  auto* largeArray = ds.getDataAs<IDataArray>(largePath);
  REQUIRE(largeArray != nullptr);

  // Both must have Empty stores (no data loaded)
  CHECK(smallArray->getStoreType() == IDataStore::StoreType::Empty);
  CHECK(largeArray->getStoreType() == IDataStore::StoreType::Empty);
}

TEST_CASE("StorageFormatPlan: current metadata preserves an empty component shape and reloads scalar values", "[StorageFormatPlan][T17][T22][T29]")
{
  DataStructure source;
  auto store = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{}, 0);
  store->setValue(0, 11);
  store->setValue(1, 22);
  store->setValue(2, 33);
  REQUIRE(DataArray<int32>::Create(source, "ScalarComponents", store) != nullptr);

  ScopedTempFile fileGuard{WriteTestFile(source, "Dream3dLoadingApiTest_EmptyComponentShape.dream3d")};
  auto resolver = std::make_shared<RecordingResolver>();
  DataStructure::setDefaultFormatResolver(resolver);

  auto metadataResult = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  DataStructure::setDefaultFormatResolver(std::make_shared<InMemoryFormatResolver>());
  SIMPLNX_RESULT_REQUIRE_VALID(metadataResult);
  const DataPath arrayPath({"ScalarComponents"});
  const auto* metadataArray = metadataResult.value().getDataAs<Int32Array>(arrayPath);
  REQUIRE(metadataArray != nullptr);
  REQUIRE(metadataArray->getTupleShape() == ShapeType{3});
  REQUIRE(metadataArray->getComponentShape().empty());
  REQUIRE(metadataArray->getNumberOfTuples() == 3);
  REQUIRE(metadataArray->getNumberOfComponents() == 1);
  REQUIRE(metadataArray->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(resolver->paths == std::vector<DataPath>{arrayPath});
  REQUIRE(resolver->types == std::vector<DataType>{DataType::int32});
  REQUIRE(resolver->bytes == std::vector<uint64>{3 * sizeof(int32)});

  auto loadedResult = DREAM3D::LoadDataStructure(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(loadedResult);
  const auto* loadedArray = loadedResult.value().getDataAs<Int32Array>(arrayPath);
  REQUIRE(loadedArray != nullptr);
  REQUIRE(loadedArray->getTupleShape() == ShapeType{3});
  REQUIRE(loadedArray->getComponentShape().empty());
  REQUIRE(loadedArray->getSize() == 3);
  CHECK(loadedArray->getDataStoreRef().getValue(0) == 11);
  CHECK(loadedArray->getDataStoreRef().getValue(1) == 22);
  CHECK(loadedArray->getDataStoreRef().getValue(2) == 33);
}

TEST_CASE("StorageFormatPlan: current metadata rejects a missing component-shape attribute", "[StorageFormatPlan][T29]")
{
  ScopedTempFile fileGuard{CreateMalformedCurrentArrayFile("Dream3dLoadingApiTest_MissingComponentShape.dream3d")};
  auto result = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find("Component") != std::string::npos);
  CHECK(result.errors().front().message.find("MalformedArray") != std::string::npos);
}

TEST_CASE("StorageFormatPlan: current metadata rejects a nonnumeric component-shape attribute", "[StorageFormatPlan][T29]")
{
  ScopedTempFile fileGuard{CreateCorruptCurrentArrayFile("Dream3dLoadingApiTest_CorruptComponentShape.dream3d")};
  auto result = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find("ComponentDimensions") != std::string::npos);
  CHECK(result.errors().front().message.find("CorruptShapeArray") != std::string::npos);
  CHECK(result.errors().front().message.find("not numeric") != std::string::npos);
}

TEST_CASE("StorageFormatPlan: current metadata propagates a missing importable attribute", "[StorageFormatPlan][T29]")
{
  ScopedTempFile fileGuard{CreateMissingImportableCurrentArrayFile("Dream3dLoadingApiTest_MissingImportable.dream3d")};
  auto fileReader = HDF5::FileIO::ReadFile(fileGuard.path);
  REQUIRE(fileReader.isValid());
  auto dataStructureGroup = fileReader.openGroup(Constants::k_DataStructureTag);
  HDF5::DataStructureReader structureReader;
  HDF5::DataArrayIO<int32> arrayIO;
  auto result = arrayIO.readData(structureReader, dataStructureGroup, "MissingImportableArray", DataObject::IdType{1}, {}, true);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find(Constants::k_ImportableTag) != std::string::npos);
  CHECK(result.errors().front().message.find("MissingImportableArray") != std::string::npos);
}

TEST_CASE("StorageFormatPlan: eager current reader rejects overflowing shape metadata", "[StorageFormatPlan][T22][T29]")
{
  ScopedTempFile fileGuard{CreateOverflowCurrentArrayFile("Dream3dLoadingApiTest_OverflowShape.dream3d")};
  auto result = HDF5::DataStructureReader::ReadFile(fileGuard.path, false);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().code == -89201);
  CHECK(result.errors().front().message.find("OverflowArray") != std::string::npos);
  CHECK(result.errors().front().message.find("overflow") != std::string::npos);
}

TEST_CASE("StorageFormatPlan: memory materialization consumes the recorded plan", "[StorageFormatPlan][T13][T21]")
{
  DataStructure source;
  auto store = std::make_shared<DataStore<int32>>(ShapeType{3}, ShapeType{1}, 0);
  store->setValue(0, 11);
  store->setValue(1, 22);
  store->setValue(2, 33);
  REQUIRE(Int32Array::Create(source, "Values", std::move(store)) != nullptr);
  ScopedTempFile fileGuard{WriteTestFile(source, "Dream3dLoadingApiTest_RecordedMemoryPlan.dream3d")};

  DataStructure prepared;
  auto plannedResult = Int32Array::CreatePlanned(prepared, "Values", {3}, {1}, Preferences::k_InMemoryFormat.str());
  SIMPLNX_RESULT_REQUIRE_VALID(plannedResult);
  auto resolver = std::make_shared<ThrowingCountingResolver>();
  prepared.setFormatResolver(resolver);
  auto fileReader = HDF5::FileIO::ReadFile(fileGuard.path);
  REQUIRE(fileReader.isValid());

  auto materializeResult = DREAM3D::detail::MaterializeImportedPaths(prepared, fileReader, {DataPath({"Values"})});
  SIMPLNX_RESULT_REQUIRE_VALID(materializeResult);
  CHECK(resolver->calls == 0);
  REQUIRE_NOTHROW(prepared.getDataRefAs<Int32Array>(DataPath({"Values"})));
  const auto& values = prepared.getDataRefAs<Int32Array>(DataPath({"Values"}));
  REQUIRE(values.getStoreType() == IDataStore::StoreType::InMemory);
  CHECK(values.getDataStoreRef().getValue(0) == 11);
  CHECK(values.getDataStoreRef().getValue(1) == 22);
  CHECK(values.getDataStoreRef().getValue(2) == 33);
}

TEST_CASE("StorageFormatPlan: legacy geometry metadata plans and reloads connectivity and bounds", "[StorageFormatPlan][T17][T29]")
{
  ScopedTempFile fileGuard{CreateLegacyGeometryFile("Dream3dLoadingApiTest_LegacyGeometries.dream3d")};
  const std::vector<DataPath> arrayPaths = {DataPath({"LegacyVertex", "SharedVertexList"}),
                                            DataPath({"LegacyEdge", "SharedVertexList"}),
                                            DataPath({"LegacyEdge", "SharedEdgeList"}),
                                            DataPath({"LegacyTriangle", "SharedVertexList"}),
                                            DataPath({"LegacyTriangle", "SharedTriList"}),
                                            DataPath({"LegacyQuad", "SharedVertexList"}),
                                            DataPath({"LegacyQuad", "SharedQuadList"}),
                                            DataPath({"LegacyTetra", "SharedVertexList"}),
                                            DataPath({"LegacyTetra", "SharedTetList"}),
                                            DataPath({"LegacyHex", "SharedVertexList"}),
                                            DataPath({"LegacyHex", "SharedHexList"}),
                                            DataPath({"LegacyRect", "xBounds"}),
                                            DataPath({"LegacyRect", "yBounds"}),
                                            DataPath({"LegacyRect", "zBounds"})};

  auto metadataResult = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadataResult);
  for(const auto& path : arrayPaths)
  {
    INFO(path.toString());
    const auto* array = metadataResult.value().getDataAs<IDataArray>(path);
    REQUIRE(array != nullptr);
    REQUIRE(array->getStoreType() == IDataStore::StoreType::Empty);
  }

  auto loadedResult = DREAM3D::LoadDataStructure(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(loadedResult);
  for(const auto& path : arrayPaths)
  {
    INFO(path.toString());
    const auto* array = loadedResult.value().getDataAs<IDataArray>(path);
    REQUIRE(array != nullptr);
    REQUIRE(array->getStoreType() == IDataStore::StoreType::InMemory);
  }
  const auto* loadedVertices = loadedResult.value().getDataAs<Float32Array>(DataPath({"LegacyVertex", "SharedVertexList"}));
  REQUIRE(loadedVertices != nullptr);
  for(usize index = 0; index < 12; ++index)
  {
    CHECK(loadedVertices->getDataStoreRef().getValue(index) == static_cast<float32>(index));
  }
  const auto* loadedEdges = loadedResult.value().getDataAs<UInt64Array>(DataPath({"LegacyEdge", "SharedEdgeList"}));
  REQUIRE(loadedEdges != nullptr);
  CHECK(loadedEdges->getDataStoreRef().getValue(3) == 3);
  const auto* xBounds = loadedResult.value().getDataAs<Float32Array>(DataPath({"LegacyRect", "xBounds"}));
  REQUIRE(xBounds != nullptr);
  CHECK(xBounds->getDataStoreRef().getValue(2) == 2.0F);
}

TEST_CASE("StorageFormatPlan: selected execute import plans against complete destination context", "[StorageFormatPlan][T15][T25][T26]")
{
  DataStructure source = CreateMultiGroupTestDataStructure();
  ScopedTempFile fileGuard{WriteTestFile(source, "Dream3dLoadingApiTest_SelectedDestinationContext.dream3d")};

  DataStructure destination;
  REQUIRE(DataGroup::Create(destination, "ExistingMarker") != nullptr);
  auto resolver = std::make_shared<RecordingResolver>();
  resolver->requiredContextPaths = {DataPath({"ExistingMarker"}), k_GroupAPath, k_AttrMatAPath};
  destination.setFormatResolver(resolver);

  ImportH5ObjectPathsAction action(fileGuard.path, {k_GroupAPath, k_AttrMatAPath, k_ArrayA1Path});
  auto result = action.apply(destination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  REQUIRE(resolver->contextWasComplete);
  REQUIRE(resolver->paths == std::vector<DataPath>{k_ArrayA1Path});
  REQUIRE(resolver->types == std::vector<DataType>{DataType::int32});
  REQUIRE(resolver->bytes == std::vector<uint64>{k_PruneArraySize * sizeof(int32)});
  REQUIRE(destination.containsData(DataPath({"ExistingMarker"})));
  REQUIRE(destination.containsData(k_ArrayA1Path));
  REQUIRE_FALSE(destination.containsData(k_ArrayA2Path));
  REQUIRE_FALSE(destination.containsData(k_GroupBPath));
  const auto& array = destination.getDataRefAs<Int32Array>(k_ArrayA1Path);
  for(usize index = 0; index < k_PruneArraySize; ++index)
  {
    CHECK(array.getDataStoreRef().getValue(index) == static_cast<int32>(index));
  }
}

TEST_CASE("StorageFormatPlan: incremental import finalizes exact leaves only", "[StorageFormatPlan][T26]")
{
  DataStructure source = CreateMultiGroupTestDataStructure();
  ScopedTempFile fileGuard{WriteTestFile(source, "Dream3dLoadingApiTest_ExactIncrementalLeaves.dream3d")};
  auto metadataResult = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(metadataResult);
  auto fileReader = HDF5::FileIO::ReadFile(fileGuard.path);
  REQUIRE(fileReader.isValid());

  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  ScopedRecordingImportFinalizer finalizerGuard(collection);
  DataStructure destination;

  REQUIRE(DREAM3D::FinishImportingObject(metadataResult.value(), destination, k_GroupAPath, fileReader, false).valid());
  REQUIRE(DREAM3D::FinishImportingObject(metadataResult.value(), destination, k_AttrMatAPath, fileReader, false).valid());
  CHECK(finalizerGuard.recorder->calls.empty());

  REQUIRE(DREAM3D::FinishImportingObject(metadataResult.value(), destination, k_ArrayA1Path, fileReader, false).valid());
  REQUIRE(finalizerGuard.recorder->calls.size() == 1);
  CHECK(finalizerGuard.recorder->calls.front() == std::vector<DataPath>{k_ArrayA1Path});
  REQUIRE(destination.containsData(k_ArrayA1Path));
  REQUIRE_FALSE(destination.containsData(k_ArrayA2Path));
}

TEST_CASE("StorageFormatPlan: selected import planning failure leaves destination unchanged", "[StorageFormatPlan][T26]")
{
  DataStructure source = CreateSimpleTestDataStructure();
  ScopedTempFile fileGuard{WriteTestFile(source, "Dream3dLoadingApiTest_SelectedPlanningFailure.dream3d")};

  DataStructure destination;
  REQUIRE(DataGroup::Create(destination, "ExistingMarker") != nullptr);
  auto resolver = std::make_shared<RecordingResolver>();
  resolver->failureMessage = "injected destination planning failure";
  destination.setFormatResolver(resolver);
  const auto originalPaths = destination.getAllDataPaths();
  const auto originalNextId = destination.getNextId();

  ImportH5ObjectPathsAction action(fileGuard.path, {k_GroupPath, k_SmallAMPath, k_SmallArrayPath});
  auto result = action.apply(destination, IDataAction::Mode::Preflight);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find(k_SmallArrayPath.toString()) != std::string::npos);
  CHECK(result.errors().front().message.find(resolver->failureMessage) != std::string::npos);
  CHECK(destination.getAllDataPaths() == originalPaths);
  CHECK(destination.getNextId() == originalNextId);
}

TEST_CASE("StorageFormatPlan: selected geometry import preserves partial selection", "[StorageFormatPlan][T26]")
{
  ScopedTempFile fileGuard{CreateLegacyGeometryFile("Dream3dLoadingApiTest_PartialLegacyGeometry.dream3d")};
  const DataPath geometryPath({"LegacyEdge"});
  const DataPath verticesPath({"LegacyEdge", "SharedVertexList"});
  const DataPath edgesPath({"LegacyEdge", "SharedEdgeList"});

  DataStructure destination;
  ImportH5ObjectPathsAction action(fileGuard.path, {geometryPath, verticesPath});
  auto actionApplyResult = action.apply(destination, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(actionApplyResult);
  REQUIRE(destination.containsData(geometryPath));
  REQUIRE(destination.containsData(verticesPath));
  REQUIRE_FALSE(destination.containsData(edgesPath));
  REQUIRE(destination.getDataAs<IDataArray>(verticesPath)->getStoreType() == IDataStore::StoreType::Empty);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructure with invalid path returns error")
{
  const fs::path bogusPath("/tmp/nonexistent_dream3d_file_12345.dream3d");

  // Suppress HDF5 error output temporarily
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);
  Result<DataStructure> result = DREAM3D::LoadDataStructure(bogusPath);
  // Restore default error handling
  H5Eset_auto(H5E_DEFAULT, (H5E_auto_t)H5Eprint, stderr);

  REQUIRE(result.invalid());
  REQUIRE(!result.errors().empty());
  CHECK(result.errors()[0].code == -1);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructure with legacy file")
{
  const fs::path legacyPath = fs::path(unit_test::k_SourceDir.view()) / "test" / "Data" / "LegacyData.dream3d";
  REQUIRE(fs::exists(legacyPath));

  Result<DataStructure> result = DREAM3D::LoadDataStructure(legacyPath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  // Verify the DataStructure has content (legacy files should load successfully)
  const DataStructure& ds = result.value();
  CHECK(!ds.getAllDataPaths().empty());
}

TEST_CASE("StorageFormatPlan: legacy dataset metadata receives a validated plan", "[StorageFormatPlan][T17][T29]")
{
  const fs::path legacyPath = fs::path(unit_test::k_SourceDir.view()) / "test" / "Data" / "LegacyData.dream3d";
  REQUIRE(fs::exists(legacyPath));
  auto resolver = std::make_shared<RecordingResolver>();
  DataStructure::setDefaultFormatResolver(resolver);

  auto result = DREAM3D::LoadDataStructureMetadata(legacyPath);
  DataStructure::setDefaultFormatResolver(std::make_shared<InMemoryFormatResolver>());
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataPath arrayPath({"DataContainer", "AttributeMatrix", "Float32"});
  const auto* array = result.value().getDataAs<Float32Array>(arrayPath);
  REQUIRE(array != nullptr);
  REQUIRE(array->getStoreType() == IDataStore::StoreType::Empty);
  REQUIRE(array->getTupleShape() == ShapeType{1});
  REQUIRE(array->getComponentShape() == ShapeType{1});
  const auto pathIter = std::find(resolver->paths.begin(), resolver->paths.end(), arrayPath);
  REQUIRE(pathIter != resolver->paths.end());
  const usize index = static_cast<usize>(std::distance(resolver->paths.begin(), pathIter));
  REQUIRE(resolver->types[index] == DataType::float32);
  REQUIRE(resolver->bytes[index] == sizeof(float32));
}

TEST_CASE("StorageFormatPlan: malformed legacy shape metadata preserves its attribute error", "[StorageFormatPlan][T29]")
{
  ScopedTempFile fileGuard{CreateMalformedLegacyArrayFile("Dream3dLoadingApiTest_MalformedLegacyShape.dream3d")};
  auto result = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  REQUIRE(result.invalid());
  REQUIRE_FALSE(result.errors().empty());
  CHECK(result.errors().front().message.find("ComponentDimensions") != std::string::npos);
  CHECK(result.errors().front().message.find("MalformedLegacyArray") != std::string::npos);
}

TEST_CASE("StorageFormatPlan: legacy Statistics attributes remain eagerly materialized", "[StorageFormatPlan][T17]")
{
  ScopedTempFile fileGuard{CreateLegacyStatisticsFile("Dream3dLoadingApiTest_LegacyStatistics.dream3d")};
  auto result = DREAM3D::LoadDataStructureMetadata(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataPath attributePath({"StatsContainer", "Statistics", "Values_Mean"});
  const auto* attributeArray = result.value().getDataAs<Float32Array>(attributePath);
  REQUIRE(attributeArray != nullptr);
  REQUIRE(attributeArray->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(attributeArray->getTupleShape() == ShapeType{3});
  REQUIRE(attributeArray->getComponentShape() == ShapeType{1});
  CHECK(attributeArray->getDataStoreRef().getValue(0) == 4.0F);
  CHECK(attributeArray->getDataStoreRef().getValue(1) == 5.0F);
  CHECK(attributeArray->getDataStoreRef().getValue(2) == 6.0F);
}

TEST_CASE("StorageFormatPlan: selected legacy Statistics values use an independent destination store", "[StorageFormatPlan][T17][T25][T26]")
{
  ScopedTempFile fileGuard{CreateLegacyStatisticsFile("Dream3dLoadingApiTest_SelectedLegacyStatistics.dream3d")};
  auto& cache = DREAM3D::Dream3dPreflightCache::Instance();
  cache.clear();
  auto cleanup = MakeScopeGuard([&cache]() noexcept { cache.clear(); });

  const DataPath containerPath({"StatsContainer"});
  const DataPath statisticsPath({"StatsContainer", "Statistics"});
  const DataPath attributePath({"StatsContainer", "Statistics", "Values_Mean"});
  auto sourceResult = cache.fetch(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(sourceResult);
  const auto* sourceArray = sourceResult.value().getDataAs<Float32Array>(attributePath);
  REQUIRE(sourceArray != nullptr);
  REQUIRE(sourceArray->getStoreType() == IDataStore::StoreType::InMemory);

  DataStructure destination;
  auto resolver = std::make_shared<RecordingResolver>();
  destination.setFormatResolver(resolver);
  ImportH5ObjectPathsAction action(fileGuard.path, {containerPath, statisticsPath, attributePath});
  auto importResult = action.apply(destination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(importResult);

  auto* destinationArray = destination.getDataAs<Float32Array>(attributePath);
  REQUIRE(destinationArray != nullptr);
  REQUIRE(destinationArray->getStoreType() == IDataStore::StoreType::InMemory);
  REQUIRE(destinationArray->getIDataStore() != sourceArray->getIDataStore());
  REQUIRE(resolver->paths == std::vector<DataPath>{attributePath});
  REQUIRE(resolver->types == std::vector<DataType>{DataType::float32});
  REQUIRE(resolver->bytes == std::vector<uint64>{3 * sizeof(float32)});
  CHECK(destinationArray->getDataStoreRef().getValue(0) == 4.0F);
  CHECK(destinationArray->getDataStoreRef().getValue(1) == 5.0F);
  CHECK(destinationArray->getDataStoreRef().getValue(2) == 6.0F);

  destinationArray->getDataStoreRef().setValue(0, 99.0F);
  auto nextSourceResult = cache.fetch(fileGuard.path);
  SIMPLNX_RESULT_REQUIRE_VALID(nextSourceResult);
  const auto* nextSourceArray = nextSourceResult.value().getDataAs<Float32Array>(attributePath);
  REQUIRE(nextSourceArray != nullptr);
  REQUIRE(nextSourceArray->getIDataStore() != destinationArray->getIDataStore());
  CHECK(nextSourceArray->getDataStoreRef().getValue(0) == 4.0F);
}

TEST_CASE("Dream3dLoadingApi: LoadDataStructureArrays prune verification")
{
  DataStructure srcDs = CreateMultiGroupTestDataStructure();
  fs::path filePath = WriteTestFile(srcDs, "Dream3dLoadingApiTest_Prune.dream3d");

  Result<DataStructure> result = DREAM3D::LoadDataStructureArrays(filePath, {k_ArrayA1Path});
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  // ArrayA1 and its ancestors must exist
  auto* arrayA1 = ds.getDataAs<Int32Array>(k_ArrayA1Path);
  REQUIRE(arrayA1 != nullptr);
  REQUIRE(arrayA1->getNumberOfTuples() == k_PruneArraySize);
  CHECK(ds.getDataAs<DataGroup>(k_GroupAPath) != nullptr);
  CHECK(ds.getDataAs<AttributeMatrix>(k_AttrMatAPath) != nullptr);

  // ArrayA2 must NOT exist (same group, but not requested)
  CHECK(ds.getDataAs<Int32Array>(k_ArrayA2Path) == nullptr);

  // GroupB and its children must NOT exist
  CHECK(ds.getDataAs<DataGroup>(k_GroupBPath) == nullptr);
  CHECK(ds.getDataAs<Float32Array>(k_ArrayB1Path) == nullptr);
  CHECK(ds.getDataAs<Float32Array>(k_ArrayB2Path) == nullptr);
}

TEST_CASE("Dream3dLoadingApi: selected StringArray preserves metadata and values", "[simplnx][Dream3dLoadingApi][SelectiveImport]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_SelectedStrings.dream3d");
  ScopedTempFile fileGuard(filePath);

  auto metadataResult = DREAM3D::LoadDataStructureArraysMetadata(filePath, {k_NonnumericStringArrayPath});
  SIMPLNX_RESULT_REQUIRE_VALID(metadataResult);
  const DataStructure& metadata = metadataResult.value();
  const StringArray* metadataStringsPtr = nullptr;
  REQUIRE_NOTHROW(metadataStringsPtr = &metadata.getDataRefAs<StringArray>(k_NonnumericStringArrayPath));
  REQUIRE(metadataStringsPtr != nullptr);
  CHECK(metadataStringsPtr->isPlaceholder());
  REQUIRE_THROWS_AS(metadataStringsPtr->at(0), std::runtime_error);
  CHECK(metadata.containsData(k_NonnumericGroupPath));
  CHECK(metadata.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericNeighborListPath));

  auto loadResult = DREAM3D::LoadDataStructureArrays(filePath, {k_NonnumericStringArrayPath});
  SIMPLNX_RESULT_REQUIRE_VALID(loadResult);
  DataStructure loaded = std::move(loadResult.value());
  StringArray* loadedStringsPtr = nullptr;
  REQUIRE_NOTHROW(loadedStringsPtr = &loaded.getDataRefAs<StringArray>(k_NonnumericStringArrayPath));
  REQUIRE(loadedStringsPtr != nullptr);
  CHECK_FALSE(loadedStringsPtr->isPlaceholder());
  CHECK(loadedStringsPtr->values() == std::vector<std::string>{"alpha", "", "omega"});
  CHECK(loaded.containsData(k_NonnumericGroupPath));
  CHECK(loaded.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericNeighborListPath));
  UnitTest::CheckArraysInheritTupleDims(loaded);

  const ImportH5ObjectPathsAction action(filePath, {k_NonnumericGroupPath, k_NonnumericAttrMatPath, k_NonnumericStringArrayPath});
  DataStructure preflightDestination;
  auto preflightResult = action.apply(preflightDestination, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  StringArray* preflightStringsPtr = nullptr;
  REQUIRE_NOTHROW(preflightStringsPtr = &preflightDestination.getDataRefAs<StringArray>(k_NonnumericStringArrayPath));
  REQUIRE(preflightStringsPtr != nullptr);
  CHECK(preflightStringsPtr->isPlaceholder());
  REQUIRE_THROWS_AS(preflightStringsPtr->at(0), std::runtime_error);
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericNeighborListPath));
  UnitTest::CheckArraysInheritTupleDims(preflightDestination);

  DataStructure executeDestination;
  auto executeResult = action.apply(executeDestination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  StringArray* executeStringsPtr = nullptr;
  REQUIRE_NOTHROW(executeStringsPtr = &executeDestination.getDataRefAs<StringArray>(k_NonnumericStringArrayPath));
  REQUIRE(executeStringsPtr != nullptr);
  CHECK_FALSE(executeStringsPtr->isPlaceholder());
  CHECK(executeStringsPtr->values() == std::vector<std::string>{"alpha", "", "omega"});
  CHECK(executeDestination.containsData(k_NonnumericGroupPath));
  CHECK(executeDestination.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericNeighborListPath));
  UnitTest::CheckArraysInheritTupleDims(executeDestination);
}

TEST_CASE("Dream3dLoadingApi: selected NeighborList preserves metadata and values", "[simplnx][Dream3dLoadingApi][SelectiveImport]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_SelectedNeighborLists.dream3d");
  ScopedTempFile fileGuard(filePath);

  auto metadataResult = DREAM3D::LoadDataStructureArraysMetadata(filePath, {k_NonnumericNeighborListPath});
  SIMPLNX_RESULT_REQUIRE_VALID(metadataResult);
  const DataStructure& metadata = metadataResult.value();
  const NeighborList<int32>* metadataNeighborListPtr = nullptr;
  REQUIRE_NOTHROW(metadataNeighborListPtr = &metadata.getDataRefAs<NeighborList<int32>>(k_NonnumericNeighborListPath));
  REQUIRE(metadataNeighborListPtr != nullptr);
  CHECK(dynamic_cast<const EmptyListStore<int32>*>(metadataNeighborListPtr->getIListStore()) != nullptr);
  CHECK(metadata.containsData(k_NonnumericGroupPath));
  CHECK(metadata.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(metadata.containsData(k_NonnumericStringArrayPath));

  auto loadResult = DREAM3D::LoadDataStructureArrays(filePath, {k_NonnumericNeighborListPath});
  SIMPLNX_RESULT_REQUIRE_VALID(loadResult);
  DataStructure loaded = std::move(loadResult.value());
  NeighborList<int32>* loadedNeighborListPtr = nullptr;
  REQUIRE_NOTHROW(loadedNeighborListPtr = &loaded.getDataRefAs<NeighborList<int32>>(k_NonnumericNeighborListPath));
  REQUIRE(loadedNeighborListPtr != nullptr);
  CHECK(loadedNeighborListPtr->getList(0) == std::vector<int32>{});
  CHECK(loadedNeighborListPtr->getList(1) == std::vector<int32>{7, 11});
  CHECK(loadedNeighborListPtr->getList(2) == std::vector<int32>{13});
  CHECK(loaded.containsData(k_NonnumericGroupPath));
  CHECK(loaded.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(loaded.containsData(k_NonnumericStringArrayPath));
  UnitTest::CheckArraysInheritTupleDims(loaded);

  const ImportH5ObjectPathsAction action(filePath, {k_NonnumericGroupPath, k_NonnumericAttrMatPath, k_NonnumericNeighborListPath});
  DataStructure preflightDestination;
  auto preflightResult = action.apply(preflightDestination, IDataAction::Mode::Preflight);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult);
  NeighborList<int32>* preflightNeighborListPtr = nullptr;
  REQUIRE_NOTHROW(preflightNeighborListPtr = &preflightDestination.getDataRefAs<NeighborList<int32>>(k_NonnumericNeighborListPath));
  REQUIRE(preflightNeighborListPtr != nullptr);
  CHECK(dynamic_cast<const EmptyListStore<int32>*>(preflightNeighborListPtr->getIListStore()) != nullptr);
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(preflightDestination.containsData(k_NonnumericStringArrayPath));
  UnitTest::CheckArraysInheritTupleDims(preflightDestination);

  DataStructure executeDestination;
  auto executeResult = action.apply(executeDestination, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult);
  NeighborList<int32>* executeNeighborListPtr = nullptr;
  REQUIRE_NOTHROW(executeNeighborListPtr = &executeDestination.getDataRefAs<NeighborList<int32>>(k_NonnumericNeighborListPath));
  REQUIRE(executeNeighborListPtr != nullptr);
  CHECK(executeNeighborListPtr->getList(0) == std::vector<int32>{});
  CHECK(executeNeighborListPtr->getList(1) == std::vector<int32>{7, 11});
  CHECK(executeNeighborListPtr->getList(2) == std::vector<int32>{13});
  CHECK(executeDestination.containsData(k_NonnumericGroupPath));
  CHECK(executeDestination.containsData(k_NonnumericAttrMatPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericNumericArrayPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericRecoverySiblingPath));
  CHECK_FALSE(executeDestination.containsData(k_NonnumericStringArrayPath));
  UnitTest::CheckArraysInheritTupleDims(executeDestination);
}

TEST_CASE("ImportH5ObjectPathsAction rejects a selected child without a parent", "[simplnx][Dream3dLoadingApi][SelectiveImport]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_ActionMissingParent.dream3d");
  ScopedTempFile fileGuard(filePath);

  const ImportH5ObjectPathsAction action(filePath, {k_NonnumericStringArrayPath});
  DataStructure destination;
  auto importResult = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(importResult.invalid());
  REQUIRE_FALSE(importResult.errors().empty());
  CHECK(importResult.errors()[0].code == -6202);
  CHECK_FALSE(destination.containsData(k_NonnumericStringArrayPath));
}

TEST_CASE("Dream3dLoadingApi: in-memory recovery placeholder warning skips only the malformed array", "[simplnx][Dream3dLoadingApi][Recovery]")
{
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_InMemoryRecoveryPlaceholder.dream3d");
  ScopedTempFile fileGuard(filePath);
  SetRecoveryPlaceholderComponentShape(filePath);

  auto fileReader = HDF5::FileIO::ReadFile(filePath);
  REQUIRE(fileReader.isValid());
  auto dataStructureGroup = fileReader.openGroup("DataStructure");
  REQUIRE(dataStructureGroup.isValid());
  auto group = dataStructureGroup.openGroup(k_NonnumericGroupName.str());
  REQUIRE(group.isValid());
  auto attrMat = group.openGroup(k_NonnumericAttrMatName.str());
  REQUIRE(attrMat.isValid());

  HDF5::DataStructureReader fullReader;
  const auto fullReaderResult = fullReader.readGroup(dataStructureGroup);
  REQUIRE(fullReaderResult.valid());
  REQUIRE(ContainsWarning(fullReaderResult, -89200));
  CHECK_FALSE(fullReaderResult.value().containsData(k_NonnumericNumericArrayPath));
  CHECK(fullReader.getDataStructure().containsData(k_NonnumericRecoverySiblingPath));

  // The local manager bypasses global optional I/O registrations. This keeps
  // the test on the same core in-memory import path in both build variants.
  HDF5::DataStructureReader coreReader(new HDF5::DataIOManager());
  const auto malformedResult = coreReader.readObjectFromGroup(attrMat, k_NonnumericNumericArrayName.str());
  REQUIRE(malformedResult.valid());
  REQUIRE(ContainsWarning(malformedResult, -89200));
  CHECK_FALSE(coreReader.getDataStructure().containsData(DataPath({k_NonnumericNumericArrayName})));

  const auto siblingResult = coreReader.readObjectFromGroup(attrMat, k_NonnumericRecoverySiblingName.str());
  REQUIRE(siblingResult.valid());
  CHECK(siblingResult.warnings().empty());
  Int32Array* siblingArrayPtr = nullptr;
  const DataPath siblingPath({k_NonnumericRecoverySiblingName});
  REQUIRE_NOTHROW(siblingArrayPtr = &coreReader.getDataStructure().getDataRefAs<Int32Array>(siblingPath));
  REQUIRE(siblingArrayPtr != nullptr);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(0) == 3);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(1) == 6);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(2) == 9);
}

TEST_CASE("Dream3dLoadingApi: public full load preserves an in-memory recovery warning", "[simplnx][Dream3dLoadingApi][Recovery]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_PublicFullRecoveryWarning.dream3d");
  ScopedTempFile fileGuard(filePath);
  SetRecoveryPlaceholderComponentShape(filePath);

  auto& collection = application->getIOCollection();
  ScopedCoreImportManagers coreManagersGuard(collection);
  auto loadResult = DREAM3D::LoadDataStructure(filePath);
  REQUIRE(loadResult.valid());
  REQUIRE(ContainsWarning(loadResult, -89200));

  Int32Array* malformedArrayPtr = nullptr;
  REQUIRE_NOTHROW(malformedArrayPtr = &loadResult.value().getDataRefAs<Int32Array>(k_NonnumericNumericArrayPath));
  REQUIRE(malformedArrayPtr != nullptr);
  CHECK(malformedArrayPtr->getStoreType() == IDataStore::StoreType::Empty);

  Int32Array* siblingArrayPtr = nullptr;
  REQUIRE_NOTHROW(siblingArrayPtr = &loadResult.value().getDataRefAs<Int32Array>(k_NonnumericRecoverySiblingPath));
  REQUIRE(siblingArrayPtr != nullptr);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(0) == 3);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(1) == 6);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(2) == 9);
}

TEST_CASE("Dream3dLoadingApi: public selected load preserves an in-memory recovery warning", "[simplnx][Dream3dLoadingApi][Recovery]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_PublicSelectedRecoveryWarning.dream3d");
  ScopedTempFile fileGuard(filePath);
  SetRecoveryPlaceholderComponentShape(filePath);

  auto& collection = application->getIOCollection();
  ScopedCoreImportManagers coreManagersGuard(collection);
  auto loadResult = DREAM3D::LoadDataStructureArrays(filePath, {k_NonnumericNumericArrayPath, k_NonnumericRecoverySiblingPath});
  REQUIRE(loadResult.valid());
  REQUIRE(ContainsWarning(loadResult, -89200));
  CHECK(loadResult.value().containsData(k_NonnumericGroupPath));
  CHECK(loadResult.value().containsData(k_NonnumericAttrMatPath));

  Int32Array* malformedArrayPtr = nullptr;
  REQUIRE_NOTHROW(malformedArrayPtr = &loadResult.value().getDataRefAs<Int32Array>(k_NonnumericNumericArrayPath));
  REQUIRE(malformedArrayPtr != nullptr);
  CHECK(malformedArrayPtr->getStoreType() == IDataStore::StoreType::Empty);

  Int32Array* siblingArrayPtr = nullptr;
  REQUIRE_NOTHROW(siblingArrayPtr = &loadResult.value().getDataRefAs<Int32Array>(k_NonnumericRecoverySiblingPath));
  REQUIRE(siblingArrayPtr != nullptr);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(0) == 3);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(1) == 6);
  CHECK(siblingArrayPtr->getDataStoreRef().getValue(2) == 9);
}

TEST_CASE("Dream3dLoadingApi: public load preserves finalizer and core recovery warnings", "[simplnx][Dream3dLoadingApi][Recovery]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_FinalizerAndSweepWarnings.dream3d");
  ScopedTempFile fileGuard(filePath);
  SetRecoveryPlaceholderComponentShape(filePath);

  auto& collection = application->getIOCollection();
  ScopedCoreImportManagers coreManagersGuard(collection);
  ScopedWarningImportFinalizerManager finalizerManagerGuard(collection);
  auto loadResult = DREAM3D::LoadDataStructure(filePath);
  REQUIRE(loadResult.valid());
  REQUIRE(ContainsWarning(loadResult, -89201));
  REQUIRE(ContainsWarning(loadResult, -89200));
}

TEST_CASE("ImportH5ObjectPathsAction preserves recovery warnings through merging", "[simplnx][Dream3dLoadingApi][Recovery]")
{
  const auto application = Application::GetOrCreateInstance();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const int mergeCase = GENERATE(0, 1, 2);
  const auto source = CreateNonnumericSelectionTestDataStructure();
  const auto fileName = std::string("Dream3dLoadingApiTest_ActionWarning_") + std::to_string(mergeCase) + ".dream3d";
  const auto filePath = WriteTestFile(source, fileName);
  ScopedTempFile fileGuard(filePath);
  SetRecoveryPlaceholderComponentShape(filePath);
  ScopedCoreImportManagers coreManagersGuard(application->getIOCollection());

  DataStructure destination;
  if(mergeCase == 1)
  {
    REQUIRE(DataGroup::Create(destination, k_NonnumericGroupName) != nullptr);
  }
  const std::vector<DataPath> paths = mergeCase == 2 ? std::vector<DataPath>{k_NonnumericNumericArrayPath} :
                                                       std::vector<DataPath>{k_NonnumericGroupPath, k_NonnumericAttrMatPath, k_NonnumericNumericArrayPath, k_NonnumericRecoverySiblingPath};
  const ImportH5ObjectPathsAction action(filePath, paths);
  const auto result = action.apply(destination, IDataAction::Mode::Execute);
  REQUIRE(ContainsWarning(result, -89200));
  if(mergeCase != 0)
  {
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == (mergeCase == 1 ? -6203 : -6202));
  }
  else
  {
    REQUIRE(result.valid());
    REQUIRE_NOTHROW(destination.getDataRefAs<Int32Array>(k_NonnumericRecoverySiblingPath));
    const auto& sibling = destination.getDataRefAs<Int32Array>(k_NonnumericRecoverySiblingPath);
    CHECK(sibling.getValue(0) == 3);
    CHECK(sibling.getValue(1) == 6);
    CHECK(sibling.getValue(2) == 9);
    REQUIRE_NOTHROW(destination.getDataRefAs<Int32Array>(k_NonnumericNumericArrayPath));
    CHECK(destination.getDataRefAs<Int32Array>(k_NonnumericNumericArrayPath).getStoreType() == IDataStore::StoreType::Empty);
  }
}

TEST_CASE("ImportH5ObjectPathsAction avoids excluded materialization", "[simplnx][Dream3dLoadingApi][SelectiveImport]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  auto source = CreateSimpleTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_ActionSelection.dream3d");
  ScopedTempFile fileGuard(filePath);
  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  const auto originalManager = collection.getManager("HDF5");
  {
    ScopedImportSelectionManager managerGuard(collection);
    g_ExcludedMaterializations = 0;
    auto fullLoad = DREAM3D::LoadDataStructure(filePath);
    SIMPLNX_RESULT_REQUIRE_VALID(fullLoad);
    REQUIRE(g_ExcludedMaterializations.load() > 0);
    g_ExcludedMaterializations = 0;

    const ImportH5ObjectPathsAction action(filePath, {k_GroupPath, k_SmallAMPath, k_SmallArrayPath});
    DataStructure destination;
    auto importResult = action.apply(destination, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(importResult);
    CHECK(g_ExcludedMaterializations.load() == 0);
    REQUIRE(destination.containsData(k_SmallArrayPath));
    CHECK_FALSE(destination.containsData(k_LargeArrayPath));
    CHECK_FALSE(destination.containsData(k_LargeAMPath));
    Int32Array* selectedArrayPtr = nullptr;
    REQUIRE_NOTHROW(selectedArrayPtr = &destination.getDataRefAs<Int32Array>(k_SmallArrayPath));
    REQUIRE(selectedArrayPtr != nullptr);
    for(usize valueIndex = 0; valueIndex < k_SmallArraySize; ++valueIndex)
    {
      CHECK(selectedArrayPtr->getDataStoreRef().getValue(valueIndex) == static_cast<int32>(valueIndex * 3));
    }
    UnitTest::CheckArraysInheritTupleDims(destination);
  }
  CHECK(collection.getManager("HDF5") == originalManager);
}

TEST_CASE("ImportH5ObjectPathsAction selection semantics", "[simplnx][Dream3dLoadingApi][SelectiveImport]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  auto source = CreateSimpleTestDataStructure();
  const auto filePath = WriteTestFile(source, "Dream3dLoadingApiTest_ActionSelectionSemantics.dream3d");
  ScopedTempFile fileGuard(filePath);
  auto& collection = Application::GetOrCreateInstance()->getIOCollection();
  const auto originalManager = collection.getManager("HDF5");
  {
    ScopedImportSelectionManager managerGuard(collection);
    g_ExcludedMaterializations = 0;

    SECTION("Preflight")
    {
      const ImportH5ObjectPathsAction action(filePath, {k_GroupPath, k_SmallAMPath, k_SmallArrayPath});
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Preflight);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      CHECK(g_ExcludedMaterializations.load() == 0);
      REQUIRE(destination.containsData(k_SmallArrayPath));
      IDataArray* selectedArrayPtr = nullptr;
      REQUIRE_NOTHROW(selectedArrayPtr = &destination.getDataRefAs<IDataArray>(k_SmallArrayPath));
      REQUIRE(selectedArrayPtr != nullptr);
      CHECK(selectedArrayPtr->getStoreType() == IDataStore::StoreType::Empty);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Group shell")
    {
      const ImportH5ObjectPathsAction action(filePath, {k_GroupPath});
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      CHECK(g_ExcludedMaterializations.load() == 0);
      REQUIRE(destination.containsData(k_GroupPath));
      CHECK_FALSE(destination.containsData(k_SmallAMPath));
      CHECK_FALSE(destination.containsData(k_LargeAMPath));
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Missing path")
    {
      const DataPath missingPath({"Missing"});
      const ImportH5ObjectPathsAction action(filePath, {k_GroupPath, k_SmallAMPath, k_SmallArrayPath, missingPath});
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      CHECK(g_ExcludedMaterializations.load() == 0);
      REQUIRE(destination.containsData(k_SmallArrayPath));
      CHECK_FALSE(destination.containsData(missingPath));
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Empty selection")
    {
      const ImportH5ObjectPathsAction action(filePath, {});
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      CHECK(destination.getAllDataPaths().empty());
      CHECK(g_ExcludedMaterializations.load() == 0);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Existing target")
    {
      DataStructure destination;
      REQUIRE(DataGroup::Create(destination, k_GroupName) != nullptr);
      const ImportH5ObjectPathsAction action(filePath, {k_GroupPath, k_SmallAMPath, k_SmallArrayPath});
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      REQUIRE(importResult.invalid());
      REQUIRE_FALSE(importResult.errors().empty());
      CHECK(importResult.errors()[0].code == -6203);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Duplicate selection")
    {
      const ImportH5ObjectPathsAction action(filePath, {k_GroupPath, k_GroupPath});
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      REQUIRE(importResult.invalid());
      REQUIRE_FALSE(importResult.errors().empty());
      CHECK(importResult.errors()[0].code == -6203);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Full selection")
    {
      const ImportH5ObjectPathsAction action(filePath, source.getAllDataPaths());
      DataStructure destination;
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      REQUIRE(destination.containsData(k_SmallArrayPath));
      REQUIRE(destination.containsData(k_LargeArrayPath));

      Int32Array* smallArrayPtr = nullptr;
      REQUIRE_NOTHROW(smallArrayPtr = &destination.getDataRefAs<Int32Array>(k_SmallArrayPath));
      REQUIRE(smallArrayPtr != nullptr);
      for(usize valueIndex = 0; valueIndex < k_SmallArraySize; ++valueIndex)
      {
        CHECK(smallArrayPtr->getDataStoreRef().getValue(valueIndex) == static_cast<int32>(valueIndex * 3));
      }

      Float32Array* largeArrayPtr = nullptr;
      REQUIRE_NOTHROW(largeArrayPtr = &destination.getDataRefAs<Float32Array>(k_LargeArrayPath));
      REQUIRE(largeArrayPtr != nullptr);
      for(usize valueIndex = 0; valueIndex < k_LargeArraySize; ++valueIndex)
      {
        CHECK(largeArrayPtr->getDataStoreRef().getValue(valueIndex) == Approx(static_cast<float32>(valueIndex) * 1.5f));
      }
      UnitTest::CheckArraysInheritTupleDims(destination);
    }

    SECTION("Parent supplied by destination")
    {
      DataStructure destination;
      auto* destinationGroup = DataGroup::Create(destination, k_GroupName);
      REQUIRE(destinationGroup != nullptr);
      const auto destinationGroupId = destinationGroup->getId();
      const ImportH5ObjectPathsAction action(filePath, {k_SmallAMPath, k_SmallArrayPath});
      auto importResult = action.apply(destination, IDataAction::Mode::Execute);
      SIMPLNX_RESULT_REQUIRE_VALID(importResult);
      CHECK(g_ExcludedMaterializations.load() == 0);
      REQUIRE(destination.containsData(k_SmallAMPath));
      REQUIRE(destination.containsData(k_SmallArrayPath));
      CHECK_FALSE(destination.containsData(k_LargeAMPath));
      CHECK(destination.getAllDataPaths().size() == 3);
      DataGroup* retainedGroupPtr = nullptr;
      REQUIRE_NOTHROW(retainedGroupPtr = &destination.getDataRefAs<DataGroup>(k_GroupPath));
      REQUIRE(retainedGroupPtr != nullptr);
      CHECK(retainedGroupPtr->getId() == destinationGroupId);
      UnitTest::CheckArraysInheritTupleDims(destination);
    }
  }
  CHECK(collection.getManager("HDF5") == originalManager);
}

TEST_CASE("Dream3dLoadingApi: Recovery file with user data path redirect")
{
  // RAII guards ensure cleanup even when a REQUIRE throws on failure.
  ScopedTempFile filePathGuard{GetTestOutputDir() / "Dream3dLoadingApiTest_RecoveryRedirect.dream3d"};
  const fs::path& filePath = filePathGuard.path;
  const fs::path userDataPath = GetTestOutputDir() / "my_user_output.dream3d";

  // Write the minimal redirect variant — dataStructure/pipeline ignored.
  DataStructure emptyDs;
  Result<> writeResult = DREAM3D::WriteRecoveryFile(filePath, emptyDs, {}, userDataPath);
  REQUIRE(writeResult.valid());

  // File exists and is small (minimal variant is kilobytes, full variant is
  // MB-GB with real data). The sanity check is "< 64 KB" to catch a
  // regression where WriteRecoveryFile silently falls through to the full
  // path despite userDataFilePath being set.
  REQUIRE(fs::exists(filePath));
  REQUIRE(fs::file_size(filePath) < 64 * 1024);

  // Read back: attribute should contain the ABSOLUTE form of userDataPath.
  auto readResult = DREAM3D::ReadUserDataFilePathAttribute(filePath);
  REQUIRE(readResult.valid());
  REQUIRE(readResult.value().has_value());
  REQUIRE(readResult.value().value() == fs::absolute(userDataPath));

  // A standard recovery file (no redirect) should return nullopt.
  ScopedTempFile standardFilePathGuard{GetTestOutputDir() / "Dream3dLoadingApiTest_RecoveryStandard.dream3d"};
  const fs::path& standardFilePath = standardFilePathGuard.path;
  DataStructure simpleDs = CreateSimpleTestDataStructure();
  REQUIRE(DREAM3D::WriteRecoveryFile(standardFilePath, simpleDs).valid());

  auto readStandard = DREAM3D::ReadUserDataFilePathAttribute(standardFilePath);
  REQUIRE(readStandard.valid());
  REQUIRE_FALSE(readStandard.value().has_value());
}

TEST_CASE("Dream3dLoadingApi: Recovery file with all in-core data")
{
  DataStructure srcDs = CreateSimpleTestDataStructure();
  fs::path filePath = GetTestOutputDir() / "Dream3dLoadingApiTest_Recovery.dream3d";

  // WriteRecoveryFile writes in-core stores' full data inline (only OOC stores
  // are written as lightweight placeholders pointing at their backing files).
  // This DataStructure is entirely in-core, so all array data is stored inline
  // in the recovery file.
  Result<> writeResult = DREAM3D::WriteRecoveryFile(filePath, srcDs);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  REQUIRE(fs::exists(filePath));

  Result<DataStructure> result = DREAM3D::LoadDataStructure(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  const DataStructure& ds = result.value();

  const auto& smallPath = k_SmallArrayPath;

  // Verify data integrity through the recovery round-trip
  const auto& smallStore = ds.getDataRefAs<Int32Array>(smallPath).getDataStoreRef();
  for(usize i = 0; i < k_SmallArraySize; ++i)
  {
    CHECK(smallStore[i] == static_cast<int32>(i * 3));
  }
}
