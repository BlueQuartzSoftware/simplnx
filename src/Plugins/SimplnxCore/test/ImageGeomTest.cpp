#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Arguments.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("SimplnxCore::ImageGeom: Test Index Calculation", "[Geometry][ImageGeom]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");

  SizeVec3 dims = {10, 20, 30};
  FloatVec3 res = {0.4f, 2.3f, 5.0f};
  FloatVec3 origin = {-1.0f, 6.0f, 10.0f};

  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(origin);
  imageGeom->setSpacing(res);

  std::array<float, 3> coords = {2.5f, 9.23f, 12.78f};
  SizeVec3 indices = {0, 0, 0};
  ImageGeom::ErrorType err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::NoError);

  // X Coord out of bounds
  coords[0] = -5.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::XOutOfBoundsLow);

  coords[0] = 200.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::XOutOfBoundsHigh);

  // Y Coord out of bounds
  coords[0] = 2.9f;
  coords[1] = 4.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::YOutOfBoundsLow);
  coords[1] = 200.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::YOutOfBoundsHigh);

  // Z Coord out of bounds
  coords[0] = 2.5f;
  coords[1] = 9.23f;
  coords[2] = 5.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::ZOutOfBoundsLow);
  coords[2] = 2000.0f;
  err = imageGeom->computeCellIndex(coords, indices);
  REQUIRE(err == ImageGeom::ErrorType::ZOutOfBoundsHigh);
}

TEST_CASE("SimplnxCore::ImageGeom: Test Coords To Index", "[Geometry][ImageGeom]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;

  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, "Image Geometry");

  SizeVec3 dims = {10, 20, 30};
  FloatVec3 spacing = {0.5f, 0.5f, 0.5f};
  FloatVec3 origin = {-10.0f, 5.0f, 2.0f};

  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(origin);
  imageGeom->setSpacing(spacing);

  {
    std::array<float, 3> coords = {-9.9f, 5.25f, 2.15f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 0);
  }

  {
    std::array<float, 3> coords = {-9.26f, 5.25f, 2.1f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 1);
  }

  {
    std::array<float, 3> coords = {-9.8f, 5.8f, 2.05f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 10);
  }

  {
    std::array<float, 3> coords = {-6.55f, 5.6f, 2.45f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 16);
  }

  {
    std::array<float, 3> coords = {-6.95f, 5.9f, 2.55f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 216);
  }

  {
    std::array<float, 3> coords = {-10.0001f, 5.75f, 2.75f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }

  {
    std::array<float, 3> coords = {-9.75f, 4.9999f, 2.75f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }

  {
    std::array<float, 3> coords = {-9.75f, 5.75f, 1.9999f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }

  {
    std::array<float, 3> coords = {-4.9f, 5.75f, 2.75f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }

  {
    std::array<float, 3> coords = {-9.75f, 15.1f, 2.75f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }

  {
    std::array<float, 3> coords = {-9.75f, 5.75f, 17.1f};
    std::optional<size_t> result = imageGeom->getIndex(coords[0], coords[1], coords[2]);
    REQUIRE(!result.has_value());
  }
}
