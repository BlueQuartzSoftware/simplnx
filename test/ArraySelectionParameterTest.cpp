#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <memory>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @class ReportingOocStore
 * @brief Reports OOC storage while values remain in memory.
 *
 * Location validation reads only store metadata, so this fixture does not need an OOC backend.
 */
class ReportingOocStore final : public DataStore<int32>
{
public:
  using DataStore<int32>::DataStore;

  /**
   * @brief Reports OOC storage for location validation.
   * @return OutOfCore for this test store.
   */
  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

/**
 * @brief Checks location validation for a parameter and its clone.
 * @param dataStructure Contains the selected array.
 * @param path Identifies the selected array.
 * @param location Specifies the required storage location.
 * @param expectedValid Specifies the expected validation result.
 */
void checkLocation(const DataStructure& dataStructure, const DataPath& path, ArraySelectionParameter::DataLocation location, bool expectedValid)
{
  ArraySelectionParameter parameter("values", "Values", "Select values.", path, {DataType::int32}, {{1}}, location);
  auto copy = parameter.clone();
  const auto* typedCopy = dynamic_cast<ArraySelectionParameter*>(copy.get());
  REQUIRE(typedCopy != nullptr);
  CHECK(parameter.allowedDataLocations() == location);
  CHECK(typedCopy->allowedDataLocations() == location);

  const auto originalResult = parameter.validate(dataStructure, std::any{path});
  const auto copyResult = typedCopy->validate(dataStructure, std::any{path});
  CHECK(originalResult.valid() == expectedValid);
  CHECK(copyResult.valid() == expectedValid);

  if(!expectedValid)
  {
    for(const auto* result : {&originalResult, &copyResult})
    {
      if(result->invalid())
      {
        CHECK_FALSE(result->errors().empty());
        if(!result->errors().empty())
        {
          CHECK(result->errors().front().code == FilterParameter::Constants::k_Validate_DataLocation_Error);
          CHECK(result->errors().front().message.find(path.toString()) != std::string::npos);
          CHECK(result->errors().front().message.find("in-memory") != std::string::npos);
          CHECK(result->errors().front().message.find("out-of-core") != std::string::npos);
        }
      }
    }
  }
}
} // namespace

TEST_CASE("ArraySelectionParameter location and clone", "[simplnx][Parameters]")
{
  DataStructure dataStructure;
  const DataPath residentPath({"Resident"});
  const DataPath reportedOocPath({"ReportedOoc"});
  const DataPath emptyPath({"Empty"});

  REQUIRE(DataArray<int32>::Create(dataStructure, residentPath.getTargetName(), std::make_shared<DataStore<int32>>(ShapeType{2}, ShapeType{1}, 0)) != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, reportedOocPath.getTargetName(), std::make_shared<ReportingOocStore>(ShapeType{2}, ShapeType{1}, 0)) != nullptr);
  auto emptyStoreResult = EmptyDataStore<int32>::Create({2}, {1}, "");
  REQUIRE(emptyStoreResult.valid());
  std::shared_ptr<EmptyDataStore<int32>> emptyStore(std::move(emptyStoreResult.value()));
  REQUIRE(DataArray<int32>::Create(dataStructure, emptyPath.getTargetName(), std::move(emptyStore)) != nullptr);

  struct StoreCase
  {
    DataPath path;
    IDataStore::StoreType actual;
  };
  const std::vector<StoreCase> stores = {{residentPath, IDataStore::StoreType::InMemory}, {reportedOocPath, IDataStore::StoreType::OutOfCore}, {emptyPath, IDataStore::StoreType::Empty}};

  const std::vector<ArraySelectionParameter::DataLocation> locations = {ArraySelectionParameter::DataLocation::Any, ArraySelectionParameter::DataLocation::InMemory,
                                                                        ArraySelectionParameter::DataLocation::OutOfCore};
  for(const auto& store : stores)
  {
    const auto* array = dataStructure.getDataAs<IDataArray>(store.path);
    REQUIRE(array != nullptr);
    CHECK(array->getStoreType() == store.actual);
    for(const auto location : locations)
    {
      INFO("array: " << store.path.toString());
      const bool expectedValid = location == ArraySelectionParameter::DataLocation::Any || store.actual == IDataStore::StoreType::Empty ||
                                 (location == ArraySelectionParameter::DataLocation::InMemory && store.actual == IDataStore::StoreType::InMemory) ||
                                 (location == ArraySelectionParameter::DataLocation::OutOfCore && store.actual == IDataStore::StoreType::OutOfCore);
      checkLocation(dataStructure, store.path, location, expectedValid);
    }
  }
}

TEST_CASE("ArraySelectionParameter keeps type and shape checks for empty stores", "[simplnx][Parameters]")
{
  DataStructure dataStructure;
  auto wrongTypeStore = EmptyDataStore<float32>::Create({2}, {1}, "");
  REQUIRE(wrongTypeStore.valid());
  std::shared_ptr<EmptyDataStore<float32>> floatStore(std::move(wrongTypeStore.value()));
  REQUIRE(DataArray<float32>::Create(dataStructure, "WrongType", std::move(floatStore)) != nullptr);

  auto wrongShapeStore = EmptyDataStore<int32>::Create({2}, {2}, "");
  REQUIRE(wrongShapeStore.valid());
  std::shared_ptr<EmptyDataStore<int32>> shapeStore(std::move(wrongShapeStore.value()));
  REQUIRE(DataArray<int32>::Create(dataStructure, "WrongShape", std::move(shapeStore)) != nullptr);

  for(const auto location : {ArraySelectionParameter::DataLocation::Any, ArraySelectionParameter::DataLocation::InMemory, ArraySelectionParameter::DataLocation::OutOfCore})
  {
    for(const auto& path : {DataPath({"WrongType"}), DataPath({"WrongShape"})})
    {
      ArraySelectionParameter parameter("values", "Values", "Select values.", path, {DataType::int32}, {{1}}, location);
      auto copy = parameter.clone();
      const auto* typedCopy = dynamic_cast<ArraySelectionParameter*>(copy.get());
      REQUIRE(typedCopy != nullptr);
      CHECK(typedCopy->allowedDataLocations() == location);
      const auto originalResult = parameter.validate(dataStructure, std::any{path});
      const auto copyResult = typedCopy->validate(dataStructure, std::any{path});
      const auto expectedCode = path.getTargetName() == "WrongType" ? FilterParameter::Constants::k_Validate_AllowedType_Error : FilterParameter::Constants::k_Validate_TupleShapeValue;
      for(const auto* result : {&originalResult, &copyResult})
      {
        CHECK(result->invalid());
        if(result->invalid())
        {
          CHECK_FALSE(result->errors().empty());
          if(!result->errors().empty())
          {
            CHECK(result->errors().front().code == expectedCode);
          }
        }
      }
    }
  }
}

TEST_CASE("ArraySelectionParameter retains StringArray and default location behavior", "[simplnx][Parameters]")
{
  DataStructure dataStructure;
  const DataPath path({"Strings"});
  REQUIRE(StringArray::CreateWithValues(dataStructure, path.getTargetName(), {2}, {"one", "two"}) != nullptr);

  ArraySelectionParameter defaultParameter("values", "Values", "Select values.", path, {DataType::int32}, {{1}});
  CHECK(defaultParameter.allowedDataLocations() == ArraySelectionParameter::DataLocation::Any);

  for(const auto location : {ArraySelectionParameter::DataLocation::Any, ArraySelectionParameter::DataLocation::InMemory, ArraySelectionParameter::DataLocation::OutOfCore})
  {
    checkLocation(dataStructure, path, location, true);
  }
}
