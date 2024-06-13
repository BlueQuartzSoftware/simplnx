#pragma once

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Array.hpp"

#include <optional>

namespace nx::core
{
/**
 * @class RectGridGeom
 * @brief
 */
class SIMPLNX_EXPORT RectGridGeom : public IGridGeometry
{
public:
  friend class DataStructure;

  static inline constexpr StringLiteral k_TypeName = "RectGridGeom";

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return RectGridGeom*
   */
  static RectGridGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return RectGridGeom*
   */
  static RectGridGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  RectGridGeom(const RectGridGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  RectGridGeom(RectGridGeom&& other) = default;

  ~RectGridGeom() noexcept override = default;

  RectGridGeom& operator=(const RectGridGeom&) = delete;
  RectGridGeom& operator=(RectGridGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief
   * @param xBounds
   * @param yBounds
   * @param zBounds
   */
  void setBounds(const Float32Array* xBounds, const Float32Array* yBounds, const Float32Array* zBounds);

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array* getXBounds();

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array* getYBounds();

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array* getZBounds();

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getXBounds() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getYBounds() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getZBounds() const;

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array& getXBoundsRef();

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array& getYBoundsRef();

  /**
   * @brief
   * @return Float32Array*
   */
  Float32Array& getZBoundsRef();

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array& getXBoundsRef() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array& getYBoundsRef() const;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array& getZBoundsRef() const;

  OptionalId getXBoundsId() const;
  OptionalId getYBoundsId() const;
  OptionalId getZBoundsId() const;

  void setXBoundsId(const OptionalId& xBoundsId);
  void setYBoundsId(const OptionalId& yBoundsId);
  void setZBoundsId(const OptionalId& zBoundsId);

  /**
   * @brief
   * @return std::shared_ptr<Float32Array>
   */
  std::shared_ptr<Float32Array> getSharedXBounds();

  /**
   * @brief
   * @return std::shared_ptr<Float32Array>
   */
  std::shared_ptr<Float32Array> getSharedYBounds();

  /**
   * @brief
   * @return std::shared_ptr<Float32Array>
   */
  std::shared_ptr<Float32Array> getSharedZBounds();

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes(bool recalculate) override;

  /**
   * @brief
   * @param pCoords
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param dims
   */
  void setDimensions(const SizeVec3& dims) override;

  /**
   * @brief
   * @return SizeVec3
   */
  SizeVec3 getDimensions() const override;

  /**
   * @brief
   * @return FloatVec3
   */
  Result<FloatVec3> getOrigin() const;

  /**
   * @brief
   * @return usize
   */
  usize getNumXCells() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumYCells() const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumZCells() const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getPlaneCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getPlaneCoords(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float32>
   */
  Point3D<float32> getCoordsf(usize idx) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx[3]) const override;

  /**
   * @brief
   * @param x
   * @param y
   * @param z
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize x, usize y, usize z) const override;

  /**
   * @brief
   * @param idx
   * @return Point3D<float64>
   */
  Point3D<float64> getCoords(usize idx) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  std::optional<usize> getIndex(float32 xCoord, float32 yCoord, float32 zCoord) const override;

  /**
   * @brief
   * @param xCoord
   * @param yCoord
   * @param zCoord
   */
  std::optional<usize> getIndex(float64 xCoord, float64 yCoord, float64 zCoord) const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  RectGridGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  RectGridGeom(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

private:
  std::optional<IdType> m_xBoundsId;
  std::optional<IdType> m_yBoundsId;
  std::optional<IdType> m_zBoundsId;
  SizeVec3 m_Dimensions;
};
} // namespace nx::core
