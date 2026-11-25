#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <any>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

static_assert(!std::is_default_constructible_v<ArraySelectionParameter::AllowedDataTypes>);
static_assert(!std::is_default_constructible_v<MultiArraySelectionParameter::AllowedDataTypesType>);
static_assert(std::is_same_v<ArraySelectionParameter::AllowedDataTypes, MultiArraySelectionParameter::AllowedDataTypesType>);
static_assert(std::is_convertible_v<DataTypeSetType, ArraySelectionParameter::AllowedDataTypes>);

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

  ~ReportingOocStore() override = default;
  ReportingOocStore(const ReportingOocStore&) = default;
  ReportingOocStore(ReportingOocStore&&) = default;
  ReportingOocStore& operator=(const ReportingOocStore&) = delete;
  ReportingOocStore& operator=(ReportingOocStore&&) noexcept = default;

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
void CheckLocation(const DataStructure& dataStructure, const DataPath& path, ArraySelectionParameter::DataLocation location, bool expectedValid)
{
  ArraySelectionParameter parameter("values", "Values", "Select values.", path, GetDataArrayOnlyTypes(), {DataType::int32}, {{1}}, location);
  auto copy = parameter.clone();
  const auto* typedCopyPtr = dynamic_cast<ArraySelectionParameter*>(copy.get());
  REQUIRE(typedCopyPtr != nullptr);
  CHECK(parameter.allowedDataLocations() == location);
  CHECK(typedCopyPtr->allowedDataLocations() == location);

  const auto originalResult = parameter.validate(dataStructure, std::any{path});
  const auto copyResult = typedCopyPtr->validate(dataStructure, std::any{path});
  CHECK(originalResult.valid() == expectedValid);
  CHECK(copyResult.valid() == expectedValid);

  if(!expectedValid)
  {
    for(const auto* resultPtr : {&originalResult, &copyResult})
    {
      if(resultPtr->invalid())
      {
        CHECK_FALSE(resultPtr->errors().empty());
        if(!resultPtr->errors().empty())
        {
          CHECK(resultPtr->errors().front().code == filter_parameter::constants::k_ValidateDataLocationError);
          CHECK(resultPtr->errors().front().message.find(path.toString()) != std::string::npos);
          CHECK(resultPtr->errors().front().message.find("in-memory") != std::string::npos);
          CHECK(resultPtr->errors().front().message.find("out-of-core") != std::string::npos);
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
  const std::vector<StoreCase> stores = {{.path = residentPath, .actual = IDataStore::StoreType::InMemory},
                                         {.path = reportedOocPath, .actual = IDataStore::StoreType::OutOfCore},
                                         {.path = emptyPath, .actual = IDataStore::StoreType::Empty}};

  const std::vector<ArraySelectionParameter::DataLocation> locations = {ArraySelectionParameter::DataLocation::Any, ArraySelectionParameter::DataLocation::InMemory,
                                                                        ArraySelectionParameter::DataLocation::OutOfCore};
  for(const auto& store : stores)
  {
    const auto* arrayPtr = dataStructure.getDataAs<IDataArray>(store.path);
    REQUIRE(arrayPtr != nullptr);
    CHECK(arrayPtr->getStoreType() == store.actual);
    for(const auto location : locations)
    {
      INFO("array: " << store.path.toString());
      const bool expectedValid = location == ArraySelectionParameter::DataLocation::Any || store.actual == IDataStore::StoreType::Empty ||
                                 (location == ArraySelectionParameter::DataLocation::InMemory && store.actual == IDataStore::StoreType::InMemory) ||
                                 (location == ArraySelectionParameter::DataLocation::OutOfCore && store.actual == IDataStore::StoreType::OutOfCore);
      CheckLocation(dataStructure, store.path, location, expectedValid);
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
      ArraySelectionParameter parameter("values", "Values", "Select values.", path, GetDataArrayOnlyTypes(), {DataType::int32}, {{1}}, location);
      auto copy = parameter.clone();
      const auto* typedCopyPtr = dynamic_cast<ArraySelectionParameter*>(copy.get());
      REQUIRE(typedCopyPtr != nullptr);
      CHECK(typedCopyPtr->allowedDataLocations() == location);
      const auto originalResult = parameter.validate(dataStructure, std::any{path});
      const auto copyResult = typedCopyPtr->validate(dataStructure, std::any{path});
      const auto expectedCode = path.getTargetName() == "WrongType" ? filter_parameter::constants::k_ValidateAllowedTypeError : filter_parameter::constants::k_ValidateTupleShapeValue;
      for(const auto* resultPtr : {&originalResult, &copyResult})
      {
        CHECK(resultPtr->invalid());
        if(resultPtr->invalid())
        {
          CHECK_FALSE(resultPtr->errors().empty());
          if(!resultPtr->errors().empty())
          {
            CHECK(resultPtr->errors().front().code == expectedCode);
          }
        }
      }
    }
  }
}

TEST_CASE("ArraySelectionParameter validates array kinds, element types, and component shapes", "[simplnx][Parameters]")
{
  using namespace filter_parameter::constants;
  using Kind = IArray::ArrayType;
  DataStructure dataStructure;
  const DataPath stringsPath({"Strings"});
  const DataPath neighborsPath({"Neighbors"});
  REQUIRE(StringArray::CreateWithValues(dataStructure, stringsPath.getTargetName(), {2}, {"one", "two"}) != nullptr);
  REQUIRE(NeighborList<int32>::Create(dataStructure, neighborsPath.getTargetName(), {2}) != nullptr);

  SECTION("DataArray-only selection rejects the other kinds before checking shapes or element types")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetDataArrayOnlyTypes(), {DataType::float32}, {{2}});
    CHECK(parameter.allowedDataLocations() == ArraySelectionParameter::DataLocation::Any);
    const bool isString = GENERATE(false, true);
    const auto& path = isString ? stringsPath : neighborsPath;
    const auto result = parameter.validatePath(dataStructure, path);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    const auto& error = result.errors().front();
    CHECK(error.code == k_Validate_ArrayType_Error);
    CHECK(error.message.find(path.toString()) != std::string::npos);
    CHECK(error.message.find(isString ? "has kind StringArray" : "has kind NeighborListArray") != std::string::npos);
    CHECK(error.message.find("Select an array of one of these kinds: DataArray") != std::string::npos);
  }

  SECTION("Non-DataArray selections have one component, independent of the storage requirement")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetAllArrayTypes(), {DataType::int32}, {{2}, {1}}, ArraySelectionParameter::DataLocation::OutOfCore);
    const auto copy = parameter.clone();
    const auto* clonedParameter = dynamic_cast<const ArraySelectionParameter*>(copy.get());
    REQUIRE(clonedParameter != nullptr);
    CHECK(clonedParameter->allowedArrayTypes() == std::set<Kind>{Kind::DataArray, Kind::NeighborListArray, Kind::StringArray});
    CHECK(clonedParameter->allowedDataTypes() == std::set<DataType>{DataType::int32});
    for(const auto& path : {stringsPath, neighborsPath})
    {
      const auto result = clonedParameter->validatePath(dataStructure, path);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
  }

  SECTION("The numeric kind set accepts a matching NeighborList")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetNumericArrayTypes(), {DataType::int32}, {{1}});
    const auto result = parameter.validatePath(dataStructure, neighborsPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  SECTION("NeighborList selection rejects a disallowed element type")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetNumericArrayTypes(), {DataType::float32}, {{1}});
    CHECK(parameter.allowedArrayTypes() == std::set<Kind>{Kind::DataArray, Kind::NeighborListArray});
    const auto result = parameter.validatePath(dataStructure, neighborsPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == k_ValidateAllowedTypeError);
    CHECK(result.errors().front().message.find("has element type 'int32'") != std::string::npos);
  }

  SECTION("Component shape requirements apply to strings and neighbor lists")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetAllArrayTypes(), {DataType::int32}, {{2}, {1, 1}});
    for(const auto& path : {stringsPath, neighborsPath})
    {
      const auto result = parameter.validatePath(dataStructure, path);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE(result.errors().size() == 1);
      CHECK(result.errors().front().code == k_ValidateTupleShapeValue);
    }
  }

  SECTION("String-only selection may omit element types")
  {
    const ArraySelectionParameter parameter("values", "Values", "Select values.", {}, {Kind::StringArray}, ArraySelectionParameter::AllowedDataTypes::StringOnly());
    CHECK(parameter.allowedDataTypes().empty());
    const auto result = parameter.validatePath(dataStructure, stringsPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
}

TEST_CASE("ArraySelectionParameter requires concrete kinds and element types for numeric arrays", "[simplnx][Parameters]")
{
  using Kind = IArray::ArrayType;
  REQUIRE_THROWS_AS(ArraySelectionParameter("values", "Values", "Select values.", {}, {}, {DataType::int32}), std::runtime_error);
  REQUIRE_THROWS_AS(ArraySelectionParameter("values", "Values", "Select values.", {}, {Kind::DataArray, Kind::Any}, {DataType::int32}), std::runtime_error);
  for(const auto kind : {Kind::DataArray, Kind::NeighborListArray})
  {
    const DataTypeSetType emptyTypes;
    REQUIRE_THROWS_AS(ArraySelectionParameter("values", "Values", "Select values.", {}, {kind}, emptyTypes), std::runtime_error);
    REQUIRE_THROWS_AS(ArraySelectionParameter("values", "Values", "Select values.", {}, {kind}, ArraySelectionParameter::AllowedDataTypes::StringOnly()), std::runtime_error);
  }
}

TEST_CASE("MultiArraySelectionParameter validates kinds, element types, and component shapes", "[simplnx][Parameters]")
{
  using namespace filter_parameter::constants;
  using Kind = IArray::ArrayType;
  DataStructure dataStructure;
  const DataPath integersPath({"Integers"});
  const DataPath floatsPath({"Floats"});
  const DataPath neighborsPath({"Neighbors"});
  const DataPath stringsPath({"Strings"});
  REQUIRE(Int32ArrayType::CreateWithStore<DataStore<int32>>(dataStructure, integersPath.getTargetName(), {2}, {1}) != nullptr);
  REQUIRE(Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, floatsPath.getTargetName(), {2}, {1}) != nullptr);
  REQUIRE(NeighborList<int32>::Create(dataStructure, neighborsPath.getTargetName(), {2}) != nullptr);
  REQUIRE(StringArray::CreateWithValues(dataStructure, stringsPath.getTargetName(), {2}, {"one", "two"}) != nullptr);

  SECTION("Any accepts all kinds and checks element types on numeric arrays")
  {
    const MultiArraySelectionParameter parameter("values", "Values", "Select values.", {}, {Kind::Any}, {DataType::float32}, {{1}});
    const auto validResult = parameter.validatePaths(dataStructure, {floatsPath, stringsPath});
    SIMPLNX_RESULT_REQUIRE_VALID(validResult);
    const auto result = parameter.validatePaths(dataStructure, {integersPath, stringsPath, neighborsPath});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 2);
    CHECK(result.errors()[0].code == k_ValidateAllowedTypeError);
    CHECK(result.errors()[0].message.find("DataArray at path 'Integers' has element type 'int32'") != std::string::npos);
    CHECK(result.errors()[1].code == k_ValidateAllowedTypeError);
    CHECK(result.errors()[1].message.find("NeighborListArray at path 'Neighbors' has element type 'int32'") != std::string::npos);
  }

  SECTION("Kind errors precede element type and component shape checks")
  {
    const MultiArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetDataArrayOnlyTypes(), {DataType::float32}, {{2}});
    const auto result = parameter.validatePaths(dataStructure, {neighborsPath});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == k_Validate_ArrayType_Error);
    CHECK(result.errors().front().message.find("Array at path 'Neighbors' has kind NeighborListArray. Select an array of one of these kinds: DataArray.") != std::string::npos);
  }

  SECTION("Strings have no element type restriction")
  {
    const MultiArraySelectionParameter parameter("values", "Values", "Select values.", {}, {Kind::StringArray}, MultiArraySelectionParameter::AllowedDataTypesType::StringOnly(), {{1}});
    CHECK(parameter.allowedDataTypes().empty());
    const auto result = parameter.validatePaths(dataStructure, {stringsPath});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }

  SECTION("Component shapes are checked for strings and neighbor lists")
  {
    const MultiArraySelectionParameter parameter("values", "Values", "Select values.", {}, GetAllArrayTypes(), {DataType::int32}, {{2}});
    const auto result = parameter.validatePaths(dataStructure, {stringsPath, neighborsPath});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 2);
    for(const auto& error : result.errors())
    {
      CHECK(error.code == k_ValidateTupleShapeValue);
    }
  }
}

TEST_CASE("MultiArraySelectionParameter requires element types for numeric arrays and Any", "[simplnx][Parameters]")
{
  using Kind = IArray::ArrayType;
  const auto kind = GENERATE(Kind::DataArray, Kind::NeighborListArray, Kind::Any);
  const DataTypeSetType emptyTypes;
  REQUIRE_THROWS_AS(MultiArraySelectionParameter("values", "Values", "Select values.", {}, {kind}, emptyTypes), std::runtime_error);
  REQUIRE_THROWS_AS(MultiArraySelectionParameter("values", "Values", "Select values.", {}, {kind}, MultiArraySelectionParameter::AllowedDataTypesType::StringOnly()), std::runtime_error);
}

TEST_CASE("MultiArraySelectionParameter requires at least one array kind", "[simplnx][Parameters]")
{
  REQUIRE_THROWS_AS(MultiArraySelectionParameter("values", "Values", "Select values.", {}, {}, {DataType::int32}), std::runtime_error);
}
