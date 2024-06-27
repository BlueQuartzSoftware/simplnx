#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataPath.hpp"

using namespace nx::core;

namespace
{
const std::vector<std::string> k_CommonStringsVec = {"Path1/Path2", "Path1", "", " "};
const std::vector<std::string> k_EdgeCaseStringsVec = {" / ", "Path1/ Path2", "Path1 / Path2 ", "Path1/ ", "/"};
const std::vector<std::string> k_InvalidStringsVec = {"Path1/", "Path1//Path3"};

bool CompareFromStringToExemplar(const std::string& input, const DataPath& exemplar)
{
  auto result = DataPath::FromString(input);
  if(result.has_value())
  {
    return (result.value() == exemplar);
  }
  else
  {
    return false;
  }
}
} // namespace

TEST_CASE("Simplnx::Valid DataPath Creation", "[Simplnx][DataPath]")
{
  DataPath commonExemplar1 = DataPath({"Path1"}).createChildPath("Path2");
  REQUIRE(CompareFromStringToExemplar(k_CommonStringsVec[0], commonExemplar1));

  DataPath commonExemplar2 = DataPath({"Path1"});
  REQUIRE(CompareFromStringToExemplar(k_CommonStringsVec[1], commonExemplar2));

  DataPath commonExemplar3 = DataPath{};
  REQUIRE(CompareFromStringToExemplar(k_CommonStringsVec[2], commonExemplar3));

  DataPath commonExemplar4 = DataPath({" "});
  REQUIRE(CompareFromStringToExemplar(k_CommonStringsVec[3], commonExemplar4));

  DataPath edgeExemplar1 = DataPath({" "}).createChildPath(" ");
  REQUIRE(CompareFromStringToExemplar(k_EdgeCaseStringsVec[0], edgeExemplar1));

  DataPath edgeExemplar2 = DataPath({"Path1"}).createChildPath(" Path2");
  REQUIRE(CompareFromStringToExemplar(k_EdgeCaseStringsVec[1], edgeExemplar2));

  DataPath edgeExemplar3 = DataPath({"Path1 "}).createChildPath(" Path2 ");
  REQUIRE(CompareFromStringToExemplar(k_EdgeCaseStringsVec[2], edgeExemplar3));

  DataPath edgeExemplar4 = DataPath({"Path1"}).createChildPath(" ");
  REQUIRE(CompareFromStringToExemplar(k_EdgeCaseStringsVec[3], edgeExemplar4));

  DataPath edgeExemplar5 = DataPath{};
  REQUIRE(CompareFromStringToExemplar(k_EdgeCaseStringsVec[4], edgeExemplar5));
}

TEST_CASE("Simplnx::Invalid DataPath Creation", "[Simplnx][DataPath]")
{
  DataPath invalidExemplar1 = DataPath{};
  REQUIRE(!CompareFromStringToExemplar(k_InvalidStringsVec[0], invalidExemplar1));

  DataPath invalidExemplar2 = DataPath{};
  REQUIRE(!CompareFromStringToExemplar(k_InvalidStringsVec[1], invalidExemplar2));

  DataPath fromInvalidVector({"", "", ""});
  REQUIRE(fromInvalidVector.toString().empty());
}
