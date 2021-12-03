#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class AbstractGeometryGrid
 * @brief
 */
class COMPLEX_EXPORT AbstractGeometryGrid : public AbstractGeometry
{
public:
  ~AbstractGeometryGrid() override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::DataObjectType getType() const override;

  /**
   * @brief
   * @return SizeVec3
   */
  virtual complex::SizeVec3 getDimensions() const = 0;

  /**
   * @brief
   * @param dims
   */
  virtual void setDimensions(const complex::SizeVec3& dims) = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumXPoints() const = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumYPoints() const = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumZPoints() const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  virtual complex::Point3D<float32> getPlaneCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float32>
   */
  virtual complex::Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  virtual complex::Point3D<float32> getPlaneCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getPlaneCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getPlaneCoords(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float32>
   */
  virtual complex::Point3D<float32> getCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @preturn complex::Point3D<float32>
   */
  virtual complex::Point3D<float32> getCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return
   */
  virtual complex::Point3D<float32> getCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getCoords(usize idx) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return usize
   */
  virtual usize getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return usize
   */
  virtual usize getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometryGrid(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  AbstractGeometryGrid(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief
   * @param other
   */
  AbstractGeometryGrid(const AbstractGeometryGrid& other);

  /**
   * @brief
   * @param other
   * @return
   */
  AbstractGeometryGrid(AbstractGeometryGrid&& other) noexcept;

private:
};
} // namespace complex
