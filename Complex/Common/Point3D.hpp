#pragma once

#include <array>

namespace Complex
{

/**
 * class Point3D
 *
 */

template <typename T>
class Point3D
{
public:
  using value_type = T;
  using pointer = T*;
  const size_t dimensions = 3;
  using array_type = std::array<T, 3>;

  /**
   * @brief Default constructor. Creates a Point3D at (0, 0, 0).
   */
  Point3D()
  : m_Pos({0, 0, 0})
  {
  }

  /**
   * @brief Constructs a Point3D from the provided std::array.
   * @param pos
   */
  Point3D(const array_type& pos)
  : m_Pos(pos)
  {
  }

  /**
   * @brief Constructs a Point3D from the provided std::array.
   * @param pos
   */
  Point3D(array_type&& pos) noexcept
  : m_Pos(std::move(pos))
  {
  }

  /**
   * @brief Constructs a Point3D from three individual values specifying X, Y, and Z positioning.
   * @param x
   * @param y
   * @param z
   */
  Point3D(value_type x, value_type y, value_type z)
  : m_Pos({x, y, z})
  {
  }

  /**
   * @brief Constructs a Point3D by copying values from the provided raw pointer.
   * The provided pointer is assumed to contain at least 3 values.
   * @param pos
   */
  Point3D(pointer pos)
  : m_Pos({pos[0], pos[1], pos[2]})
  {
  }

  ~Point3D() = default;

  /**
   * @brief Returns the point's X value.
   * @return value_type
   */
  value_type getX() const
  {
    return m_Pos[0];
  }

  /**
   * @brief Returns the point's Y value.
   * @return value_type
   */
  value_type getY() const
  {
    return m_Pos[1];
  }

  /**
   * @brief Returns the point's Z value.
   * @return value_type
   */
  value_type getZ() const
  {
    return m_Pos[2];
  }

  /**
   * @brief Sets the point's X position.
   * @param pos
   */
  void setX(value_type pos)
  {
    m_Pos[0] = pos;
  }

  /**
   * @brief Sets the point's Y position.
   * @param pos
   */
  void setY(value_type pos)
  {
    m_Pos[1] = pos;
  }

  /**
   * @brief Sets the point's Z position.
   * @param pos
   */
  void setZ(value_type pos)
  {
    m_Pos[2] = pos;
  }

  /**
   * @brief Sets the new position by copying a array_type.
   * @param pos
   */
  void setPos(const array_type& pos)
  {
    m_Pos = pos;
  }

  /**
   * @brief Sets the new position using the provided X, Y, and Z values.
   * @param x
   * @param y
   * @param z
   */
  void setPos(value_type x, value_type y, value_type z)
  {
    m_Pos[0] = x;
    m_Pos[1] = y;
    m_Pos[2] = z;
  }

  /**
   * @brief Sets the new position by copying X, Y, and Z values from the provided
   * raw pointer. The pointer is assumed to be an array with at least three values.
   * @param pos
   */
  void setPos(pointer pos)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] = pos[i];
    }
  }

  /**
   * @brief Returns a array_type representation of the current position.
   * @return array_type
   */
  array_type toArray() const
  {
    return m_Pos;
  }

  /**
   * @brief Creates a new Point3D by adding position values with another Point3D.
   * @param rhs
   * @return Point3D
   */
  Point3D operator+(const Point3D& rhs) const
  {
    array_type pos = {0, 0, 0};
    for(size_t i = 0; i < dimensions; i++)
    {
      pos[i] = m_Pos[i] + rhs.m_Pos[i];
    }
    return Point3D(pos);
  }

  /**
   * @brief Creates a new Point3D by subtracting position values with another Point3D.
   * @param rhs
   * @return Point3D
   */
  Point3D operator-(const Point3D& rhs) const
  {
    array_type pos = {0, 0, 0};
    for(size_t i = 0; i < dimensions; i++)
    {
      pos[i] = m_Pos[i] - rhs.m_Pos[i];
    }
    return Point3D(pos);
  }

  /**
   * @brief Increments the position values by adding position values with another Point3D.
   * @param rhs
   * @return Point3D&
   */
  Point3D& operator+=(const Point3D& rhs)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] += rhs.m_Pos[i];
    }
    return *this;
  }

  /**
   * @brief Decrements the position values by subtracting position values with another Point3D.
   * @param rhs
   * @return Point3D&
   */
  Point3D& operator-=(const Point3D& rhs)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] -= rhs.m_Pos[i];
    }
    return *this;
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Point3D&
   */
  Point3D& operator=(const Point3D& rhs)
  {
    m_Pos = rhs.m_Pos;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Point3D&
   */
  Point3D& operator=(Point3D&& rhs)
  {
    m_Pos = std::move(rhs.m_Pos);
    return *this;
  }

  /**
   * @brief Equality operator
   * @param rhs
   * @return bool
   */
  bool operator==(const Point3D& rhs) const
  {
    return rhs.m_Pos == m_Pos;
  }

  /**
   * @brief Inequality operator
   * @param rhs
   * @return bool
   */
  bool operator!=(const Point3D& rhs) const
  {
    return rhs.m_Pos != m_Pos;
  }

protected:
private:
  array_type m_Pos;
};
} // namespace Complex
