#pragma once

#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT IGridGeometry : public IGeometry
{
public:
  ~IGridGeometry() noexcept override = default;

  /**
   * @brief
   * @return SizeVec3
   */
  virtual SizeVec3 getDimensions() const = 0;

  /**
   * @brief
   * @param dims
   */
  virtual void setDimensions(const SizeVec3& dims) = 0;

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
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getPlaneCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getPlaneCoords(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  virtual Point3D<float32> getCoordsf(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @preturn Point3D<float32>
   */
  virtual Point3D<float32> getCoordsf(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return
   */
  virtual Point3D<float32> getCoordsf(usize idx) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize idx[3]) const = 0;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize x, usize y, usize z) const = 0;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  virtual Point3D<float64> getCoords(usize idx) const = 0;

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
  IGridGeometry() = default;

  IGridGeometry(DataStructure& ds, std::string name)
  : IGeometry(ds, std::move(name))
  {
  }

  IGridGeometry(DataStructure& ds, std::string name, IdType importId)
  : IGeometry(ds, std::move(name), importId)
  {
  }

  IGridGeometry(const IGridGeometry&) = default;
  IGridGeometry(IGridGeometry&&) noexcept = default;

  IGridGeometry& operator=(const IGridGeometry&) = default;
  IGridGeometry& operator=(IGridGeometry&&) noexcept = default;
};
} // namespace complex
