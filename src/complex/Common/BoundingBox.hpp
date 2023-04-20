#pragma once

#include <array>

#include "complex/Common/Array.hpp"

namespace complex
{
/**
 * @class BoundingBox
 * @brief The BoundingBox class is designed to describe a box in 3D/2D space
 * within which all points of interest are contained. This is primarily
 * used to describe the getSize of a geometry but is also used by GeometryMath
 * for calculations checking for points in a given region. As the BoundingBox
 * class operates along X, Y, and Z axis (in 3D version), no rotation information is available.
 *
 * Template Type: Point - Point is a parameter for the type of (complex) Point used
 * by the BoundingBox based on the templated value type.
 */
template <typename T, template <typename> class Point = Point3D>
class BoundingBox
{
public:
  //========================================= STL INTERFACE COMPATIBILITY =================================
  using size_type = usize;
  using reference = T&;
  using const_reference = const T&;

  using PointType = Point<T>;

  /**
   * @brief ValueType is an alias for the value type describing bounds along
   * each axis. The ValueType describes the type of Point3D, std::arrays, or
   * pointers used for BoundingBox.
   */
  using ValueType = T;

  /**
   * @brief Pointer is an alias for the pointer arrays used by the BoundingBox.
   */
  using Pointer = T*;

  /**
   * @brief Constructs a new BoundingBox defined by two corner positions.
   * The points are assumed to be positioned such that the lowerLeft point
   * is less than or equal to upperRight values along each dimension.
   * @param lowerLeft
   * @param upperRight
   */
  BoundingBox(PointType lowerLeft, PointType upperRight)
  : m_Lower(lowerLeft)
  , m_Upper(upperRight)
  {
  }

  /**
   * @brief Constructs a new BoundingBox3D defined by the array of position values.
   * The format is min X, min Y, min Z, max X, max Y, max Z.
   * @param arr
   */
  template <class PointT = PointType, class = std::enable_if_t<std::is_same<PointT,Point3D<T>>::value>>
  explicit BoundingBox(const std::array<T, 6>& arr)
  : m_Lower(Point3D<T>(arr[0], arr[1], arr[2]))
  , m_Upper(Point3D<T>(arr[3], arr[4], arr[5]))
  {
  }

  /**
   * @brief Constructs a new BoundingBox2D defined by the array of position values.
   * The format is min X, min Y, max X, max Y.
   * @param arr
   */
  template <class PointT = PointType, class = std::enable_if_t<std::is_same<PointT,Point2D<T>>::value>>
  explicit BoundingBox(const std::array<T, 4>& arr)
  : m_Lower(Point2D<T>(arr[0], arr[1]))
  , m_Upper(Point2D<T>(arr[2], arr[3]))
  {
  }

  /**
   * @brief Constructs a new BoundingBox by copying values from the specified pointer.
   * The pointer is assumed to contain either 4 (2D) or 6 (3D) values formatted the same as the
   * std::array<T, 6> constructor, min X, min Y, max X, max Y or the
   * std::array<T, 6> constructor, min X, min Y, min Z, max X, max Y, max Z
   * respectively
   * @param arr
   */
  explicit BoundingBox(const Pointer arr)
  : m_Lower(Point3D<T>(arr[0], arr[1], arr[2]))
  , m_Upper(Point3D<T>(arr[3], arr[4], arr[5]))
  {
  }

  ~BoundingBox() = default;

  /**
   * @brief Returns the Min point
   * @return
   */
  const PointType& getMinPoint() const
  {
    return m_Lower;
  }

  /**
   * @brief Returns the Max point
   * @return
   */
  const PointType& getMaxPoint() const
  {
    return m_Upper;
  }

  /**
   * @brief Sets the Min point
   * @return
   */
  void setMinPoint(PointType min)
  {
    m_Lower = min;
  }

  /**
   * @brief Sets the Max point
   * @return
   */
  void setMaxPoint(PointType max)
  {
    m_Upper = max;
  }

  /**
   * @brief Compute the length of each side of the bounding box
   * @return
   */
  PointType sideLengths() const
  {
    return {m_Upper - m_Lower};
  }

  /**
   * @brief compute the center of the bounding box.
   * @return
   */
  PointType center() const
  {
    PointType boxSize = sideLengths();
    if constexpr(std::is_same_v<Point<ValueType>, Point3D<ValueType>>)
    {
      return {m_Lower[0] + boxSize[0] / 2.0, m_Lower[1] + boxSize[1] / 2.0, m_Lower[2] + boxSize[2] / 2.0};
    }
    if constexpr(std::is_same_v<Point<ValueType>, Point2D<ValueType>>)
    {
      return {m_Lower[0] + boxSize[0] / 2.0, m_Lower[1] + boxSize[1] / 2.0};
    }
  }

  /**
   * @brief Checks if the bounding box is valid. A valid box has a non-negative
   * difference between all minimum and maximum values.
   * @return bool
   */
  bool isValid() const
  {
    bool valid = true;
    for(usize i = 0; i < m_Lower.size(); i++)
    {
      valid = valid && (m_Lower[i] <= m_Upper[i]);
    }
    return valid;
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return BoundingBox&
   */
  BoundingBox& operator=(const BoundingBox& rhs)
  {
    m_Lower = rhs.m_Lower;
    m_Upper = rhs.m_Upper;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return BoundingBox&
   */
  BoundingBox& operator=(BoundingBox&& rhs) noexcept
  {
    m_Lower = std::move(rhs.m_Lower);
    m_Upper = std::move(rhs.m_Upper);
    return *this;
  }

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(const BoundingBox& rhs) const
  {
    return (rhs.m_Lower == m_Lower) && (rhs.m_Upper == m_Upper);
  }

  /**
   * @brief Inequality operator
   * @param rhs
   * @return bool
   */
  bool operator!=(const BoundingBox& rhs) const
  {
    return (rhs.m_Lower != m_Lower) || (rhs.m_Upper != m_Upper);
  }

protected:
  PointType m_Lower;
  PointType m_Upper;
};

template <typename T>
using BoundingBox3D = BoundingBox<T, Point3D>;
using BoundingBox3Df = BoundingBox<float32, Point3D>;
using BoundingBox3Dd = BoundingBox<float64, Point3D>;

template <typename T>
using BoundingBox2D = BoundingBox<T, Point2D>;
using BoundingBox2Df = BoundingBox<float32, Point2D>;
using BoundingBox2Dd = BoundingBox<float64, Point2D>;
} // namespace complex
