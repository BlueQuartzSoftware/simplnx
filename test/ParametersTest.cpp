#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FooParamKey = "FooParam";
constexpr StringLiteral k_BarParamKey = "BarParam";
constexpr StringLiteral k_BazParamKey = "BazParam";
constexpr StringLiteral k_BizParamKey = "BizParam";
} // namespace

TEST_CASE("ParametersTest")
{
  Parameters params;
  params.insert(std::make_unique<Int32Parameter>(k_FooParamKey, "Foo", "Test parameter", 42));
  params.insertSeparator(Parameters::Separator{"Separator Name"});

  REQUIRE_FALSE(params.containsGroup(k_BarParamKey));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_BarParamKey, "Bar", "Test parameter", false));
  REQUIRE(params.containsGroup(k_BarParamKey));

  params.insert(std::make_unique<StringParameter>(k_BazParamKey, "Baz", "Test parameter", "default"));

  params.linkParameters(k_BarParamKey, k_BazParamKey, true);

  REQUIRE(params.size() == 3);

  REQUIRE(params.getLayout().size() == 4);

  REQUIRE(std::holds_alternative<Parameters::ParameterKey>(params.getLayout().at(0)));
  REQUIRE(std::holds_alternative<Parameters::Separator>(params.getLayout().at(1)));

  REQUIRE(params.getNumberOfLinkedGroups(k_FooParamKey) == 0);
  REQUIRE(params.getNumberOfLinkedGroups(k_BazParamKey) == 1);

  REQUIRE(params.getLinkedGroups(k_FooParamKey).empty());
  const auto& bazGroups = params.getLinkedGroups(k_BazParamKey);
  REQUIRE(bazGroups.size() == 1);
  REQUIRE(bazGroups.at(0).first == k_BarParamKey);

  const std::vector<std::string> expectedKeys = {k_FooParamKey, k_BarParamKey, k_BazParamKey};

  REQUIRE(params.getKeys() == expectedKeys);

  const std::vector<std::string> expectedGroupKeys = {k_BarParamKey};

  REQUIRE(params.getGroupKeys() == expectedGroupKeys);

  const std::vector<std::string> expectedKeyInGroup = {k_BazParamKey};

  REQUIRE(params.getKeysInGroup(k_BarParamKey) == expectedKeyInGroup);

  REQUIRE(params.isParameterActive(k_FooParamKey, {}));

  Arguments args;
  args.insertOrAssign(k_BarParamKey, true);

  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, false);

  REQUIRE_FALSE(params.isParameterActive(k_BazParamKey, args));

  // Test multiple linked groups

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_BizParamKey, "Biz", "Test Parameter", 0, ChoicesParameter::Choices{"Zero", "One", "Two"}));

  REQUIRE(params.size() == 4);

  params.linkParameters(k_BizParamKey, k_BazParamKey, std::make_any<ChoicesParameter::ValueType>(1));

  args.insertOrAssign(k_BarParamKey, false);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(0));
  REQUIRE_FALSE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, true);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(0));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, false);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(1));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));

  args.insertOrAssign(k_BarParamKey, true);
  args.insertOrAssign(k_BizParamKey, std::make_any<ChoicesParameter::ValueType>(1));
  REQUIRE(params.isParameterActive(k_BazParamKey, args));
}

TEST_CASE("FileSystemPathParameter::MatchExtension")
{
  using ExtensionsType = FileSystemPathParameter::ExtensionsType;

  SECTION("simple single-dot match")
  {
    const ExtensionsType accepted = {".nii"};
    auto m = FileSystemPathParameter::MatchExtension("scan.nii", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".nii");
  }

  SECTION("compound .nii.gz match")
  {
    const ExtensionsType accepted = {".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("scan.nii.gz", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".nii.gz");
  }

  SECTION("longest-wins when .gz and .nii.gz are both registered")
  {
    const ExtensionsType accepted = {".gz", ".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("scan.nii.gz", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".nii.gz");
  }

  SECTION(".gz still wins for plain .gz when no compound is registered")
  {
    const ExtensionsType accepted = {".gz", ".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("archive.tar.gz", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".gz");
  }

  SECTION("case-insensitive matching on both sides")
  {
    const ExtensionsType accepted = {".NII.GZ"};
    auto m = FileSystemPathParameter::MatchExtension("Scan.nii.gz", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".NII.GZ");
  }

  SECTION("non-matching suffix returns nullopt")
  {
    const ExtensionsType accepted = {".nii", ".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("image.tif", accepted);
    REQUIRE_FALSE(m.has_value());
  }

  SECTION("empty accepted set returns nullopt")
  {
    const ExtensionsType accepted;
    auto m = FileSystemPathParameter::MatchExtension("scan.nii", accepted);
    REQUIRE_FALSE(m.has_value());
  }

  SECTION("accepted extension longer than filename is skipped, not an error")
  {
    const ExtensionsType accepted = {".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("a", accepted);
    REQUIRE_FALSE(m.has_value());
  }

  SECTION("partial stem ending in the extension still matches")
  {
    // Matching is a pure literal suffix test, consistent with how users drop
    // files that happen to end in ".nii.gz" regardless of the stem.
    const ExtensionsType accepted = {".nii.gz"};
    auto m = FileSystemPathParameter::MatchExtension("subject42.nii.gz", accepted);
    REQUIRE(m.has_value());
    REQUIRE(*m == ".nii.gz");
  }

  SECTION("file that merely contains '.gz' inside its name is not matched as .gz")
  {
    // "foo.gz.bak" should NOT match ".gz" because ".gz" is not a suffix.
    const ExtensionsType accepted = {".gz"};
    auto m = FileSystemPathParameter::MatchExtension("foo.gz.bak", accepted);
    REQUIRE_FALSE(m.has_value());
  }
}
