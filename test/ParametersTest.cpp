#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_FooParamKey = "FooParam";
constexpr StringLiteral k_BarParamKey = "BarParam";
constexpr StringLiteral k_BazParamKey = "BazParam";
constexpr StringLiteral k_BizParamKey = "BizParam";

template <typename T>
void requireLegacyFloatRoundTrip(T expected)
{
  const auto result = SIMPLConversion::NumberToStringFilterParameterConverter<T>::convert(nlohmann::json(expected));
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CAPTURE(expected, result.value());

  usize consumed = 0;
  T parsed = 0;
  if constexpr(std::is_same_v<T, float32>)
  {
    parsed = std::stof(result.value(), &consumed);
  }
  else
  {
    parsed = std::stod(result.value(), &consumed);
  }
  CHECK((consumed == result.value().size()));
  CHECK((parsed == expected));
  CHECK((std::signbit(parsed) == std::signbit(expected)));
}

template <typename T>
void requireLegacyIntegerText(T value, const std::string& expected)
{
  const auto result = SIMPLConversion::NumberToStringFilterParameterConverter<T>::convert(nlohmann::json(value));
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  CHECK((result.value() == expected));
}

template <typename T>
void requireLegacyBinaryBoundaries(int32 lowestExponent)
{
  for(int32 exponent = -20; exponent >= lowestExponent; --exponent)
  {
    CAPTURE(exponent);
    for(const auto sign : std::array<T, 2>{1, -1})
    {
      const auto value = std::ldexp(sign, exponent);
      REQUIRE(std::isnormal(value));
      requireLegacyFloatRoundTrip(value);
      requireLegacyFloatRoundTrip(std::nextafter(value, -std::numeric_limits<T>::infinity()));
      requireLegacyFloatRoundTrip(std::nextafter(value, std::numeric_limits<T>::infinity()));
    }
  }
}
} // namespace

TEST_CASE("simplnx::StringParameter: Legacy numeric conversion preserves precision", "[simplnx][StringParameter][SIMPLConversion]")
{
  SECTION("float32 values round-trip to the original typed value")
  {
    for(const auto value : std::array<float32, 7>{1e-7F, -1e-7F, 1.00000011920928955078125F, 3.5F, -13.25F, 0.0F, -0.0F})
    {
      requireLegacyFloatRoundTrip(value);
    }
  }

  SECTION("float64 values round-trip to the original typed value")
  {
    for(const auto value : std::array<float64, 7>{1e-7, -1e-7, 1.0000000596046446, 3.5, -13.25, 0.0, -0.0})
    {
      requireLegacyFloatRoundTrip(value);
    }
  }

  SECTION("integer decimal strings retain all signed and unsigned boundaries")
  {
    requireLegacyIntegerText<int8>(std::numeric_limits<int8>::lowest(), "-128");
    requireLegacyIntegerText<int8>(std::numeric_limits<int8>::max(), "127");
    requireLegacyIntegerText<uint8>(std::numeric_limits<uint8>::lowest(), "0");
    requireLegacyIntegerText<uint8>(std::numeric_limits<uint8>::max(), "255");
    requireLegacyIntegerText<int16>(std::numeric_limits<int16>::lowest(), "-32768");
    requireLegacyIntegerText<int16>(std::numeric_limits<int16>::max(), "32767");
    requireLegacyIntegerText<uint16>(std::numeric_limits<uint16>::lowest(), "0");
    requireLegacyIntegerText<uint16>(std::numeric_limits<uint16>::max(), "65535");
    requireLegacyIntegerText<int32>(std::numeric_limits<int32>::lowest(), "-2147483648");
    requireLegacyIntegerText<int32>(std::numeric_limits<int32>::max(), "2147483647");
    requireLegacyIntegerText<uint32>(std::numeric_limits<uint32>::lowest(), "0");
    requireLegacyIntegerText<uint32>(std::numeric_limits<uint32>::max(), "4294967295");
    requireLegacyIntegerText<int64>(std::numeric_limits<int64>::lowest(), "-9223372036854775808");
    requireLegacyIntegerText<int64>(std::numeric_limits<int64>::max(), "9223372036854775807");
    requireLegacyIntegerText<uint64>(std::numeric_limits<uint64>::lowest(), "0");
    requireLegacyIntegerText<uint64>(std::numeric_limits<uint64>::max(), "18446744073709551615");
    requireLegacyIntegerText<int32>(-42, "-42");
    requireLegacyIntegerText<uint32>(42, "42");
  }

  SECTION("nonnumeric JSON retains the existing error code")
  {
    for(const auto& json : std::array<nlohmann::json, 5>{nlohmann::json("1e-7"), nlohmann::json(true), nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object()})
    {
      CAPTURE(json.dump());
      const auto result = SIMPLConversion::DoubleToStringFilterParameterConverter::convert(json);
      SIMPLNX_RESULT_REQUIRE_INVALID(result);
      REQUIRE((result.errors().size() == 1));
      CHECK((result.errors().front().code == -1));
    }
  }
}

TEST_CASE("simplnx::StringParameter: Legacy numeric conversion preserves binary boundaries", "[simplnx][StringParameter][SIMPLConversion]")
{
  SECTION("explicit asymmetric float64 rounding intervals")
  {
    requireLegacyFloatRoundTrip<float64>(0x1p-44);
    requireLegacyFloatRoundTrip<float64>(0x1p-24);
  }

  SECTION("normal float32 powers and immediate neighbors retain their original values")
  {
    requireLegacyBinaryBoundaries<float32>(-120);
  }

  SECTION("normal float64 powers and immediate neighbors retain their original values")
  {
    requireLegacyBinaryBoundaries<float64>(-1000);
  }
}

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
