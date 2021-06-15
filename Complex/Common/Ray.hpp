#pragma once

#include "Complex/Common/EulerAngle.h"
#include "Complex/Common/Point3D.hpp"

namespace Complex
{

/**
 * class Ray
 *
 */

template <typename T>
class Ray
{
public:
  using PointType = Point3D<T>;
  using ZXZEulerType = ZXZEuler<T>;
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
  Ray(const PointType& origin, const ZXZEulerType& ang, LengthType length)
  : m_Origin(origin)
  , m_Angle(ang)
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
  void setOrigin(const PointType& origin)
  {
    m_Origin = origin;
  }

  /**
   * @brief Sets a new Euler angle.
   * @param ang
   */
  void setEuler(const ZXZEulerType& ang)
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
  void setLength(LengthType length)
  {
    m_Length = length;
  }

  /**
   * @brief Returns the end point determined by the origin point, Euler angle, and length.
   * @return PointType
   */
  PointType getEndPoint() const
  {
    throw std::exception();
  }

  /**
   * @brief Returns a Point3D at the specified length from the origin point.
   * @param length
   * @return PointType
   */
  PointType getPointAtDist(LengthType length) const
  {
    throw std::exception();
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
private:
  PointType m_Origin;
  ZXZEulerType m_Angle;
  LengthType m_Length;
};
} // namespace Complex
