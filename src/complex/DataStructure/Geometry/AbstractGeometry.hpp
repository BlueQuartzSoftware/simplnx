#pragma once

#include <memory>
#include <optional>

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"
#include "complex/DataStructure/Geometry/LinkedGeometryData.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class ITransformContainer;
class Observable;

/**
 * @brief The InfoStringFormat enum specifies which format should be used when
 * generating the geometries information string.
 */
enum class InfoStringFormat : uint8
{
  HtmlFormat = 0,
  MarkDown = 1,
  // JsonFormat,
  // TextFormat,
  // XmlFormat,
  UnknownFormat
};

namespace H5
{
class ObjectReader;
} // namespace H5

/**
 * @class AbstractGeometry
 * @brief
 */
class COMPLEX_EXPORT AbstractGeometry : public BaseGroup
{
public:
  friend class DataStructure;

  using EnumType = uint32;
  using StatusCode = int32;

  /**
   * @brief The VtkCellType enum specifies a type of VTK geometry the geometry
   * best relates to.
   */
  enum class VtkCellType : EnumType
  {
    Image = 11,
    RectGrid = 11,
    Vertex = 1,
    Edge = 3,
    Triangle = 5,

    Quad = 9,
    Tetrahedral = 10,
    Hexahedral = 12,
    Unknown = 999,
    Any = 4294967295U
  };

  /**
   * @brief The type of geometry
   */
  enum class Type : EnumType
  {
    Image,
    RectGrid,
    Vertex,
    Edge,
    Triangle,
    Quad,
    Tetrahedral,
    Hexahedral,
    Unknown = 999,
    Any = 4294967295U
  };

  static std::set<std::string> StringListFromGeometryType(const std::set<Type>& geomTypes)
  {
    static std::map<Type, std::string> k_TypeToStringMap = {
        {Type::Image, "ImageGeom"}, {Type::RectGrid, "RectGrid"},       {Type::Vertex, "Vertex"},         {Type::Edge, "Edge"},       {Type::Triangle, "Triangle"},
        {Type::Quad, "Quad"},       {Type::Tetrahedral, "Tetrahedral"}, {Type::Hexahedral, "Hexahedral"}, {Type::Unknown, "Unknown"}, {Type::Any, "Any"}};

    std::set<std::string> stringValues;
    for(const auto& geomType : geomTypes)
    {
      stringValues.insert(k_TypeToStringMap[geomType]);
    }
    return stringValues;
  }

  /**
   * @brief Specifies which unit should be used for length measurements.
   */
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

  /**
   * @brief
   * @param unit
   * @return std::string
   */
  static std::string LengthUnitToString(LengthUnit unit);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry(const AbstractGeometry& other);

  /**
   * @brief
   * @param other
   */
  AbstractGeometry(AbstractGeometry&& other) noexcept;

  ~AbstractGeometry() override;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  virtual AbstractGeometry::Type getGeomType() const = 0;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief
   * @return float
   */
  float32 getTimeValue() const;

  /**
   * @brief
   * @param value
   */
  void setTimeValue(float32 value);

  /**
   * @brief Returns true if time series are enabled. Returns false otherwise.
   * @return bool
   */
  bool getTimeSeriesEnabled() const;

  /**
   * @brief Sets whether or not time series are enabled.
   * @param value
   */
  void setTimeSeriesEnabled(bool value);

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
   * @return StatusCode
   */
  virtual StatusCode findElementsContainingVert() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  virtual const ElementDynamicList* getElementsContainingVert() const = 0;

  /**
   * @brief
   */
  virtual void deleteElementsContainingVert() = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementNeighbors() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  virtual const ElementDynamicList* getElementNeighbors() const = 0;

  /**
   * @brief
   */
  virtual void deleteElementNeighbors() = 0;

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
  virtual const Float32Array* getElementSizes() const = 0;

  /**
   * @brief
   */
  virtual void deleteElementSizes() = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementCentroids() = 0;

  /**
   * @brief
   * @return const Float32Array*
   */
  virtual const Float32Array* getElementCentroids() const = 0;

  /**
   * @brief
   */
  virtual void deleteElementCentroids() = 0;

  /**
   * @brief
   * @return complex::Point3D<float64>
   */
  virtual complex::Point3D<float64> getParametricCenter() const = 0;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  virtual void getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const = 0;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  virtual void findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable) const = 0;

  /**
   * @brief
   * @return std::string
   */
  virtual std::string getGeometryTypeAsString() const = 0;

  /**
   * @brief
   * @param complex::InfoStringFormat
   * @return std::string
   */
  virtual std::string getInfoString(complex::InfoStringFormat format) const;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  virtual complex::TooltipGenerator getTooltipGenerator() const = 0;

  /**
   * @brief
   * @return uint32
   */
  virtual uint32 getXdmfGridType() const = 0;

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

  /**
   * @brief
   */
  virtual void initializeWithZeros() = 0;

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

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  AbstractGeometry(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief
   * @param elementsContainingVert
   */
  virtual void setElementsContainingVert(const ElementDynamicList* elementsContainingVert) = 0;

  /**
   * @brief
   * @param elementsNeighbors
   */
  virtual void setElementNeighbors(const ElementDynamicList* elementsNeighbors) = 0;

  /**
   * @brief
   * @param elementCentroids
   */
  virtual void setElementCentroids(const Float32Array* elementCentroids) = 0;

  /**
   * @brief
   * @param elementSizes
   */
  virtual void setElementSizes(const Float32Array* elementSizes) = 0;

  /**
   * @brief
   * @param numEdges
   * @return SharedEdgeList*
   */
  SharedEdgeList* createSharedEdgeList(usize numEdges);

  /**
   * @brief
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

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

private:
  LengthUnit m_Units = LengthUnit::Meter;
  bool m_IsTimeSeriesEnabled = false;
  float32 m_TimeValue = 0.0f;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;
  LinkedGeometryData m_LinkedGeometryData;
};
} // namespace complex
