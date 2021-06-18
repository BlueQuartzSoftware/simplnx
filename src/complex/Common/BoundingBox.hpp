#pragma once

#include <array>

#include "complex/Common/Point3D.hpp"

namespace complex
{

/**
 * class BoundingBox
 *
 */

template <typename T>
class BoundingBox
{
public:
  using PointType = complex::Point3D<T>;
  using value_type = T;
  using pointer = T*;

  /**
   * @brief Constructs a new BoundingBox defined by two corner positions.
   * The points are assumed to be positioned such that the lowerLeft point
   * is less than or equal to upperRight values along each dimension.
   * @param lowerLeft
   * @param upperRight
   */
  BoundingBox(const PointType& lowerLeft, const PointType& upperRight)
  : m_Bounds({lowerLeft[0], lowerLeft[1], lowerLeft[2], upperRight[0], upperRight[1], upperRight[2]})
  {
  }

  /**
   * @brief Constructs a new BoundingBox defined by the array of position values.
   * The format is min X, min Y, min Z, max X, max Y, max Z.
   * @param arr
   */
  BoundingBox(const std::array<T, 6>& arr)
  : m_Bounds(arr)
  {
  }

  /**
   * @brief Constructs a new BoundingBox by copying values from the specified pointer.
   * The pointer is assumed to contain at least six values formatted the same as the
   * std::array<T, 6> constructor, min X, min Y, min Z, max X, max Y, max Z.
   * @param arr
   */
  BoundingBox(pointer arr)
  : m_Bounds({arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]})
  {
  }

  ~BoundingBox() = default;

  /**
   * @brief Returns the current minimum X value.
   * @return value_type
   */
  value_type getMinX() const
  {
    return m_Bounds[0];
  }

  /**
   * @brief Returns the current maximum X value.
   * @return value_type
   */
  value_type getMaxX() const
  {
    return m_Bounds[3];
  }

  /**
   * @brief Returns the current minimum Y value.
   * @return value_type
   */
  value_type getMinY() const
  {
    return m_Bounds[1];
  }

  /**
   * @brief Returns the current maximum Y value.
   * @return value_type
   */
  value_type getMaxY() const
  {
    return m_Bounds[4];
  }

  /**
   * @brief Returns the current minimum Z value.
   * @return value_type
   */
  value_type getMinZ() const
  {
    return m_Bounds[2];
  }

  /**
   * @brief Returns the current maximum Z value.
   * @return value_type
   */
  value_type getMaxZ() const
  {
    return m_Bounds[5];
  }

  /**
   * @brief Sets the current minimum X value.
   * @param value
   */
  void setMinX(value_type value)
  {
    m_Bounds[0] = value;
  }

  /**
   * @brief Sets the current maximum X value.
   * @param value
   */
  void setMaxX(value_type value)
  {
    m_Bounds[3] = value;
  }

  /**
   * @brief Sets the current minimum Y value.
   * @param value
   */
  void setMinY(value_type value)
  {
    m_Bounds[1] = value;
  }

  /**
   * @brief Sets the current maximum Y value.
   * @param value
   */
  void setMaxY(value_type value)
  {
    m_Bounds[4] = value;
  }

  /**
   * @brief Sets the current minimum Z value.
   * @param value
   */
  void setMinZ(value_type value)
  {
    m_Bounds[2] = value;
  }

  /**
   * @brief Sets the current maximum Z value.
   * @param value
   */
  void setMaxZ(value_type value)
  {
    m_Bounds[5] = value;
  }

  /**
   * @brief Checks if the bounding box is valid. A valid box has a non-negative
   * difference between all minimum and maximum values.
   * @return bool
   */
  bool isValid() const
  {
    bool xValid = m_Bounds[0] <= m_Bounds[3];
    bool yValid = m_Bounds[1] <= m_Bounds[4];
    bool zValid = m_Bounds[2] <= m_Bounds[5];
    return xValid && yValid && zValid;
  }

  /**
   * @brief Returns a std::array representation of the bounding box.
   * @return std::array<T, 6>
   */
  std::array<T, 6> toArray() const
  {
    return m_Bounds;
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return BoundingBox&
   */
  BoundingBox& operator=(const BoundingBox& rhs)
  {
    m_Bounds = rhs.m_Bounds;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return BoundingBox&
   */
  BoundingBox& operator=(BoundingBox&& rhs)
  {
    m_Bounds = std::move(rhs.m_Bounds);
    return *this;
  }

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(const BoundingBox& rhs) const
  {
    return rhs.m_Bounds == m_Bounds;
  }

  /**
   * @brief Inequality operator
   * @param rhs
   * @return bool
   */
  bool operator!=(const BoundingBox& rhs) const
  {
    return rhs.m_Bounds != m_Bounds;
  }

protected:
private:
  std::array<T, 6> m_Bounds;
};
} // namespace complex
