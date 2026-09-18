#include "simplnx/DataStructure/ListStore.hpp"
#include "simplnx/DataStructure/StringStore.hpp"

#include <catch2/catch.hpp>

#include <limits>
#include <string>
#include <vector>

using namespace nx::core;

TEST_CASE("StringStore resize preserves initialized values and rejects impossible lengths", "[simplnx][StoreResizeContracts]")
{
  StringStore store(std::vector<std::string>{"alpha", "beta"}, ShapeType{2, 1});

  SECTION("Growth initializes new strings and shrink retains the prefix")
  {
    REQUIRE(store.resizeTuples({2, 2}).valid());
    REQUIRE(store.getTupleShape() == ShapeType{2, 2});
    REQUIRE(store.getNumberOfTuples() == 4);
    CHECK(store.getValue(0) == "alpha");
    CHECK(store.getValue(1) == "beta");
    CHECK(store.getValue(2).empty());
    CHECK(store.getValue(3).empty());

    REQUIRE(store.resizeTuples({1}).valid());
    REQUIRE(store.getTupleShape() == ShapeType{1});
    REQUIRE(store.getNumberOfTuples() == 1);
    CHECK(store.getValue(0) == "alpha");
    REQUIRE(store.resizeTuples({0}).valid());
    REQUIRE(store.empty());
    REQUIRE(store.resizeTuples({2}).valid());
    CHECK(store.getValue(0).empty());
    CHECK(store.getValue(1).empty());
  }

  SECTION("Rejected length preserves the shape and values")
  {
    const usize impossibleSize = std::numeric_limits<usize>::max();
    REQUIRE(impossibleSize > std::vector<std::string>{}.max_size());
    Result<> result;
    REQUIRE_NOTHROW(result = store.resizeTuples({impossibleSize}));
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -6035);
    CHECK(result.errors().front().message.find("StringStore") != std::string::npos);
    CHECK(store.getTupleShape() == ShapeType{2, 1});
    REQUIRE(store.getNumberOfTuples() == 2);
    CHECK(store.getValue(0) == "alpha");
    CHECK(store.getValue(1) == "beta");
  }
}

TEST_CASE("ListStore resize preserves initialized lists and rejects impossible lengths", "[simplnx][StoreResizeContracts]")
{
  ListStore<int32> store(ShapeType{2, 1});
  store.setList(0, std::vector<int32>{7, 9});
  store.setList(1, std::vector<int32>{-4});

  SECTION("Growth initializes empty lists and shrink retains the prefix")
  {
    REQUIRE(store.resizeTuples({2, 2}).valid());
    REQUIRE(store.getTupleShape() == ShapeType{2, 2});
    REQUIRE(store.getNumberOfTuples() == 4);
    CHECK(store.getList(0) == std::vector<int32>{7, 9});
    CHECK(store.getList(1) == std::vector<int32>{-4});
    CHECK(store.getList(2).empty());
    CHECK(store.getList(3).empty());

    REQUIRE(store.resizeTuples({1}).valid());
    CHECK(store.getTupleShape() == ShapeType{1});
    REQUIRE(store.getNumberOfTuples() == 1);
    CHECK(store.getList(0) == std::vector<int32>{7, 9});
    REQUIRE(store.resizeTuples({0}).valid());
    REQUIRE(store.getNumberOfTuples() == 0);
    REQUIRE(store.resizeTuples({2}).valid());
    CHECK(store.getList(0).empty());
    CHECK(store.getList(1).empty());
  }

  SECTION("Rejected length preserves the shape and lists")
  {
    const usize impossibleSize = std::numeric_limits<usize>::max();
    REQUIRE(impossibleSize > std::vector<std::vector<int32>>{}.max_size());
    Result<> result;
    REQUIRE_NOTHROW(result = store.resizeTuples({impossibleSize}));
    REQUIRE(result.invalid());
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors().front().code == -6035);
    CHECK(result.errors().front().message.find("ListStore") != std::string::npos);
    CHECK(store.getTupleShape() == ShapeType{2, 1});
    REQUIRE(store.getNumberOfTuples() == 2);
    CHECK(store.getList(0) == std::vector<int32>{7, 9});
    CHECK(store.getList(1) == std::vector<int32>{-4});
  }
}
