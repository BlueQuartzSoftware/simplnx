#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"

#include <catch2/catch.hpp>

#include <cmath>
#include <sstream>

using namespace nx::core;

TEST_CASE("Vec3 Constructors")
{
  SECTION("Default constructor initializes to zero")
  {
    Vec3<float32> v;
    REQUIRE(v[0] == 0.0f);
    REQUIRE(v[1] == 0.0f);
    REQUIRE(v[2] == 0.0f);
  }

  SECTION("Three-argument constructor")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
  }

  SECTION("std::array constructor")
  {
    std::array<float32, 3> data = {4.0f, 5.0f, 6.0f};
    Vec3<float32> v(data);
    REQUIRE(v[0] == 4.0f);
    REQUIRE(v[1] == 5.0f);
    REQUIRE(v[2] == 6.0f);
  }

  SECTION("std::tuple constructor")
  {
    auto t = std::make_tuple(7.0f, 8.0f, 9.0f);
    Vec3<float32> v(t);
    REQUIRE(v[0] == 7.0f);
    REQUIRE(v[1] == 8.0f);
    REQUIRE(v[2] == 9.0f);
  }

  SECTION("Pointer constructor")
  {
    float32 data[] = {10.0f, 11.0f, 12.0f};
    Vec3<float32> v(data);
    REQUIRE(v[0] == 10.0f);
    REQUIRE(v[1] == 11.0f);
    REQUIRE(v[2] == 12.0f);
  }

  SECTION("std::vector constructor")
  {
    std::vector<float32> data = {13.0f, 14.0f, 15.0f};
    Vec3<float32> v(data);
    REQUIRE(v[0] == 13.0f);
    REQUIRE(v[1] == 14.0f);
    REQUIRE(v[2] == 15.0f);
  }

  SECTION("Copy constructor")
  {
    Vec3<float32> original(1.0f, 2.0f, 3.0f);
    Vec3<float32> copy(original);
    REQUIRE(copy[0] == 1.0f);
    REQUIRE(copy[1] == 2.0f);
    REQUIRE(copy[2] == 3.0f);
  }

  SECTION("Move constructor")
  {
    Vec3<float32> original(1.0f, 2.0f, 3.0f);
    Vec3<float32> moved(std::move(original));
    REQUIRE(moved[0] == 1.0f);
    REQUIRE(moved[1] == 2.0f);
    REQUIRE(moved[2] == 3.0f);
  }
}

TEST_CASE("Vec3 Assignment Operators")
{
  SECTION("Copy assignment")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b;
    b = a;
    REQUIRE(b[0] == 1.0f);
    REQUIRE(b[1] == 2.0f);
    REQUIRE(b[2] == 3.0f);
  }

  SECTION("Move assignment")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b;
    b = std::move(a);
    REQUIRE(b[0] == 1.0f);
    REQUIRE(b[1] == 2.0f);
    REQUIRE(b[2] == 3.0f);
  }
}

TEST_CASE("Vec3 Accessors")
{
  Vec3<float32> v(1.0f, 2.0f, 3.0f);

  SECTION("getX/getY/getZ")
  {
    REQUIRE(v.getX() == 1.0f);
    REQUIRE(v.getY() == 2.0f);
    REQUIRE(v.getZ() == 3.0f);
  }

  SECTION("setX/setY/setZ")
  {
    v.setX(10.0f);
    v.setY(20.0f);
    v.setZ(30.0f);
    REQUIRE(v.getX() == 10.0f);
    REQUIRE(v.getY() == 20.0f);
    REQUIRE(v.getZ() == 30.0f);
  }

  SECTION("setValues")
  {
    v.setValues(100.0f, 200.0f, 300.0f);
    REQUIRE(v[0] == 100.0f);
    REQUIRE(v[1] == 200.0f);
    REQUIRE(v[2] == 300.0f);
  }

  SECTION("operator[] non-const")
  {
    v[0] = 42.0f;
    REQUIRE(v[0] == 42.0f);
  }

  SECTION("at() bounds-checked access")
  {
    REQUIRE(v.at(0) == 1.0f);
    REQUIRE(v.at(1) == 2.0f);
    REQUIRE(v.at(2) == 3.0f);
  }

  SECTION("data() pointer access")
  {
    float32* ptr = v.data();
    REQUIRE(ptr[0] == 1.0f);
    REQUIRE(ptr[1] == 2.0f);
    REQUIRE(ptr[2] == 3.0f);
  }

  SECTION("size()")
  {
    REQUIRE(v.size() == 3);
  }
}

TEST_CASE("Vec3 Conversion")
{
  SECTION("toTuple")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    auto t = v.toTuple();
    REQUIRE(std::get<0>(t) == 1.0f);
    REQUIRE(std::get<1>(t) == 2.0f);
    REQUIRE(std::get<2>(t) == 3.0f);
  }

  SECTION("toArray")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    auto arr = v.toArray();
    REQUIRE(arr[0] == 1.0f);
    REQUIRE(arr[1] == 2.0f);
    REQUIRE(arr[2] == 3.0f);
  }

  SECTION("convertType float to int")
  {
    Vec3<float32> v(1.5f, 2.7f, 3.9f);
    auto vi = v.convertType<int32>();
    REQUIRE(vi[0] == 1);
    REQUIRE(vi[1] == 2);
    REQUIRE(vi[2] == 3);
  }

  SECTION("convertType int to float")
  {
    Vec3<int32> v(1, 2, 3);
    auto vf = v.convertType<float32>();
    REQUIRE(vf[0] == 1.0f);
    REQUIRE(vf[1] == 2.0f);
    REQUIRE(vf[2] == 3.0f);
  }

  SECTION("toContainer")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    auto vec = v.toContainer<std::vector<float32>>();
    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == 1.0f);
    REQUIRE(vec[1] == 2.0f);
    REQUIRE(vec[2] == 3.0f);
  }
}

TEST_CASE("Vec3 Comparison Operators")
{
  SECTION("operator==")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b(1.0f, 2.0f, 3.0f);
    REQUIRE(a == b);
  }

  SECTION("operator!=")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b(4.0f, 5.0f, 6.0f);
    REQUIRE(a != b);
  }

  SECTION("operator<")
  {
    Vec3<float32> a(1.0f, 0.0f, 0.0f);
    Vec3<float32> b(2.0f, 0.0f, 0.0f);
    REQUIRE(a < b);
  }
}

TEST_CASE("Vec3 Arithmetic Operators")
{
  Vec3<float32> a(1.0f, 2.0f, 3.0f);
  Vec3<float32> b(4.0f, 5.0f, 6.0f);

  SECTION("operator+ (vector)")
  {
    auto c = a + b;
    REQUIRE(c[0] == 5.0f);
    REQUIRE(c[1] == 7.0f);
    REQUIRE(c[2] == 9.0f);
  }

  SECTION("operator+ (scalar)")
  {
    auto c = a + 10.0f;
    REQUIRE(c[0] == 11.0f);
    REQUIRE(c[1] == 12.0f);
    REQUIRE(c[2] == 13.0f);
  }

  SECTION("operator- (vector)")
  {
    auto c = a - b;
    REQUIRE(c[0] == -3.0f);
    REQUIRE(c[1] == -3.0f);
    REQUIRE(c[2] == -3.0f);
  }

  SECTION("unary operator-")
  {
    auto c = -a;
    REQUIRE(c[0] == -1.0f);
    REQUIRE(c[1] == -2.0f);
    REQUIRE(c[2] == -3.0f);
  }

  SECTION("operator* (scalar)")
  {
    auto c = a * 2.0f;
    REQUIRE(c[0] == 2.0f);
    REQUIRE(c[1] == 4.0f);
    REQUIRE(c[2] == 6.0f);
  }

  SECTION("operator/ (scalar)")
  {
    auto c = a / 2.0f;
    REQUIRE(c[0] == Approx(0.5f));
    REQUIRE(c[1] == Approx(1.0f));
    REQUIRE(c[2] == Approx(1.5f));
  }

  SECTION("operator*= (scalar)")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    v *= 3.0f;
    REQUIRE(v[0] == 3.0f);
    REQUIRE(v[1] == 6.0f);
    REQUIRE(v[2] == 9.0f);
  }

  SECTION("operator/= (scalar)")
  {
    Vec3<float32> v(6.0f, 8.0f, 10.0f);
    v /= 2.0f;
    REQUIRE(v[0] == 3.0f);
    REQUIRE(v[1] == 4.0f);
    REQUIRE(v[2] == 5.0f);
  }

  SECTION("free operator* (scalar * vec)")
  {
    auto c = 2.0f * a;
    REQUIRE(c[0] == 2.0f);
    REQUIRE(c[1] == 4.0f);
    REQUIRE(c[2] == 6.0f);
  }

  SECTION("free operator* (mixed types: int * float vec)")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    auto c = 2 * v;
    REQUIRE(c[0] == Approx(2.0f));
    REQUIRE(c[1] == Approx(4.0f));
    REQUIRE(c[2] == Approx(6.0f));
  }
}

TEST_CASE("Vec3 Linear Algebra")
{
  SECTION("dot product (two-arg)")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b(4.0f, 5.0f, 6.0f);
    float32 result = a.dot(b);
    REQUIRE(result == Approx(32.0f)); // 1*4 + 2*5 + 3*6
  }

  SECTION("dot product (no-arg, dot with self)")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    float32 result = a.dot();
    REQUIRE(result == Approx(14.0f)); // 1*1 + 2*2 + 3*3
  }

  SECTION("sumOfSquares")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    REQUIRE(a.sumOfSquares() == Approx(14.0f));
  }

  SECTION("cross product")
  {
    Vec3<float32> a(1.0f, 0.0f, 0.0f);
    Vec3<float32> b(0.0f, 1.0f, 0.0f);
    auto c = a.cross(b);
    REQUIRE(c[0] == Approx(0.0f));
    REQUIRE(c[1] == Approx(0.0f));
    REQUIRE(c[2] == Approx(1.0f));
  }

  SECTION("cross product general")
  {
    Vec3<float32> a(1.0f, 2.0f, 3.0f);
    Vec3<float32> b(4.0f, 5.0f, 6.0f);
    auto c = a.cross(b);
    REQUIRE(c[0] == Approx(-3.0f)); // 2*6 - 3*5
    REQUIRE(c[1] == Approx(6.0f));  // 3*4 - 1*6
    REQUIRE(c[2] == Approx(-3.0f)); // 1*5 - 2*4
  }

  SECTION("magnitude")
  {
    Vec3<float32> v(3.0f, 4.0f, 0.0f);
    REQUIRE(v.magnitude() == Approx(5.0f));
  }

  SECTION("magnitude unit vector")
  {
    Vec3<float32> v(1.0f, 0.0f, 0.0f);
    REQUIRE(v.magnitude() == Approx(1.0f));
  }

  SECTION("normalize")
  {
    Vec3<float32> v(3.0f, 0.0f, 0.0f);
    auto n = v.normalize();
    REQUIRE(n[0] == Approx(1.0f));
    REQUIRE(n[1] == Approx(0.0f));
    REQUIRE(n[2] == Approx(0.0f));
  }

  SECTION("normalize general")
  {
    Vec3<float32> v(1.0f, 1.0f, 1.0f);
    auto n = v.normalize();
    float32 expected = 1.0f / std::sqrt(3.0f);
    REQUIRE(n[0] == Approx(expected));
    REQUIRE(n[1] == Approx(expected));
    REQUIRE(n[2] == Approx(expected));
    REQUIRE(n.magnitude() == Approx(1.0f));
  }

  SECTION("cosTheta parallel vectors")
  {
    Vec3<float32> a(1.0f, 0.0f, 0.0f);
    Vec3<float32> b(2.0f, 0.0f, 0.0f);
    REQUIRE(a.cosTheta(b) == Approx(1.0f));
  }

  SECTION("cosTheta perpendicular vectors")
  {
    Vec3<float32> a(1.0f, 0.0f, 0.0f);
    Vec3<float32> b(0.0f, 1.0f, 0.0f);
    REQUIRE(a.cosTheta(b) == Approx(0.0f));
  }

  SECTION("cosTheta antiparallel vectors")
  {
    Vec3<float32> a(1.0f, 0.0f, 0.0f);
    Vec3<float32> b(-1.0f, 0.0f, 0.0f);
    REQUIRE(a.cosTheta(b) == Approx(-1.0f));
  }

  SECTION("cosTheta zero vector returns 1")
  {
    Vec3<float32> a(0.0f, 0.0f, 0.0f);
    Vec3<float32> b(1.0f, 0.0f, 0.0f);
    REQUIRE(a.cosTheta(b) == Approx(1.0f));
  }
}

TEST_CASE("Vec3 Element-wise Operations")
{
  SECTION("abs")
  {
    Vec3<float32> v(-1.0f, -2.0f, 3.0f);
    auto a = v.abs();
    REQUIRE(a[0] == 1.0f);
    REQUIRE(a[1] == 2.0f);
    REQUIRE(a[2] == 3.0f);
  }

  SECTION("abs all positive")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    auto a = v.abs();
    REQUIRE(a[0] == 1.0f);
    REQUIRE(a[1] == 2.0f);
    REQUIRE(a[2] == 3.0f);
  }

  SECTION("maxValueIndex - first element largest")
  {
    Vec3<float32> v(10.0f, 5.0f, 3.0f);
    REQUIRE(v.maxValueIndex() == 0);
  }

  SECTION("maxValueIndex - second element largest")
  {
    Vec3<float32> v(1.0f, 10.0f, 3.0f);
    REQUIRE(v.maxValueIndex() == 1);
  }

  SECTION("maxValueIndex - third element largest")
  {
    Vec3<float32> v(1.0f, 2.0f, 10.0f);
    REQUIRE(v.maxValueIndex() == 2);
  }

  SECTION("maxValueIndex uses absolute value")
  {
    Vec3<float32> v(-10.0f, 5.0f, 3.0f);
    REQUIRE(v.maxValueIndex() == 0);
  }
}

TEST_CASE("Vec3 copyInto")
{
  SECTION("copyInto same type")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    float32 dest[3] = {0.0f, 0.0f, 0.0f};
    v.copyInto(dest);
    REQUIRE(dest[0] == 1.0f);
    REQUIRE(dest[1] == 2.0f);
    REQUIRE(dest[2] == 3.0f);
  }

  SECTION("copyInto with type conversion")
  {
    Vec3<float32> v(1.5f, 2.7f, 3.9f);
    int32 dest[3] = {0, 0, 0};
    v.copyInto(dest);
    REQUIRE(dest[0] == 1);
    REQUIRE(dest[1] == 2);
    REQUIRE(dest[2] == 3);
  }

  SECTION("copyInto float64")
  {
    Vec3<float32> v(1.0f, 2.0f, 3.0f);
    float64 dest[3] = {0.0, 0.0, 0.0};
    v.copyInto(dest);
    REQUIRE(dest[0] == Approx(1.0));
    REQUIRE(dest[1] == Approx(2.0));
    REQUIRE(dest[2] == Approx(3.0));
  }
}

TEST_CASE("Vec3 Iterator Support")
{
  Vec3<float32> v(1.0f, 2.0f, 3.0f);

  SECTION("range-based for loop")
  {
    float32 sum = 0.0f;
    for(auto val : v)
    {
      sum += val;
    }
    REQUIRE(sum == Approx(6.0f));
  }

  SECTION("begin/end")
  {
    auto it = v.begin();
    REQUIRE(*it == 1.0f);
    ++it;
    REQUIRE(*it == 2.0f);
    ++it;
    REQUIRE(*it == 3.0f);
    ++it;
    REQUIRE(it == v.end());
  }
}

TEST_CASE("Vec3 Stream Output")
{
  Vec3<float32> v(1.0f, 2.0f, 3.0f);
  std::ostringstream oss;
  oss << v;
  std::string result = oss.str();
  REQUIRE(result.find("1") != std::string::npos);
  REQUIRE(result.find("2") != std::string::npos);
  REQUIRE(result.find("3") != std::string::npos);
}

TEST_CASE("Vec3 Type Aliases")
{
  SECTION("FloatVec3")
  {
    FloatVec3 v(1.0f, 2.0f, 3.0f);
    REQUIRE(v[0] == 1.0f);
  }

  SECTION("Float64Vec3")
  {
    Float64Vec3 v(1.0, 2.0, 3.0);
    REQUIRE(v[0] == 1.0);
  }

  SECTION("IntVec3")
  {
    IntVec3 v(1, 2, 3);
    REQUIRE(v[0] == 1);
  }

  SECTION("SizeVec3")
  {
    SizeVec3 v(1, 2, 3);
    REQUIRE(v[0] == 1);
  }

  SECTION("Point3D alias")
  {
    Point3D<float32> v(1.0f, 2.0f, 3.0f);
    REQUIRE(v[0] == 1.0f);
  }

  SECTION("Matrix3X1 alias")
  {
    Matrix3X1<float32> v(1.0f, 2.0f, 3.0f);
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
  }

  SECTION("Matrix3X1F alias")
  {
    Matrix3X1F v(1.0f, 2.0f, 3.0f);
    REQUIRE(v[0] == 1.0f);
  }

  SECTION("Matrix3X1D alias")
  {
    Matrix3X1D v(1.0, 2.0, 3.0);
    REQUIRE(v[0] == 1.0);
  }
}

TEST_CASE("Vec3 Integer Type")
{
  SECTION("Integer arithmetic")
  {
    Vec3<int32> a(1, 2, 3);
    Vec3<int32> b(4, 5, 6);
    auto c = a + b;
    REQUIRE(c[0] == 5);
    REQUIRE(c[1] == 7);
    REQUIRE(c[2] == 9);
  }

  SECTION("Integer dot product")
  {
    Vec3<int32> a(1, 2, 3);
    Vec3<int32> b(4, 5, 6);
    REQUIRE(a.dot(b) == 32);
  }

  SECTION("Integer cross product")
  {
    Vec3<int32> a(1, 0, 0);
    Vec3<int32> b(0, 1, 0);
    auto c = a.cross(b);
    REQUIRE(c[0] == 0);
    REQUIRE(c[1] == 0);
    REQUIRE(c[2] == 1);
  }
}
