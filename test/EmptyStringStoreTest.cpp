#include "simplnx/DataStructure/EmptyStringStore.hpp"
#include "simplnx/DataStructure/StringStore.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("EmptyStringStore: basic metadata")
{
  ShapeType tupleShape = {5};
  EmptyStringStore store(tupleShape);

  REQUIRE(store.size() == 5);
  REQUIRE(store.getNumberOfTuples() == 5);
  REQUIRE(store.getTupleShape() == tupleShape);
  REQUIRE(store.empty() == false);
  REQUIRE(store.isPlaceholder() == true);
}

TEST_CASE("EmptyStringStore: zero tuples")
{
  EmptyStringStore store({0});
  REQUIRE(store.size() == 0);
  REQUIRE(store.empty() == true);
  REQUIRE(store.isPlaceholder() == true);
}

TEST_CASE("EmptyStringStore: data access throws")
{
  EmptyStringStore store({3});

  REQUIRE_THROWS_AS(store[0], std::runtime_error);
  REQUIRE_THROWS_AS(store.at(0), std::runtime_error);
  REQUIRE_THROWS_AS(store.getValue(0), std::runtime_error);
  REQUIRE_THROWS_AS(store.setValue(0, "test"), std::runtime_error);
}

TEST_CASE("EmptyStringStore: deep copy preserves placeholder status")
{
  EmptyStringStore original({4});
  auto copy = original.deepCopy();

  REQUIRE(copy->isPlaceholder() == true);
  REQUIRE(copy->size() == 4);
  REQUIRE(copy->getTupleShape() == ShapeType{4});
}

TEST_CASE("EmptyStringStore: resize")
{
  EmptyStringStore store({2});
  store.resizeTuples({10});
  REQUIRE(store.getNumberOfTuples() == 10);
  REQUIRE(store.size() == 10);
}

TEST_CASE("StringStore: isPlaceholder returns false")
{
  StringStore store(std::vector<std::string>{"a", "b", "c"}, ShapeType{3});
  REQUIRE(store.isPlaceholder() == false);
}
