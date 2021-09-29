#pragma once

#include <array>

namespace complex
{

/**
 * @class Point2D
 * @brief The Point2D class describes a point in 2D space and provides basic
 * calculations for manipulating them. Point2D objects are used primarily for
 * use in geometries and GeometryMath.
 */
template <typename T>
class Point2D
{
public:
  using ValueType = T;
  using Pointer = T*;
  const usize dimensions = 2;
  using ArrayType = std::array<T, 2>;

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
  Point2D(const ArrayType& pos)
  : m_Pos(pos)
  {
  }

  /**
   * @brief Constructs a Point2D from the provided std::array
   * @param pos
   */
  Point2D(ArrayType&& pos) noexcept
  : m_Pos(std::move(pos))
  {
  }

  /**
   * @brief Constructs a Point2D from two individual values.
   * @param x
   * @param y
   */
  Point2D(ValueType x, ValueType y)
  : m_Pos({x, y})
  {
  }

  /**
   * @brief Constructs a Point2D from a raw Pointer array. This array is
   * expected to have two values that are copied to create the Point2D.
   * @param pos
   */
  Point2D(Pointer pos)
  : m_Pos({pos[0], pos[1]})
  {
  }

  ~Point2D() = default;

  /**
   * @brief Returns the X value.
   * @return ValueType
   */
  ValueType getX() const
  {
    return m_Pos[0];
  }

  /**
   * @brief Returns the Y value.
   * @return ValueType
   */
  ValueType getY() const
  {
    return m_Pos[1];
  }

  /**
   * @brief Sets the X value.
   * @param x
   */
  void setX(ValueType x)
  {
    m_Pos[0] = x;
  }

  /**
   * @brief Sets the Y value.
   * @param y
   */
  void setY(ValueType y)
  {
    m_Pos[1] = y;
  }

  /**
   * @brief Copies the std::array to set the new position.
   * @param pos
   */
  void setPos(const ArrayType& pos)
  {
    m_Pos = pos;
  }

  /**
   * @brief Sets the new position from two individual values.
   * @param  x
   * @param  y
   */
  void setPos(ValueType x, ValueType y)
  {
    m_Pos[0] = x;
    m_Pos[1] = y;
  }

  /**
   * @brief Sets the new position by copying X and Y values from the provided Pointer.
   * @param pos
   */
  void setPos(Pointer pos)
  {
    m_Pos[0] = pos[0];
    m_Pos[1] = pos[1];
  }

  /**
   * @brief Returns the position as a ArrayType
   * @return ArrayType
   */
  ArrayType toArray() const
  {
    return m_Pos;
  }

  /**
   * @brief Returns a reference to the target position index.
   * @param index
   * @return ValueType&
   */
  ValueType& operator[](usize index)
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
    return other.m_Pos == m_Pos;
  }

  /**
   * @brief Checks inequality between two Point2Ds
   * @param other
   * @return bool
   */
  bool operator!=(const Point2D& other) const
  {
    return other.m_Pos != m_Pos;
  }

  /**
   * @brief Creates a new Point2D by adding X and Y values with another Point2D.
   * @param rhs
   * @return Point2D
   */
  Point2D operator+(const Point2D& rhs) const
  {
    ArrayType pos = {0, 0};
    for(usize i = 0; i < dimensions; i++)
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
    ArrayType pos = {0, 0};
    for(usize i = 0; i < dimensions; i++)
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
    for(usize i = 0; i < dimensions; i++)
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
    for(usize i = 0; i < dimensions; i++)
    {
      m_Pos[i] -= rhs.m_Pos[i];
    }
    return *this;
  }

protected:
private:
  ArrayType m_Pos;
};
} // namespace complex
