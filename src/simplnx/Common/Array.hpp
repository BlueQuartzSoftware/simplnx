#pragma once

#include "simplnx/Common/Types.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <tuple>
#include <vector>

namespace nx::core
{
template <typename T, usize Dimensions>
class Array
{
public:
  //========================================= STL INTERFACE COMPATIBILITY =================================
  using size_type = usize;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using iterator_category = std::input_iterator_tag;
  using pointer = T*;
  using difference_type = value_type;
  using iterator = typename std::array<T, Dimensions>::iterator;
  using const_iterator = typename std::array<T, Dimensions>::const_iterator;
  //========================================= END STL INTERFACE COMPATIBILITY ==============================

protected:
  Array()
  {
    for(usize i = 0; i < Dimensions; i++)
    {
      m_Array[i] = static_cast<T>(0);
    }
  }

  Array(const std::array<value_type, Dimensions>& data)
  : m_Array(data)
  {
  }

  Array(const std::vector<value_type>& data)
  {
    for(usize i = 0; i < Dimensions; i++)
    {
      m_Array[i] = data[i];
    }
  }

  Array(const value_type* data)
  {
    for(usize i = 0; i < Dimensions; i++)
    {
      m_Array[i] = data[i];
    }
  }

  /**
   * @brief Copy constructor
   * @param other
   */
  Array(const Array& other)
  : m_Array(other.m_Array)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Array(Array&& other) noexcept
  : m_Array(std::move(other.m_Array))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Array&
   */
  Array& operator=(const Array& rhs)
  {
    m_Array = rhs.m_Array;
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Array&
   */
  Array& operator=(Array&& rhs) noexcept
  {
    m_Array = std::move(rhs.m_Array);
    return *this;
  }

  virtual ~Array() = default;

  /**
   * @brief Constructor using a random access container type as the input. This will copy the data
   */
  template <typename InType>
  Array(const InType& data)
  {
    for(usize i = 0; i < Dimensions; i++)
    {
      m_Array[i] = data[i];
    }
  }

public:
  /**
   * @brief access specified element
   * @param index
   * @return reference
   */
  inline reference operator[](size_type index)
  {
    return m_Array[index];
  }

  /**
   * @brief access specified element
   * @param index
   * @return const_reference
   */
  inline const_reference operator[](size_type index) const
  {
    return m_Array[index];
  }

  /**
   * @brief access specified element with bounds checking
   * @param index
   * @return reference
   */
  inline reference at(size_type index)
  {
    assert(index < Dimensions);
    return m_Array.at(index);
  }

  /**
   * @brief access specified element with bounds checking
   * @param index
   * @return const_reference
   */
  inline const_reference at(size_type index) const
  {
    assert(index < Dimensions);
    return m_Array.at(index);
  }

  /**
   * @brief returns an iterator to the beginning
   * @return iterator
   */
  iterator begin()
  {
    return m_Array.begin();
  }

  /**
   * @brief returns an iterator to the end
   * @return iterator
   */
  iterator end()
  {
    return m_Array.end();
  }

  /**
   * @brief returns an iterator to the beginning
   * @return const_iterator
   */
  const_iterator begin() const
  {
    return m_Array.cbegin();
  }

  /**
   * @brief returns an iterator to the end
   * @return const_iterator
   */
  const_iterator end() const
  {
    return m_Array.cend();
  }

  /**
   * @brief direct access to the underlying array
   * @return pointer
   */
  pointer data()
  {
    return m_Array.data();
  }

  /**
   * @brief Returns the number of elements
   * @return size_type
   */
  size_type size() const
  {
    return Dimensions;
  }

  /**
   * @brief operator == Tests for an element by element equivalence of the underlying data
   * @param rhs
   * @return bool
   */
  bool operator==(const Array& rhs) const
  {
    return m_Array == rhs.m_Array;
  }

  /**
   * @brief operator != Tests for inequality of the underlying data
   * @param rhs
   * @return bool
   */
  bool operator!=(const Array& rhs) const
  {
    return m_Array != rhs.m_Array;
  }

  /**
   * @brief Converts to another container type. The output type that is being used needs to have the "push_back()" method implemented.
   *
   *   For STL containers this includes Vector, Deque. QVector will also work.
   */
  template <typename OutContainerType>
  OutContainerType toContainer() const
  {
    OutContainerType dest(Dimensions);
    for(typename OutContainerType::size_type i = 0; i < Dimensions; i++)
    {
      dest[i] = m_Array[i];
    }
    return dest;
  }

  /**
   * @brief Returns a copy of the data as a std::array<T,Dimensions>
   * @return std::array<T, Dimensions>
   */
  std::array<T, Dimensions> toArray() const
  {
    return m_Array;
  }

protected:
  /**
   * @brief Assigns the value at the specified index.
   * @param i
   * @param value
   */
  void setValue(usize i, value_type value)
  {
    m_Array[i] = value;
  }

private:
  std::array<T, Dimensions> m_Array;
};

// -----------------------------------------------------------------------------
template <typename T>
class Vec2 : public Array<T, 2>
{
  using ParentType = Array<T, 2>;

public:
  using tuple_type = std::tuple<T, T>;

  /**
   * @brief Copy constructor
   * @param other
   */
  Vec2(const Vec2& other)
  : Array<T, 2>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Vec2(Vec2&& other) noexcept
  : Array<T, 2>(std::move(other))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Vec2&
   */
  Vec2& operator=(const Vec2& rhs)
  {
    Array<T, 2>::operator=(rhs);
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Vec2&
   */
  Vec2& operator=(Vec2&& rhs) noexcept
  {
    Array<T, 2>::operator=(std::move(rhs));
    return *this;
  }

  ~Vec2() override = default;

  /**
   * @brief Vec2 Default constructor initializes all values to ZERO.
   */
  Vec2() = default;

  /**
   * @brief Vec2 constructor taking both X and Y values
   * @param x
   * @param y
   */
  Vec2(T x, T y)
  {
    (*this)[0] = x;
    (*this)[1] = y;
  }

  /**
   * @brief Vec2 constructor copying X and Y values from a std::array.
   * @param data
   */
  Vec2(const std::array<T, 2>& data)
  : Array<T, 2>(data)
  {
  }

  /**
   * @brief Vec2 constructor copying X and Y values from the specified tuple.
   * @param data
   */
  Vec2(const tuple_type& data)
  {
    (*this)[0] = std::get<0>(data);
    (*this)[1] = std::get<1>(data);
  }

  /**
   * @brief Vec2 constructor copying X and Y values from the provided pointer.
   * The pointer is assumed to be an array with at least two values.
   * @param data
   */
  Vec2(const T* data)
  : Array<T, 2>(data)
  {
  }

  /**
   * @brief Vec2 constructor copying X and Y values from the provided vector.
   * The vector is assumed to have at least two values.
   * @param data
   */
  Vec2(const std::vector<T>& data)
  : Array<T, 2>(data)
  {
  }

  /**
   * @brief Returns the current X value.
   * @return value_type
   */
  inline T getX() const
  {
    return ParentType::operator[](0);
  }

  /**
   * @brief Returns the current Y value.
   * @return value_type
   */
  inline T getY() const
  {
    return ParentType::operator[](1);
  }

  /**
   * @brief Assigns a new X value.
   * @param x
   */
  inline void setX(const T& x)
  {
    (*this)[0] = x;
  }

  /**
   * @brief Assigns a new Y value.
   * @param y
   */
  inline void setY(const T& y)
  {
    (*this)[1] = y;
  }

  /**
   * @brief Assigns both X and Y values.
   * This is a convenience function that helps with the python bindings.
   * @param x
   * @param y
   */
  inline void setValues(const T& x, const T& y)
  {
    (*this)[0] = x;
    (*this)[1] = y;
  }

  /**
   * @brief Converts the internal data structure to a tuple_type.
   * @return tuple_type
   */
  tuple_type toTuple() const
  {
    return std::make_tuple(getX(), getY());
  }

  /**
   * @brief Returns a Vec2 of the specified type by static_casting both X and Y values.
   * @return Vec2<OutType>
   */
  template <typename OutType>
  Vec2<OutType> convertType()
  {
    return Vec2<OutType>(static_cast<OutType>((*this)[0]), static_cast<OutType>((*this)[1]));
  }

  /**
   * @brief Returns the dot product of this and an input Vec2
   * @return Vec2<T>
   */
  inline T dot(const Vec2<T>& v) const
  {
    return (*this)[0] * v[0] + (*this)[1] * v[1];
  }

  /**
   * @brief Returns the sum of squares of this
   * @return T
   */
  inline T sumOfSquares() const
  {
    return (*this)[0] * (*this)[0] + (*this)[1] * (*this)[1];
  }

  /**
   * @brief Returns the magnitude of the 3x1 vector
   */
  inline T magnitude() const
  {
    return std::sqrt(sumOfSquares());
  }

  /**
   * @brief Multiples this Vec2 with another Vec2 using element wise multiplication and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 elemMultiply(const Vec2& v) const
  {
    return Vec2((*this)[0] * v.x, (*this)[1] * v.y);
  }

  /**
   * @brief Divides this Vec2 with another Vec2 using element wise multiplication and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 elemDivide(const Vec2& v) const
  {
    return Vec2((*this)[0] / v.x, (*this)[1] / v.y);
  }

  /**
   * @brief Adds this Vec2 to another Vec2 and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 operator+(const Vec2& v) const
  {
    return Vec2((*this)[0] + v[0], (*this)[1] + v[1]);
  }
  /**
   * @brief Subtracts this Vec2 to another Vec2 and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 operator-(const Vec2& v) const
  {
    return Vec2((*this)[0] - v[0], (*this)[1] - v[1]);
  }
  /**
   * @brief Negates this Vec2 and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 operator-() const
  {
    return Vec2(-(*this)[0], -(*this)[1]);
  }
  /**
   * @brief Multiples this Vec2 by a scalar value and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 operator*(const T r) const
  {
    return Vec2((*this)[0] * r, (*this)[1] * r);
  }
  /**
   * @brief Divides this Vec2 by a scalar value and returns a new instance
   * @param Vec2
   * @return new Vec2 instance with the result
   */
  inline Vec2 operator/(const T r) const
  {
    return Vec2((*this)[0] / r, (*this)[1] / r);
  }
  /**
   * @brief Divides this Vec2 with another Vec2 using element wise division. Performs in-place division
   * @param Vec2
   * @return Current Instance is returned
   */
  inline Vec2& operator/=(const T r)
  {
    (*this)[0] /= r;
    (*this)[1] /= r;
    return *this;
  }
  /**
   * @brief Multiplies this Vec2 by a scalar value using element wise multiplication. Performs in-place.
   * @param Vec2
   * @return Current Instance is returned
   */
  inline Vec2& operator*=(const T r)
  {
    (*this)[0] *= r;
    (*this)[1] *= r;
    return *this;
  }
};

using FloatVec2Type = Vec2<float32>;
using IntVec2Type = Vec2<int32>;
using SizeVec2 = Vec2<usize>;

// -----------------------------------------------------------------------------
template <typename T>
class Vec3 : public Array<T, 3>
{
  using ParentType = Array<T, 3>;

public:
  using tuple_type = std::tuple<T, T, T>;

  /**
   * @brief Copy constructor
   * @param other
   */
  Vec3(const Vec3& other)
  : Array<T, 3>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Vec3(Vec3&& other) noexcept
  : Array<T, 3>(std::move(other))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Vec3&
   */
  Vec3& operator=(const Vec3& rhs)
  {
    Array<T, 3>::operator=(rhs);
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Vec3&
   */
  Vec3& operator=(Vec3&& rhs) noexcept
  {
    Array<T, 3>::operator=(std::move(rhs));
    return *this;
  }

  ~Vec3() override = default;

  /**
   * @brief Vec3 Default constructor initializes all values to ZERO.
   */
  Vec3() = default;

  /**
   * @brief Constructs a Vec3 using individual X, Y, and Z values.
   * @param x
   * @param y
   * @param z
   */
  Vec3(T x, T y, T z)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
  }

  /**
   * @brief Constructs a Vec3 by copying values from the std::array.
   * @param data
   */
  Vec3(const std::array<T, 3>& data)
  : Array<T, 3>(data)
  {
  }

  /**
   * @brief Constructs a Vec3 by copying values from the std::tuple.
   * @param data
   */
  Vec3(const tuple_type& data)
  {
    (*this)[0] = std::get<0>(data);
    (*this)[1] = std::get<1>(data);
    (*this)[2] = std::get<2>(data);
  }

  /**
   * @brief Constructs a Vec3 by copying values from the provided pointer.
   * The pointer is assumed to be an array with at least three values.
   * @param data
   */
  Vec3(const T* data)
  : Array<T, 3>(data)
  {
  }

  /**
   * @brief Constructs a Vec3 by copying values from the specified vector.
   * The vector is assumed to contain at least three values.
   * @param data
   */
  Vec3(const std::vector<T>& data)
  : Array<T, 3>(data)
  {
  }

  /**
   * @brief Returns the current X value.
   * @return value_type
   */
  inline T getX() const
  {
    return ParentType::operator[](0);
  }

  /**
   * @brief Returns the current Y value.
   * @return value_type
   */
  inline T getY() const
  {
    return ParentType::operator[](1);
  }

  /**
   * @brief Returns the current Z value.
   * @return value_type
   */
  inline T getZ() const
  {
    return ParentType::operator[](2);
  }

  /**
   * @brief Assigns a new X value.
   * @param x
   */
  inline void setX(const T& x)
  {
    (*this)[0] = x;
  }

  /**
   * @brief Assigns a new Y value.
   * @param y
   */
  inline void setY(const T& y)
  {
    (*this)[1] = y;
  }

  /**
   * @brief Assigns a new Z value.
   * @param z
   */
  inline void setZ(const T& z)
  {
    (*this)[2] = z;
  }

  /**
   * @brief Assigns new X, Y, and Z values.
   * This is a convenience function that helps with the python bindings
   * @param x
   * @param y
   * @param z
   */
  inline void setValues(const T& x, const T& y, const T& z)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
  }

  /**
   * @brief Converts the internal data structure to a tuple.
   * @return tuple_type
   */
  tuple_type toTuple() const
  {
    return std::make_tuple(getX(), getY(), getZ());
  }

  /**
   * @brief Converts to a new Array with a different storage data type.
   * Values should be static_cast convertible to the new type.
   * @return Vec3<OutType>
   */
  template <typename OutType>
  Vec3<OutType> convertType()
  {
    return Vec3<OutType>(static_cast<OutType>((*this)[0]), static_cast<OutType>((*this)[1]), static_cast<OutType>((*this)[2]));
  }

  /**
   * @brief Returns the cross product of this and an input Vec3
   * @return Vec3<T>
   */
  inline Vec3 cross(const Vec3<T>& v) const
  {
    return Vec3<T>((*this)[1] * v[2] - (*this)[2] * v[1], (*this)[2] * v[0] - (*this)[0] * v[2], (*this)[0] * v[1] - (*this)[1] * v[0]);
  }

  /**
   * @brief Returns the dot product of this and an input Vec3
   * @return Vec3<T>
   */
  inline T dot(const Vec3<T>& v) const
  {
    return (*this)[0] * v[0] + (*this)[1] * v[1] + (*this)[2] * v[2];
  }

  /**
   * @brief Returns the sum of squares of this
   * @return T
   */
  inline T sumOfSquares() const
  {
    return (*this)[0] * (*this)[0] + (*this)[1] * (*this)[1] + (*this)[2] * (*this)[2];
  }

  /**
   * @brief Returns the magnitude of the 3x1 vector
   */
  inline T magnitude() const
  {
    return std::sqrt(dot(*this));
  }

  /**
   * @brief Multiples this Vec3 with another Vec3 using element wise multiplication and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 elemMultiply(const Vec3& v) const
  {
    return Vec3((*this)[0] * v.x, (*this)[1] * v.y, (*this)[2] * v.z);
  }

  /**
   * @brief Divides this Vec3 with another Vec3 using element wise multiplication and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 elemDivide(const Vec3& v) const
  {
    return Vec3((*this)[0] / v.x, (*this)[1] / v.y, (*this)[2] / v.z);
  }

  /**
   * @brief Adds this Vec3 to another Vect3 and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 operator+(const Vec3& v) const
  {
    return Vec3((*this)[0] + v[0], (*this)[1] + v[1], (*this)[2] + v[2]);
  }
  /**
   * @brief Subtracts this Vec3 to another Vect3 and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 operator-(const Vec3& v) const
  {
    return Vec3((*this)[0] - v[0], (*this)[1] - v[1], (*this)[2] - v[2]);
  }
  /**
   * @brief Negates this Vec3 and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 operator-() const
  {
    return Vec3(-(*this)[0], -(*this)[1], -(*this)[2]);
  }
  /**
   * @brief Multiples this Vec3 by a scalar value and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 operator*(const T r) const
  {
    return Vec3((*this)[0] * r, (*this)[1] * r, (*this)[2] * r);
  }
  /**
   * @brief Divides this Vec3 by a scalar value and returns a new instance
   * @param Vec3
   * @return new Vec3 instance with the result
   */
  inline Vec3 operator/(const T r) const
  {
    return Vec3((*this)[0] / r, (*this)[1] / r, (*this)[2] / r);
  }
  /**
   * @brief Divides this Vec3 with another Vec3 using element wise division. Performs in-place division
   * @param Vec3
   * @return Current Instance is returned
   */
  inline Vec3& operator/=(const T r)
  {
    (*this)[0] /= r;
    (*this)[1] /= r;
    (*this)[2] /= r;
    return *this;
  }
  /**
   * @brief Multiplies this Vec3 by a scalar value using element wise multiplication. Performs in-place.
   * @param Vec3
   * @return Current Instance is returned
   */
  inline Vec3& operator*=(const T r)
  {
    (*this)[0] *= r;
    (*this)[1] *= r;
    (*this)[2] *= r;
    return *this;
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class Vec4 : public Array<T, 4>
{
  using ParentType = Array<T, 4>;

public:
  using tuple_type = std::tuple<T, T, T, T>;

  /**
   * @brief Copy constructor
   * @param other
   */
  Vec4(const Vec4& other)
  : Array<T, 4>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Vec4(Vec4&& other) noexcept
  : Array<T, 4>(std::move(other))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Vec4&
   */
  Vec4& operator=(const Vec4& rhs)
  {
    Array<T, 4>::operator=(rhs);
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Vec4&
   */
  Vec4& operator=(Vec4&& rhs) noexcept
  {
    Array<T, 4>::operator=(std::move(rhs));
    return *this;
  }

  ~Vec4() override = default;

  /**
   * @brief Vec4 Default constructor initializes all values to ZERO.
   */
  Vec4() = default;

  /**
   * @brief Constructs a Vec4 using X, Y, Z, and W values.
   * @param x
   * @param y
   * @param z
   * @param w
   */
  Vec4(T x, T y, T z, T w)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = w;
  }

  /**
   * @brief Constructs a Vec4 by copying values from a std::array.
   * @param data
   */
  Vec4(const std::array<T, 4>& data)
  : Array<T, 4>(data)
  {
  }

  /**
   * @brief Constructs a Vec4 by copying values from a std::tuple.
   * @param data
   */
  Vec4(const tuple_type& data)
  {
    (*this)[0] = std::get<0>(data);
    (*this)[1] = std::get<1>(data);
    (*this)[2] = std::get<2>(data);
    (*this)[3] = std::get<3>(data);
  }

  /**
   * @brief Constructs a Vec4 by copying values from the specified pointer.
   * The pointer is assumed to be an array with at least four values.
   * @param data
   */
  Vec4(const T* data)
  : Array<T, 4>(data)
  {
  }

  /**
   * @brief Constructs a Vec4 by copying values from the specified vector.
   * The vector is assumed to contain at least four values.
   * @param data
   */
  Vec4(const std::vector<T>& data)
  : Array<T, 4>(data)
  {
  }

  /**
   * @brief Returns the current X value.
   * @return value_type
   */
  inline T getX() const
  {
    return ParentType::operator[](0);
  }

  /**
   * @brief Returns the current Y value.
   * @return value_type
   */
  inline T getY() const
  {
    return ParentType::operator[](1);
  }

  /**
   * @brief Returns the current Z value.
   * @return value_type
   */
  inline T getZ() const
  {
    return ParentType::operator[](2);
  }

  /**
   * @brief Returns the current W value.
   * @return value_type
   */
  inline T getW() const
  {
    return ParentType::operator[](3);
  }

  /**
   * @brief Assigns a new X value.
   * @param x
   */
  inline void setX(const T& x)
  {
    (*this)[0] = x;
  }

  /**
   * @brief Assigns a new Y value.
   * @param y
   */
  inline void setY(const T& y)
  {
    (*this)[1] = y;
  }

  /**
   * @brief Assigns a new Z value.
   * @param z
   */
  inline void setZ(const T& z)
  {
    (*this)[2] = z;
  }

  /**
   * @brief Assigns a new W value.
   * @param w
   */
  inline void setW(const T& w)
  {
    (*this)[3] = w;
  }

  /**
   * @brief Assigns new X, Y, Z, and W values.
   * This is a convenience function that helps with the python bindings
   * @param x
   * @param y
   * @param z
   * @param w
   */
  inline void setValues(const T& x, const T& y, const T& z, const T& w)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = w;
  }

  /**
   * @brief Converts the internal data structure to a std::tuple.
   * @return tuple_type
   */
  tuple_type toTuple() const
  {
    return std::make_tuple(getX(), getY(), getZ(), getW());
  }

  /**
   * @brief Converts this array into another array using a static_cast<OutType> mechanism
   * @return Vec4<OutType>
   */
  template <typename OutType>
  Vec4<OutType> convertType()
  {
    return Vec4<OutType>(static_cast<OutType>((*this)[0]), static_cast<OutType>((*this)[1]), static_cast<OutType>((*this)[2]), static_cast<OutType>((*this)[3]));
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class Vec6 : public Array<T, 6>
{
  using ParentType = Array<T, 6>;

public:
  using tuple_type = std::tuple<T, T, T, T, T, T>;

  /**
   * @brief Copy constructor
   * @param other
   */
  Vec6(const Vec6& other)
  : Array<T, 6>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Vec6(Vec6&& other) noexcept
  : Array<T, 6>(std::move(other))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Vec6&
   */
  Vec6& operator=(const Vec6& rhs)
  {
    Array<T, 6>::operator=(rhs);
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Vec6&
   */
  Vec6& operator=(Vec6&& rhs) noexcept
  {
    Array<T, 6>::operator=(std::move(rhs));
    return *this;
  }

  ~Vec6() override = default;

  /**
   * @brief Vec6 Default constructor initializes all values to ZERO.
   */
  Vec6() = default;

  /**
   * @brief Constructs a Vec6 using specified values.
   * @param x
   * @param y
   * @param z
   * @param a
   * @param b
   * @param c
   */
  Vec6(T x, T y, T z, T a, T b, T c)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = a;
    (*this)[4] = b;
    (*this)[5] = c;
  }

  /**
   * @brief Constructs a Vec6 by copying values from the provided std::array.
   * @param data
   */
  Vec6(const std::array<T, 6>& data)
  : Array<T, 6>(data)
  {
  }

  /**
   * @brief Constructs a Vec6 by copying values from the provided std::tuple.
   * @param data
   */
  Vec6(const tuple_type& data)
  {
    (*this)[0] = std::get<0>(data);
    (*this)[1] = std::get<1>(data);
    (*this)[2] = std::get<2>(data);
    (*this)[3] = std::get<3>(data);
    (*this)[4] = std::get<4>(data);
    (*this)[5] = std::get<5>(data);
  }

  /**
   * @brief Constructs a Vec6 by copying values from the provided pointer.
   * The pointer is assumed to be an array with at least six values.
   * @param data
   */
  Vec6(const T* data)
  : Array<T, 6>(data)
  {
  }

  /**
   * @brief Constructs a Vec6 by copying values from the provided vector.
   * The vector is assumed to have at least six values.
   * @param data
   */
  Vec6(const std::vector<T>& data)
  : Array<T, 6>(data)
  {
  }

  /**
   * @brief Assigns new values to each position in the Vec6.
   * This is a convenience function that helps with the python bindings
   * @param x
   * @param y
   * @param z
   * @param i
   * @param j
   * @param k
   */
  inline void setValues(const T& x, const T& y, const T& z, const T& i, const T& j, const T& k)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = i;
    (*this)[4] = j;
    (*this)[5] = k;
  }

  /**
   * @brief Converts the internal data structure to a std::tuple.
   * @return tuple_type
   */
  tuple_type toTuple() const
  {
    return std::make_tuple((*this)[0], (*this)[1], (*this)[2], (*this)[3], (*this)[4], (*this)[5]);
  }

  /**
   * @brief Converts this array into another array using a static_cast<OutType> mechanism
   * @return Vec6<OutType>
   */
  template <typename OutType>
  Vec6<OutType> convertType()
  {
    return Vec6<OutType>(static_cast<OutType>((*this)[0]), static_cast<OutType>((*this)[1]), static_cast<OutType>((*this)[2]), static_cast<OutType>((*this)[3]), static_cast<OutType>((*this)[4]),
                         static_cast<OutType>((*this)[5]));
  }
};

// -----------------------------------------------------------------------------
template <typename T>
class Vec7 : public Array<T, 7>
{
  using ParentType = Array<T, 7>;

public:
  using tuple_type = std::tuple<T, T, T, T, T, T, T>;

  /**
   * @brief Copy constructor
   * @param other
   */
  Vec7(const Vec7& other)
  : Array<T, 7>(other)
  {
  }

  /**
   * @brief Move constructor
   * @param other
   */
  Vec7(Vec7&& other) noexcept
  : Array<T, 7>(std::move(other))
  {
  }

  /**
   * @brief Copy assignment operator
   * @param rhs
   * @return Vec7&
   */
  Vec7& operator=(const Vec7& rhs)
  {
    Array<T, 7>::operator=(rhs);
    return *this;
  }

  /**
   * @brief Move assignment operator
   * @param rhs
   * @return Vec7&
   */
  Vec7& operator=(Vec7&& rhs) noexcept
  {
    Array<T, 7>::operator=(std::move(rhs));
    return *this;
  }

  ~Vec7() override = default;

  /**
   * @brief Vec7 Default constructor initializes all values to ZERO.
   */
  Vec7() = default;

  /**
   * @brief Constructs a Vec7 with the specified values.
   * @param x
   * @param y
   * @param z
   * @param a
   * @param b
   * @param c
   * @param d
   */
  Vec7(T x, T y, T z, T a, T b, T c, T d)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = a;
    (*this)[4] = b;
    (*this)[5] = c;
    (*this)[6] = d;
  }

  /**
   * @brief Constructs a Vec7 by copying values from the specified array.
   * @param data
   */
  Vec7(const std::array<T, 7>& data)
  : Array<T, 7>(data)
  {
  }

  /**
   * @brief Constructs a Vec7 by copying values from the specified tuple.
   * @param data
   */
  Vec7(const tuple_type& data)
  {
    (*this)[0] = std::get<0>(data);
    (*this)[1] = std::get<1>(data);
    (*this)[2] = std::get<2>(data);
    (*this)[3] = std::get<3>(data);
    (*this)[4] = std::get<4>(data);
    (*this)[5] = std::get<5>(data);
    (*this)[6] = std::get<6>(data);
  }

  /**
   * @brief Constructs a Vec7 by copying values from the specified pointer.
   * The pointer is assumed to be an array with at least seven values.
   * @param data
   */
  Vec7(const T* data)
  : Array<T, 7>(data)
  {
  }

  /**
   * @brief Constructs a Vec7 by copying values from the specified vector.
   * The vector is assumed to contain at least seven values.
   * @param data
   */
  Vec7(const std::vector<T>& data)
  : Array<T, 7>(data)
  {
  }

  /**
   * @brief Assigns new values to the Vec7.
   * This is a convenience function that helps with the python bindings
   * @param x
   * @param y
   * @param z
   * @param i
   * @param j
   * @param k
   * @param l
   */
  void setValues(const T& x, const T& y, const T& z, const T& i, const T& j, const T& k, const T& l)
  {
    (*this)[0] = x;
    (*this)[1] = y;
    (*this)[2] = z;
    (*this)[3] = i;
    (*this)[4] = j;
    (*this)[5] = k;
    (*this)[6] = l;
  }

  /**
   * @brief Converts the internal data structure to a std::tuple
   * @return tuple_type
   */
  tuple_type toTuple() const
  {
    return std::make_tuple((*this)[0], (*this)[1], (*this)[2], (*this)[3], (*this)[4], (*this)[5], (*this)[6]);
  }

  /**
   * @brief Converts this array into another array using a static_cast<OutType> mechanism.
   * @return Vec7<OutType>
   */
  template <typename OutType>
  Vec7<OutType> convertType()
  {
    return Vec6<OutType>(static_cast<OutType>((*this)[0]), static_cast<OutType>((*this)[1]), static_cast<OutType>((*this)[2]), static_cast<OutType>((*this)[3]), static_cast<OutType>((*this)[4]),
                         static_cast<OutType>((*this)[5]), static_cast<OutType>((*this)[6]));
  }
};

using FloatVec3 = Vec3<float32>;
using IntVec3 = Vec3<int32>;
using UIntVec3 = Vec3<uint32>;
using SizeVec3 = Vec3<usize>;
using UInt64Vec3 = Vec3<uint64>;
using USizeVec3 = Vec3<usize>;

using FloatVec4 = Vec4<float32>;
using IntVec4 = Vec4<int32>;
using SizeVec4 = Vec4<usize>;
using USizeVec4 = Vec4<usize>;

using FloatVec6 = Vec6<float32>;
using IntVec6 = Vec6<int32>;

using FloatVec7 = Vec7<float32>;
using IntVec7 = Vec7<int32>;

// type aliasing to protect deprecated API
template <typename T>
using Point3D = Vec3<T>;

using Point3Df = Vec3<float32>;
using Point3Dd = Vec3<float64>;

template <typename T>
using Point2D = Vec2<T>;

using Point2Df = Vec2<float32>;
using Point2Dd = Vec2<float64>;
} // namespace nx::core
