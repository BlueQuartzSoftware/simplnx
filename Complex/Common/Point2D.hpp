#pragma once

#include <array>

namespace Complex
{

/**
 * class Point2D
 *
 */

template <typename T>
class Point2D
{
public:
  using value_type = T;
  using pointer = T*;
  const size_t dimensions = 2;
  using array_type = std::array<T, 2>;

  /**
   * @brief Default constructor. Creates a Point3D at (0, 0).
   */
  Point2D()
  : m_Pos({0, 0})
  {
  }

  /**
   * @brief Constructs a Point2D from the provided std::array
   * @param pos
   */
  Point2D(const array_type& pos)
  : m_Pos(pos)
  {
  }

  /**
   * @brief Constructs a Point2D from the provided std::array
   * @param pos
   */
  Point2D(array_type&& pos) noexcept
  : m_Pos(std::move(pos))
  {
  }

  /**
   * @brief Constructs a Point2D from two individual values.
   * @param x
   * @param y
   */
  Point2D(value_type x, value_type y)
  : m_Pos({x, y})
  {
  }

  /**
   * @brief Constructs a Point2D from a raw pointer array. This array is
   * expected to have two values that are copied to create the Point2D.
   * @param pos
   */
  Point2D(pointer pos)
  : m_Pos({pos[0], pos[1]})
  {
  }

  ~Point2D() = default;

  /**
   * @brief Returns the X value.
   * @return value_type
   */
  value_type getX() const
  {
    return m_Pos[0];
  }

  /**
   * @brief Returns the Y value.
   * @return value_type
   */
  value_type getY() const
  {
    return m_Pos[1];
  }

  /**
   * @brief Sets the X value.
   * @param x
   */
  void setX(value_type x)
  {
    m_Pos[0] = x;
  }

  /**
   * @brief Sets the Y value.
   * @param y
   */
  void setY(value_type y)
  {
    m_Pos[1] = y;
  }

  /**
   * @brief Copies the std::array to set the new position.
   * @param pos
   */
  void setPos(const array_type& pos)
  {
    m_Pos = pos;
  }

  /**
   * @brief Sets the new position from two individual values.
   * @param  x
   * @param  y
   */
  void setPos(value_type x, value_type y)
  {
    m_Pos[0] = x;
    m_Pos[1] = y;
  }

  /**
   * @brief Sets the new position by copying X and Y values from the provided pointer.
   * @param pos
   */
  void setPos(pointer pos)
  {
    m_Pos[0] = pos[0];
    m_Pos[1] = pos[1];
  }

  /**
   * @brief Returns the position as a array_type
   * @return array_type
   */
  array_type toArray() const
  {
    return m_Pos;
  }

  /**
   * @brief Returns a reference to the target position index.
   * @param index
   * @return value_type&
   */
  value_type& operator[](size_t index)
  {
    return m_Pos[index];
  }

  /**
   * @brief Copy assignment operator
   * @param other
   * @return Point2D&
   */
  Point2D& operator=(const Point2D& other)
  {
    m_Pos = other.m_Pos;
  }

  /**
   * @brief Move assignment operator
   * @param other
   * @return Point2D&
   */
  Point2D& operator=(Point2D&& other)
  {
    m_Pos = std::move(other.m_Pos);
  }

  /**
   * @brief Checks equality between two Point2Ds
   * @param other
   * @return bool
   */
  bool operator==(const Point2D& other) const
  {
    return ther.m_Pos == m_Pos;
  }

  /**
   * @brief Checks inequality between two Point2Ds
   * @param other
   * @return bool
   */
  bool operator!=(const Point2D& other) const
  {
    return ther.m_Pos != m_Pos;
  }

  /**
   * @brief Creates a new Point2D by adding X and Y values with another Point2D.
   * @param rhs
   * @return Point2D
   */
  Point2D operator+(const Point2D& rhs) const
  {
    array_type pos = {0, 0};
    for(size_t i = 0; i < dimensions; i++)
    {
      pos[i] = m_Pos[i] + rhs.m_Pos[i];
    }
    return Point2D(pos);
  }

  /**
   * @brief Creates a new Point2D by subtracting X and Y values with another Point2D.
   * @param rhs
   * @return Point2D
   */
  Point2D operator-(const Point2D& rhs) const
  {
    array_type pos = {0, 0};
    for(size_t i = 0; i < dimensions; i++)
    {
      pos[i] = m_Pos[i] - rhs.m_Pos[i];
    }
    return Point2D(pos);
  }

  /**
   * @brief Increments the position value by adding X and Y values from another Point2D.
   * @param rhs
   * @return Point2D&
   */
  Point2D& operator+=(const Point2D& rhs)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] += rhs.m_Pos[i];
    }
    return *this;
  }

  /**
   * @brief Decrements the position value by subtraccting X and Y values from another Point2D.
   * @param rhs
   * @return Point2D&
   */
  Point2D& operator-=(const Point2D& rhs)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] -= rhs.m_Pos[i];
    }
    return *this;
  }

protected:
private:
  array_type m_Pos;
};
} // namespace Complex
