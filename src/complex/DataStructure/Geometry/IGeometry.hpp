#pragma once

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/DataStructure/Geometry/LinkedGeometryData.hpp"

namespace complex
{
class COMPLEX_EXPORT IGeometry : public BaseGroup
{
public:
  friend class DataStructure;

  using StatusCode = int32;

  using MeshIndexType = uint64;
  using MeshIndexArrayType = DataArray<MeshIndexType>;
  using SharedVertexList = Float32Array;
  using SharedEdgeList = MeshIndexArrayType;
  using SharedFaceList = MeshIndexArrayType;
  using SharedTriList = MeshIndexArrayType;
  using SharedQuadList = MeshIndexArrayType;
  using SharedTetList = MeshIndexArrayType;
  using SharedHexList = MeshIndexArrayType;
  using ElementDynamicList = DynamicListArray<uint16, MeshIndexType>;

  enum class Type : uint32
  {
    Image,
    RectGrid,
    Vertex,
    Edge,
    Triangle,
    Quad,
    Tetrahedral,
    Hexahedral
  };

  enum class LengthUnit : EnumType
  {
    Yoctometer,
    Zeptometer,
    Attometer,
    Femtometer,
    Picometer,
    Nanometer,
    Micrometer,
    Millimeter,
    Centimeter,
    Decimeter,
    Meter,
    Decameter,
    Hectometer,
    Kilometer,
    Megameter,
    Gigameter,
    Terameter,
    Petameter,
    Exameter,
    Zettameter,
    Yottameter,
    Angstrom,
    Mil,
    Inch,
    Foot,
    Mile,
    Fathom,
    Unspecified = 100U,
    Unknown = 101U
  };

  ~IGeometry() noexcept override = default;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  virtual IGeometry::Type getGeomType() const = 0;

  /**
   * @brief
   * @return usize
   */
  virtual usize getNumberOfElements() const = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementSizes() = 0;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getElementSizes() const;

  /**
   * @brief
   */
  void deleteElementSizes();

  /**
   * @brief
   * @return complex::Point3D<float64>
   */
  virtual Point3D<float64> getParametricCenter() const = 0;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  virtual void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const = 0;

  /**
   * @brief
   * @return uint32
   */
  uint32 getUnitDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setUnitDimensionality(uint32 value);

  /**
   * @brief
   * @return uint32
   */
  uint32 getSpatialDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setSpatialDimensionality(uint32 value);

  static std::set<std::string> StringListFromGeometryType(const std::set<Type>& geomTypes);

  static const std::set<Type>& GetAllGeomTypes();

  /**
   * @brief Returns the DataPaths of DataArrays that are associated or "Linked" to
   * a specific part of the geometry. @see the docs for LinkedGeometryPath.
   * @return
   */
  const LinkedGeometryData& getLinkedGeometryData() const;

  /**
   * @brief Returns the DataPaths of DataArrays that are associated or "Linked" to
   * a specific part of the geometry. @see the docs for LinkedGeometryPath. Non-const version
   * @return
   */
  LinkedGeometryData& getLinkedGeometryData();

  /**
   * @brief Returns the length units used by the geometry.
   * @return LengthUnit
   */
  LengthUnit getUnits() const;

  /**
   * @brief Sets the length units used by the geometry.
   * @param units
   */
  void setUnits(LengthUnit units);

  /**
   * @brief
   * @param unit
   * @return std::string
   */
  static std::string LengthUnitToString(LengthUnit unit);

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  IGeometry() = delete;

  IGeometry(DataStructure& ds, std::string name);

  IGeometry(DataStructure& ds, std::string name, IdType importId);

  IGeometry(const IGeometry&) = default;
  IGeometry(IGeometry&&) = default;

  IGeometry& operator=(const IGeometry&) = delete;
  IGeometry& operator=(IGeometry&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /**
   * @brief Reads an optional DataObject ID from HDF5.
   * @param objectReader
   * @param attributeName
   * @return std::optional<DataObject::IdType>
   */
  static std::optional<IdType> ReadH5DataId(const H5::ObjectReader& objectReader, const std::string& attributeName);

  /**
   * @brief Writes an optional DataObject ID to HDF5. Returns an error code if
   * a problem occurred. Returns 0 otherwise.
   * @param objectWriter
   * @param dataId
   * @param attributeName
   * @return H5::ErrorType
   */
  static H5::ErrorType WriteH5DataId(H5::ObjectWriter& objectWriter, const std::optional<IdType>& dataId, const std::string& attributeName);

  std::optional<IdType> m_ElementSizesId;

  LengthUnit m_Units = LengthUnit::Meter;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;

  LinkedGeometryData m_LinkedGeometryData;
};
} // namespace complex
