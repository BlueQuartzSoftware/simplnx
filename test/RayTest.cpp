#include "simplnx/Common/Ray.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

#include <catch2/catch.hpp>

#include <array>

using namespace nx::core;

namespace
{
void CheckEndpoint(const Point3D<float64>& actual, const std::array<float64, 3>& expected)
{
  for(usize axis = 0; axis < 3; axis++)
  {
    CAPTURE(axis);
    CHECK(actual[axis] == Approx(expected[axis]).margin(1.0e-6));
  }
}

template <typename RayT>
void CheckRotatedDiagonal()
{
  RayT ray(Point3D<float64>{0, 0, 0}, ZXZEuler{0, 0, 0}, 1.0);
  // The identity rotation leaves (L,L,L)=(1,1,1); its squared displacement is 3.
  CheckEndpoint(ray.getEndPoint(), {1, 1, 1});
  const auto endpoint = ray.getEndPoint();
  CHECK(endpoint.dot(endpoint) == Approx(3.0));
  // At x=1/2 the segment crosses (1/2,1/2,1/2), strictly inside this triangle.
  CHECK(GeometryMath::RayIntersectsTriangle(ray, Point3D<float64>{0.5, 0, 0}, Point3D<float64>{0.5, 2, 0}, Point3D<float64>{0.5, 0, 2}) == 'f');

  // Compose Rz(pi/3)*Rx(pi/2)*Rz(pi/6) on (1,1,1), independently of Ray's basis expansion.
  // Rz(pi/6) gives ((sqrt(3)-1)/2,(1+sqrt(3))/2,1); Rx(pi/2) gives ((sqrt(3)-1)/2,-1,(1+sqrt(3))/2).
  // The final vector is ((3*sqrt(3)-1)/4,(1-sqrt(3))/4,(1+sqrt(3))/2).
  const ZXZEuler angles{1.0471975511965977F, 1.5707963267948966F, 0.5235987755982988F};
  RayT rotated(Point3D<float64>{0, 0, 0}, angles, 1.0);
  CheckEndpoint(rotated.getEndPoint(), {1.049038105676658, -0.1830127018922193, 1.3660254037844386});
  ray.setEuler(angles);
  CheckEndpoint(ray.getEndPoint(), {1.049038105676658, -0.1830127018922193, 1.3660254037844386});
  ray.setLength(2.0);
  CheckEndpoint(ray.getEndPoint(), {2.098076211353316, -0.3660254037844386, 2.7320508075688772});
  ray.setOrigin(Point3D<float64>{10, 20, 30});
  CheckEndpoint(ray.getEndPoint(), {12.098076211353316, 19.633974596215562, 32.732050807568877});
}
} // namespace

TEST_CASE("simplnx::Ray: ZXZ endpoint preserves the rotated diagonal", "[simplnx][Ray]")
{
  SECTION("Ray")
  {
    CheckRotatedDiagonal<Ray<float64>>();
  }
  SECTION("CachedRay")
  {
    CheckRotatedDiagonal<CachedRay<float64>>();
    CachedRay<float64> cached(Point3D<float64>{0, 0, 0}, ZXZEuler{0, 0, 0}, 1.0);
    CheckEndpoint(cached.getEndPointRef(), {1, 1, 1});
    cached.setOrigin(Point3D<float64>{2, 3, 5});
    CheckEndpoint(cached.getEndPointRef(), {3, 4, 6});
  }
}
