#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <string>

using namespace nx::core;

namespace
{
/**
 * @brief Gives the corner direction that FindOctant reports for one octant.
 * @param octant Specifies the octant index.
 * @return The sign of each axis, -1 toward the low side and 1 toward the high side.
 *
 * FindOctant returns the index of the nearest voxel corner. It builds the corners in the
 * order P1 to P8, so this table repeats that order. The test derives the expected offsets
 * from this order. It does not read them from the tables that it checks.
 */
std::array<int64, 3> CornerDirection(usize octant)
{
  // P1(-,-,-) P2(+,-,-) P3(+,+,-) P4(-,+,-) P5(-,-,+) P6(+,-,+) P7(+,+,+) P8(-,+,+)
  constexpr std::array<std::array<int64, 3>, 8> k_Directions = {{{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}};
  return k_Directions[octant];
}
} // namespace

TEST_CASE("ImageRotationUtilities: Octant offsets match the trilinear corner order", "[ImageRotationUtilities][Octant]")
{
  // calculateInterpolatedValue reads the eight offsets in this order:
  // [0]=c000 [1]=c100 [2]=c110 [3]=c010 [4]=c001 [5]=c101 [6]=c111 [7]=c011.
  // Each entry therefore selects a low or a high neighbor on each axis.
  constexpr std::array<std::array<int, 3>, 8> k_CornerIsHigh = {{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}};

  for(usize octant = 0; octant < 8; ++octant)
  {
    const std::array<int64, 3> direction = CornerDirection(octant);
    // The cell spans from the source voxel toward the nearest corner.
    std::array<int64, 3> low{};
    std::array<int64, 3> high{};
    for(usize axis = 0; axis < 3; ++axis)
    {
      low[axis] = direction[axis] < 0 ? -1 : 0;
      high[axis] = direction[axis] < 0 ? 0 : 1;
    }

    const auto& offsets = ImageRotationUtilities::k_AllOctantOffsets[octant];
    for(usize corner = 0; corner < 8; ++corner)
    {
      for(usize axis = 0; axis < 3; ++axis)
      {
        const int64 expected = k_CornerIsHigh[corner][axis] == 1 ? high[axis] : low[axis];
        const std::string message = "octant " + std::to_string(octant) + " corner " + std::to_string(corner) + " axis " + std::to_string(axis);
        INFO(message);
        REQUIRE(offsets[corner][static_cast<int>(axis)] == expected);
      }
    }
  }
}

TEST_CASE("ImageRotationUtilities: Scaled three-cycle takes spacing from the scaled source axis", "[simplnx][ImageRotationUtilities][CreateRotationArgs]")
{
  DataStructure dataStructure;
  ImageGeom* image = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 3, 4});
  image->setSpacing({2.0F, 3.0F, 5.0F});
  image->setOrigin({1.0F, 2.0F, 3.0F});

  // The matrix maps (x,y,z) to (-4z, 2x, 3y). It is a three-cycle with a reflection and a different scale on each source axis.
  ImageRotationUtilities::Matrix4fR transform = ImageRotationUtilities::Matrix4fR::Identity();
  transform.block<3, 3>(0, 0) << 0.0F, 0.0F, -4.0F, 2.0F, 0.0F, 0.0F, 0.0F, 3.0F, 0.0F;

  // Hand calculation:
  // Output X comes from source Z: spacing 4 * 5 = 20, extent 4 * (4 * 5) = 80, so 4 cells.
  // Output Y comes from source X: spacing 2 * 2 = 4, extent 2 * (2 * 2) = 8, so 2 cells.
  // Output Z comes from source Y: spacing 3 * 3 = 9, extent 3 * (3 * 3) = 27, so 3 cells.
  // Minimum corner: X = -4 * (3 + 20) = -92, Y = 2 * 1 = 2, Z = 3 * 2 = 6.
  const ImageRotationUtilities::RotateArgs args = ImageRotationUtilities::CreateRotationArgs(*image, transform);
  const std::array<usize, 3> expectedDims = {4, 2, 3};
  const std::array<float32, 3> expectedSpacing = {20.0F, 4.0F, 9.0F};
  const std::array<float32, 3> expectedOrigin = {-92.0F, 2.0F, 6.0F};
  for(usize axis = 0; axis < 3; ++axis)
  {
    INFO("axis " + std::to_string(axis));
    CHECK(args.outputDims[axis] == expectedDims[axis]);
    CHECK(args.outputSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.TransformedSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.TransformedOrigin[axis] == Approx(expectedOrigin[axis]));
  }
}

TEST_CASE("ImageRotationUtilities: Uneven scaling keeps the classified source axis", "[simplnx][ImageRotationUtilities][CreateRotationArgs]")
{
  DataStructure dataStructure;
  ImageGeom* image = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 2, 2});
  image->setSpacing({2.0F, 3.0F, 5.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});

  ImageRotationUtilities::Matrix4fR transform = ImageRotationUtilities::Matrix4fR::Identity();
  transform(0, 1) = 5.0F;
  transform(1, 1) = 100000.0F;

  // Hand calculation: (x,y,z) -> (x+5y,100000y,z). The normalized Y column's
  // off-axis entry is about 5e-5, so the classified source axes remain X, Y, Z.
  // Spacing is (2, 3*sqrt(100000^2+5^2), 5), which rounds to (2,300000,5) in float32.
  // Input extents (4,6,10) become (4+5*6,100000*6,10) = (34,600000,10).
  // Dividing by spacing and rounding gives dimensions (17,2,2).
  const auto args = ImageRotationUtilities::CreateRotationArgs(*image, transform);
  const std::array<float32, 3> expectedSpacing = {2.0F, 300000.0F, 5.0F};
  const std::array<usize, 3> expectedDims = {17, 2, 2};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CAPTURE(axis);
    CHECK(args.outputSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.TransformedSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.outputDims[axis] == expectedDims[axis]);
    CHECK(args.TransformedDims[axis] == expectedDims[axis]);
  }
}

TEST_CASE("ImageRotationUtilities: Off-axis scaling keeps the column-based spacing", "[simplnx][ImageRotationUtilities][CreateRotationArgs]")
{
  DataStructure dataStructure;
  ImageGeom* image = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 3, 4});
  image->setSpacing({2.0F, 3.0F, 5.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});

  const float32 c = std::sqrt(0.5F);
  ImageRotationUtilities::Matrix4fR transform = ImageRotationUtilities::Matrix4fR::Identity();
  transform.block<3, 3>(0, 0) << 2 * c, -c, 0, 2 * c, c, 0, 0, 0, 1;

  // Each column selects spacing from its largest absolute direction cosine. A 45-degree tie
  // selects X. The result is multiplied by the column norm.
  // Column norms (2,1,1) therefore give spacing (2*2,2*1,5*1) = (4,2,5).
  // Source extents are (4,9,20). Rotated XY extents are (2*4+9)/sqrt(2).
  // nearbyint gives dimensions (round(17/(4*sqrt(2))),round(17/(2*sqrt(2))),4) = (3,6,4).
  const auto args = ImageRotationUtilities::CreateRotationArgs(*image, transform);
  const std::array<float32, 3> expectedSpacing = {4.0F, 2.0F, 5.0F};
  const std::array<usize, 3> expectedDims = {3, 6, 4};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CHECK(args.outputSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.outputDims[axis] == expectedDims[axis]);
  }
}

TEST_CASE("ImageRotationUtilities: Near-grid three-cycle accepted by Rotate Sample Reference Frame uses source-axis spacing", "[simplnx][ImageRotationUtilities][CreateRotationArgs]")
{
  DataStructure dataStructure;
  ImageGeom* image = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(image != nullptr);
  image->setDimensions({2, 3, 4});
  image->setSpacing({2.0F, 3.0F, 5.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});

  // 119.995 degrees about (111) differs from the exact three-cycle by about 5e-5 per entry.
  // Rotate Sample Reference Frame accepts it as a grid rotation (tolerance 1e-4).
  // The exact map is R(x,y,z) = (z,x,y). Output axes X, Y and Z take source axes Z, X and Y.
  // Spacing (2,3,5) becomes (5,2,3) and dimensions (2,3,4) become (4,2,3).
  const ImageRotationUtilities::Matrix4fR transform = ImageRotationUtilities::GenerateRotationTransformationMatrix({1.0F, 1.0F, 1.0F, 119.995F});
  const auto args = ImageRotationUtilities::CreateRotationArgs(*image, transform);
  const std::array<float32, 3> expectedSpacing = {5.0F, 2.0F, 3.0F};
  const std::array<usize, 3> expectedDims = {4, 2, 3};
  for(usize axis = 0; axis < 3; ++axis)
  {
    CHECK(args.outputSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.outputDims[axis] == expectedDims[axis]);
  }
}

TEST_CASE("ImageRotationUtilities: Raw grid tolerance accounts for column normalization", "[simplnx][ImageRotationUtilities][CreateRotationArgs]")
{
  DataStructure dataStructure;
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, "Input");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 4});
  imageGeomPtr->setSpacing({2.0F, 3.0F, 5.0F});
  imageGeomPtr->setOrigin({0.0F, 0.0F, 0.0F});

  ImageRotationUtilities::Matrix4fR transform = ImageRotationUtilities::Matrix4fR::Identity();
  transform.block<3, 3>(0, 0) << 0.000099998F, 0.0F, 1.0F, 0.99995F, 0.0F, 0.0F, 0.0F, 1.00005F, 0.0F;

  // Raw entries and determinant pass the filter's grid guard. Normalizing column 0 raises its small entry above 1e-4.
  REQUIRE(std::abs(transform(0, 0)) < 1.0e-4F);
  REQUIRE(std::abs(transform(1, 0) - 1.0F) < 1.0e-4F);
  REQUIRE(std::abs(transform(2, 1) - 1.0F) < 1.0e-4F);
  REQUIRE(std::abs(transform.block<3, 3>(0, 0).determinant() - 1.0F) < 1.0e-4F);
  REQUIRE(transform(0, 0) / transform.block<3, 1>(0, 0).norm() > 1.0e-4F);

  // Source axes Z, X, Y supply output spacing. Their scales are 1, sqrt(0.99995^2+0.000099998^2), 1.00005.
  // Output extents are 20+4*0.000099998, 4*0.99995, 9*1.00005. Rounding extent/spacing gives (4,2,3).
  const std::array<float64, 3> expectedSpacing = {5.0, 2.0 * std::sqrt(0.99995 * 0.99995 + 0.000099998 * 0.000099998), 3.0 * 1.00005};
  const std::array<usize, 3> expectedDims = {4, 2, 3};
  const auto args = ImageRotationUtilities::CreateRotationArgs(*imageGeomPtr, transform);
  for(usize axis = 0; axis < 3; ++axis)
  {
    CHECK(args.outputSpacing[axis] == Approx(expectedSpacing[axis]));
    CHECK(args.outputDims[axis] == expectedDims[axis]);
  }
}

TEST_CASE("ImageRotationUtilities: Scaled axis regression", "[simplnx][ImageRotationUtilities][NumericDomainRed]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6);
  CAPTURE(row);
  const std::vector<VectorFloat32Parameter::ValueType> inputs = {{0, 0, 1e20f, 90}, {1e-22f, 0, 0, 90}, {1e-40f, 0, 0, 90}, {3e38f, 3e38f, 3e38f, 120},
                                                                 {0, 0, 2, 90},     {1, 1, 1, 120},     {1, 1, 1, 36000}};
  auto matrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(inputs[row]);
  Eigen::Matrix3f expected;
  if(row == 0 || row == 4)
  {
    expected << 0, -1, 0, 1, 0, 0, 0, 0, 1;
  }
  else if(row == 1 || row == 2)
  {
    expected << 1, 0, 0, 0, 0, -1, 0, 1, 0;
  }
  else if(row == 6)
  {
    expected.setIdentity();
  }
  else
  {
    expected << 0, 0, 1, 1, 0, 0, 0, 1, 0;
  }
  Eigen::Matrix3f rotation = matrix.block<3, 3>(0, 0);
  CHECK((rotation - expected).cwiseAbs().maxCoeff() < (row == 6 ? 5e-5f : 1e-6f));
  CHECK((rotation * rotation.transpose() - Eigen::Matrix3f::Identity()).cwiseAbs().maxCoeff() < 1e-6f);
  CHECK(rotation.determinant() == Approx(1).margin(1e-6));
}
TEST_CASE("ImageRotationUtilities: Invalid matrix regression", "[simplnx][ImageRotationUtilities][NumericDomainRed]")
{
  const auto matrix = ImageRotationUtilities::GenerateRotationTransformationMatrix({0, 0, 0, 90});
  for(usize index = 0; index < 16; ++index)
  {
    CHECK(std::isnan(matrix.data()[index]));
  }
}

TEST_CASE("ImageRotationUtilities: Axis-angle input validation", "[simplnx][ImageRotationUtilities]")
{
  const auto row = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8);
  const float32 nan = std::numeric_limits<float32>::quiet_NaN(), inf = std::numeric_limits<float32>::infinity();
  const std::vector<VectorFloat32Parameter::ValueType> values = {{0, 0, 1},      {nan, 0, 1, 90}, {0, inf, 1, 90},  {0, 0, -inf, 90}, {0, 0, 0, 90},
                                                                 {0, 0, 1, nan}, {0, 0, 1, inf},  {nan, 0, 1, nan}, {0, 0, 1, 90}};
  auto result = ImageRotationUtilities::ValidateRotationAxisAngle(values[row], "Rotation");
  const std::array<int32, 9> codes = {-86050, -86051, -86051, -86051, -86052, -86053, -86053, -86051, 0};
  if(row == 8)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors()[0].code == codes[row]);
    CHECK(result.errors()[0].message.find("Rotation") != std::string::npos);
  }
  if(row != 8)
  {
    auto matrix = ImageRotationUtilities::GenerateRotationTransformationMatrix(values[row]);
    for(usize idx = 0; idx < 16; ++idx)
    {
      CHECK(std::isnan(matrix.data()[idx]));
    }
  }
}
TEST_CASE("ImageRotationUtilities: Ordinary angle compatibility", "[simplnx][ImageRotationUtilities]")
{
  const auto matrix = ImageRotationUtilities::GenerateRotationTransformationMatrix({1, 1, 1, 36000});
  const float32 radians = 36000.0f * Constants::k_PiOver180F;
  const float32 cosine = cos(radians), sine = sin(radians), unit = FloatVec3{1, 1, 1}.normalize()[0];
  ImageRotationUtilities::Matrix4fR previous = ImageRotationUtilities::Matrix4fR::Identity();
  for(usize row = 0; row < 3; ++row)
  {
    for(usize col = 0; col < 3; ++col)
    {
      previous(row, col) = unit * unit * (1 - cosine) + (row == col ? cosine : ((row + 1) % 3 == col ? -unit * sine : unit * sine));
    }
  }
  CHECK((matrix - previous).cwiseAbs().maxCoeff() < 2e-6f);
}
