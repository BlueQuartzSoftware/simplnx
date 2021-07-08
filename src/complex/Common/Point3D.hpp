#pragma once

#include <array>

namespace complex
{

/**
 * @class Point3D
 * @brief The Point3D class describes a point in 3D space and provides basic
 * calculations for manipulating them. Point3D objects are used primarily for
 * use in geometries and GeometryMath.
 */
template <typename T>
class Point3D
{
public:
  using ValueType = T;
  using Pointer = T*;
  using ConstPointer = const T*;
  const size_t dimensions = 3;
  using ArrayType = std::array<T, 3>;

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
  Point3D(const ArrayType& pos)
  : m_Pos(pos)
  {
  }

  /**
   * @brief Constructs a Point3D from the provided std::array.
   * @param pos
   */
  Point3D(ArrayType&& pos) noexcept
  : m_Pos(std::move(pos))
  {
  }

  /**
   * @brief Constructs a Point3D from three individual values specifying X, Y, and Z positioning.
   * @param x
   * @param y
   * @param z
   */
  Point3D(ValueType x, ValueType y, ValueType z)
  : m_Pos({x, y, z})
  {
  }

  /**
   * @brief Constructs a Point3D by copying values from the provided raw Pointer.
   * The provided Pointer is assumed to contain at least 3 values.
   * @param pos
   */
  Point3D(Pointer pos)
  : m_Pos({pos[0], pos[1], pos[2]})
  {
  }

  /**
   * @brief Constructs a Point3D by copying values from the provided raw Pointer.
   * The provided Pointer is assumed to contain at least 3 values.
   * @param pos
   */
  Point3D(ConstPointer pos)
  : m_Pos({pos[0], pos[1], pos[2]})
  {
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  Point3D(const Point3D& other)
  : m_Pos(other.m_Pos)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Point3D(Point3D&& other) noexcept
  : m_Pos(std::move(other.m_Pos))
  {
  }

  virtual ~Point3D() = default;

  /**
   * @brief Returns the point's X value.
   * @return ValueType
   */
  ValueType getX() const
  {
    return m_Pos[0];
  }

  /**
   * @brief Returns the point's Y value.
   * @return ValueType
   */
  ValueType getY() const
  {
    return m_Pos[1];
  }

  /**
   * @brief Returns the point's Z value.
   * @return ValueType
   */
  ValueType getZ() const
  {
    return m_Pos[2];
  }

  /**
   * @brief Sets the point's X position.
   * @param pos
   */
  void setX(ValueType pos)
  {
    m_Pos[0] = pos;
  }

  /**
   * @brief Sets the point's Y position.
   * @param pos
   */
  void setY(ValueType pos)
  {
    m_Pos[1] = pos;
  }

  /**
   * @brief Sets the point's Z position.
   * @param pos
   */
  void setZ(ValueType pos)
  {
    m_Pos[2] = pos;
  }

  /**
   * @brief Sets the new position by copying a ArrayType.
   * @param pos
   */
  void setPos(const ArrayType& pos)
  {
    m_Pos = pos;
  }

  /**
   * @brief Sets the new position using the provided X, Y, and Z values.
   * @param x
   * @param y
   * @param z
   */
  void setPos(ValueType x, ValueType y, ValueType z)
  {
    m_Pos[0] = x;
    m_Pos[1] = y;
    m_Pos[2] = z;
  }

  /**
   * @brief Sets the new position by copying X, Y, and Z values from the provided
   * raw Pointer. The Pointer is assumed to be an array with at least three values.
   * @param pos
   */
  void setPos(Pointer pos)
  {
    for(size_t i = 0; i < dimensions; i++)
    {
      m_Pos[i] = pos[i];
    }
  }

  /**
   * @brief Returns a ArrayType representation of the current position.
   * @return ArrayType
   */
  ArrayType toArray() const
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
    ArrayType pos = {0, 0, 0};
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
    ArrayType pos = {0, 0, 0};
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

  /**
   * @brief Returns a reference to the value at the specified index. Throws an
   * exception if the index is greater than the number of dimensions.
   * @param index
   * @return ValueType&
   */
  ValueType& operator[](size_t index)
  {
    if(index >= 3)
    {
      throw std::exception();
    }
    return m_Pos[index];
  }

  /**
   * @brief Returns a const reference to the value at the specified index.
   * Throws an exception if the index is greater than the number of dimensions.
   * @param index
   * @return const ValueType&
   */
  const ValueType& operator[](size_t index) const
  {
    if(index >= 3)
    {
      throw std::exception();
    }
    return m_Pos[index];
  }

protected:
private:
  ArrayType m_Pos;
};
} // namespace complex
