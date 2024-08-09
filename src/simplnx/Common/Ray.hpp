#pragma once

#include <stdexcept>
#include <utility>

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/EulerAngle.hpp"

namespace nx::core
{
/**
 * @class Ray
 * @brief The Ray class describes a ray or line segment in 3D space. Using
 * the origin and angle values, points can be found at any specified length or
 * the endpoint can be found at the current length. Rays are primarily used to
 * simplify and better describe values used in GeometryMath. The rays initial alignment
 * is assumed to be with the Y-axis.
 */
template <typename T>
class Ray
{
public:
  using PointType = Point3D<T>;
  using ZXZEulerType = ZXZEuler;
  using LengthType = T;

  /**
   * @brief Default constructor. Creates a ray starting at (0, 0, 0) with no
   * rotation and a length of 0.
   */
  Ray()
  : m_Origin()
  , m_Angle()
  , m_Length(0)
  {
  }

  /**
   * @brief Creates a Ray from an origin point, ZXZEuler angle, and target length.
   * @param origin
   * @param ang
   * @param length
   */
  Ray(const PointType& origin, ZXZEulerType ang, LengthType length)
  : m_Origin(origin)
  , m_Angle(std::move(ang))
  , m_Length(length)
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  Ray(const Ray& other)
  : m_Origin(other.m_Origin)
  , m_Angle(other.m_Angle)
  , m_Length(other.m_Length)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Ray(Ray&& other) noexcept
  : m_Origin(std::move(other.m_Origin))
  , m_Angle(std::move(other.m_Angle))
  , m_Length(std::move(other.m_Length))
  {
  }

  ~Ray() = default;

  /**
   * @brief Returns the origin point.
   * @return PointType
   */
  PointType getOrigin() const
  {
    return m_Origin;
  }

  /**
   * @brief Returns the origin point.
   * @return const PointType&
   */
  const PointType& getOriginRef() const
  {
    return m_Origin;
  }

  /**
   * @brief Returns the Euler angle describing the Ray's direction.
   * @return ZXZEulerType
   */
  ZXZEulerType getEuler() const
  {
    return m_Angle;
  }

  /**
   * @brief Sets a new origin point.
   * @param origin
   */
  virtual void setOrigin(const PointType& origin)
  {
    m_Origin = origin;
  }

  /**
   * @brief Sets a new Euler angle.
   * @param ang
   */
  virtual void setEuler(const ZXZEulerType& ang)
  {
    m_Angle = ang;
  }

  /**
   * @brief Returns the current ray length.
   * @return LengthType
   */
  LengthType getLength() const
  {
    return m_Length;
  }

  /**
   * @brief Sets a new ray length.
   * @param length
   */
  virtual void setLength(LengthType length)
  {
    m_Length = length;
  }

  /**
   * @brief Returns the end point determined by the origin point, Euler angle, and length.
   * Based on the assumption the point is initially aligned with the global axis' and the that this vector specifically aligned with the local y-axis.
   * @return PointType
   */
  virtual PointType getEndPoint() const
  {
    return calculateEndpoint();
  }

  /**
   * @brief Returns a Point3D at the specified length from the origin point.
   * @param length
   * @return PointType
   */
  PointType getPointAtDist(LengthType length) const
  {
    throw std::runtime_error("");
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Ray&
   */
  Ray& operator=(const Ray& rhs)
  {
    m_Origin = rhs.m_Origin;
    m_Angle = rhs.m_Angle;
    m_Length = rhs.m_Length;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Ray&
   */
  Ray& operator=(Ray&& rhs) noexcept
  {
    m_Origin = std::move(rhs.m_Origin);
    m_Angle = std::move(rhs.m_Angle);
    m_Length = std::move(rhs.m_Length);
    return *this;
  }

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(const Ray& rhs) const
  {
    bool origin = (rhs.m_Origin == m_Origin);
    bool angle = (rhs.m_Angle == m_Angle);
    bool length = (rhs.m_Length == m_Length);
    return origin && angle && length;
  }

  /**
   * @brief Inequality operator
   * @param rhs
   * @return bool
   */
  bool operator!=(const Ray& rhs) const
  {
    bool origin = (rhs.m_Origin == m_Origin);
    bool angle = (rhs.m_Angle == m_Angle);
    bool length = (rhs.m_Length == m_Length);
    return !(origin && angle && length);
  }

protected:
  PointType m_Origin;
  ZXZEulerType m_Angle;
  LengthType m_Length;

  PointType calculateEndpoint() const
  {
    const auto sin1 = std::sin(m_Angle[0]);
    const auto sin2 = std::sin(m_Angle[1]);
    const auto sin3 = std::sin(m_Angle[2]);

    const auto cos1 = std::cos(m_Angle[0]);
    const auto cos2 = std::cos(m_Angle[1]);
    const auto cos3 = std::cos(m_Angle[2]);

    // Reference: https://ntrs.nasa.gov/api/citations/19770019231/downloads/19770019231.pdf Page:23
    Vec3<T> localXRotationVec((-sin1 * cos2 * sin3) + (cos1 * cos3), (cos1 * cos2 * sin3) + (sin1 * cos3), sin2 * sin3);
    Vec3<T> localYRotationVec((-sin1 * cos2 * cos3) - (cos1 * cos3), (cos1 * cos2 * cos3) - (sin1 * sin3), sin2 * cos3);
    Vec3<T> localZRotationVec((sin1 * sin2), -cos1 * sin2, cos2);

    return m_Origin + (localXRotationVec * m_Length) + (localYRotationVec * m_Length) + (localZRotationVec * m_Length);
  }
};

template <typename T>
class CachedRay : public Ray<T>
{
public:
  /**
   * @brief Creates a Ray from an origin point, ZXZEuler angle, and target length.
   * @param origin
   * @param ang
   * @param length
   */
  CachedRay(const typename Ray<T>::PointType& origin, typename Ray<T>::ZXZEulerType ang, typename Ray<T>::LengthType length)
  : Ray<T>(origin, std::move(ang), length)
  {
    m_Endpoint = this->calculateEndpoint();
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  CachedRay(const CachedRay& other)
  : m_Endpoint(other.m_Endpoint)
  , Ray<T>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  CachedRay(CachedRay&& other) noexcept
  : m_Endpoint(std::move(other.m_Endpoint))
  , Ray<T>(std::move(other))
  {
  }

  ~CachedRay() = default;

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return CachedRay&
   */
  CachedRay& operator=(const CachedRay& rhs)
  {
    this->m_Origin = rhs.m_Origin;
    this->m_Angle = rhs.m_Angle;
    this->m_Length = rhs.m_Length;
    m_Endpoint = rhs.m_Endpoint;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Ray&
   */
  CachedRay& operator=(CachedRay&& rhs) noexcept
  {
    this->m_Origin = std::move(rhs.m_Origin);
    this->m_Angle = std::move(rhs.m_Angle);
    this->m_Length = std::move(rhs.m_Length);
    m_Endpoint = std::move(rhs.m_Endpoint);
    return *this;
  }

  void setOrigin(const typename Ray<T>::PointType& origin) override
  {
    this->m_Origin = origin;
    m_Endpoint = this->calculateEndpoint();
  }

  void setLength(typename Ray<T>::LengthType length) override
  {
    this->m_Length = length;
    m_Endpoint = this->calculateEndpoint();
  }

  void setEuler(const typename Ray<T>::ZXZEulerType& ang) override
  {
    this->m_Angle = ang;
    m_Endpoint = this->calculateEndpoint();
  }

  const typename Ray<T>::PointType& getEndPointRef() const
  {
    return m_Endpoint;
  }

  typename Ray<T>::PointType getEndPoint() const override
  {
    return m_Endpoint;
  }

private:
  // Optional caching for speed
  typename Ray<T>::PointType m_Endpoint = {};
};
} // namespace nx::core
