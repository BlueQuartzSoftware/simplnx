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
  virtual ~AbstractGeometryGrid();

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
   * @return size_t
   */
  virtual size_t getNumXPoints() const = 0;

  /**
   * @brief
   * @return size_t
   */
  virtual size_t getNumYPoints() const = 0;

  /**
   * @brief
   * @return size_t
   */
  virtual size_t getNumZPoints() const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getPlaneCoordsf(size_t idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getPlaneCoordsf(size_t x, size_t y, size_t z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getPlaneCoordsf(size_t idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getPlaneCoords(size_t idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getPlaneCoords(size_t x, size_t y, size_t z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getPlaneCoords(size_t idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<float>
   */
  virtual complex::Point3D<float> getCoordsf(size_t idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @preturn complex::Point3D<float>
   */
  virtual complex::Point3D<float> getCoordsf(size_t x, size_t y, size_t z) const = 0;

  /**
   * @brief
   * @param idx
   * @return
   */
  virtual complex::Point3D<float> getCoordsf(size_t idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getCoords(size_t idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getCoords(size_t x, size_t y, size_t z) const = 0;

  /**
   * @brief
   * @param idx
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getCoords(size_t idx) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return size_t
   */
  virtual size_t getIndex(float xCoord, float yCoord, float zCoord) const = 0;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   * @return size_t
   */
  virtual size_t getIndex(double xCoord, double yCoord, double zCoord) const = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometryGrid(DataStructure& ds, const std::string& name);

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
