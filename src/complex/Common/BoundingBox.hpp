#pragma once

#include <array>

#include "complex/Common/Point3D.hpp"

namespace complex
{

/**
 * @class BoundingBox
 * @brief The BoundingBox class is designed to describe a box in 3D space
 * within which all points of interest are contained. This is primarily
 * used to describe the getSize of a geometry but is also used by GeometryMath
 * for calculations checking for points in a given region. As the BoundingBox
 * class operates along X, Y, and Z axis, no rotation information is available.
 */
template <typename T>
class BoundingBox
{
public:
  //========================================= STL INTERFACE COMPATIBILITY =================================
  using size_type = usize;
  using reference = T&;
  using const_reference = const T&;

  /**
   * @brief PointType is an alias for the type of Point3D used by the
   * BoundingBox based on the templated value type.
   */
  using PointType = complex::Point3D<T>;

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
  BoundingBox(Pointer arr)
  : m_Bounds({arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]})
  {
  }

  ~BoundingBox() = default;

  /**
   * @brief Returns the Min point
   * @return
   */
  std::array<ValueType, 3> getMinPoint() const
  {
    return {m_Bounds[0], m_Bounds[1], m_Bounds[2]};
  }

  /**
   * @brief Returns the Max point
   * @return
   */
  std::array<ValueType, 3> getMaxPoint() const
  {
    return {m_Bounds[3], m_Bounds[4], m_Bounds[5]};
  }

  /**
   * @brief Compute the length of each side of the bounding box
   * @return
   */
  std::array<ValueType, 3> sideLengths() const
  {
    return {m_Bounds[3] - m_Bounds[0], m_Bounds[4] - m_Bounds[1], m_Bounds[5] - m_Bounds[2]};
  }

  /**
   * @brief compute the center of the bounding box.
   * @return
   */
  std::array<ValueType, 3> center() const
  {
    std::array<ValueType, 3> boxSize = sideLengths();
    return {m_Bounds[0] + boxSize[0] / 2.0, m_Bounds[1] + boxSize[1] / 2.0, m_Bounds[2] + boxSize[2] / 2.0};
  }

  /**
   * @brief Returns the current minimum X value.
   * @return ValueType
   */
  ValueType getMinX() const
  {
    return m_Bounds[0];
  }

  /**
   * @brief Returns the current maximum X value.
   * @return ValueType
   */
  ValueType getMaxX() const
  {
    return m_Bounds[3];
  }

  /**
   * @brief Returns the current minimum Y value.
   * @return ValueType
   */
  ValueType getMinY() const
  {
    return m_Bounds[1];
  }

  /**
   * @brief Returns the current maximum Y value.
   * @return ValueType
   */
  ValueType getMaxY() const
  {
    return m_Bounds[4];
  }

  /**
   * @brief Returns the current minimum Z value.
   * @return ValueType
   */
  ValueType getMinZ() const
  {
    return m_Bounds[2];
  }

  /**
   * @brief Returns the current maximum Z value.
   * @return ValueType
   */
  ValueType getMaxZ() const
  {
    return m_Bounds[5];
  }

  /**
   * @brief Sets the current minimum X value.
   * @param value
   */
  void setMinX(ValueType value)
  {
    m_Bounds[0] = value;
  }

  /**
   * @brief Sets the current maximum X value.
   * @param value
   */
  void setMaxX(ValueType value)
  {
    m_Bounds[3] = value;
  }

  /**
   * @brief Sets the current minimum Y value.
   * @param value
   */
  void setMinY(ValueType value)
  {
    m_Bounds[1] = value;
  }

  /**
   * @brief Sets the current maximum Y value.
   * @param value
   */
  void setMaxY(ValueType value)
  {
    m_Bounds[4] = value;
  }

  /**
   * @brief Sets the current minimum Z value.
   * @param value
   */
  void setMinZ(ValueType value)
  {
    m_Bounds[2] = value;
  }

  /**
   * @brief Sets the current maximum Z value.
   * @param value
   */
  void setMaxZ(ValueType value)
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
   * @brief access specified element
   * @param index
   * @return reference
   */
  inline reference operator[](size_type index)
  {
    return m_Bounds[index];
  }

  /**
   * @brief access specified element
   * @param index
   * @return const_reference
   */
  inline const_reference operator[](size_type index) const
  {
    return m_Bounds[index];
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
