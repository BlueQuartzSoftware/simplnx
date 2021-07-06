#pragma once

#include <memory>

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DynamicListArray.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class ITransformContainer;
class Observable;

/**
 * @brief The InfoStringFormat enum specifies which format should be used when
 * generating the geometries information string.
 */
enum class InfoStringFormat : uint8_t
{
  HtmlFormat = 0,
  MarkDown = 1,
  // JsonFormat,
  // TextFormat,
  // XmlFormat,
  UnknownFormat
};

/**
 * @class AbstractGeometry
 * @brief
 */
class COMPLEX_EXPORT AbstractGeometry : public BaseGroup
{
public:
  friend class DataStructure;

  using EnumType = uint32_t;
  using StatusCode = int32_t;

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

  using MeshIndexType = size_t;
  using MeshIndexArrayType = DataArray<MeshIndexType>;
  using SharedVertexList = FloatArray;
  using SharedEdgeList = MeshIndexArrayType;
  using SharedTriList = MeshIndexArrayType;
  using SharedQuadList = MeshIndexArrayType;
  using SharedTetList = MeshIndexArrayType;
  using SharedHexList = MeshIndexArrayType;
  using SharedFaceList = MeshIndexArrayType;
  using ElementDynamicList = DynamicListArray<uint16_t, MeshIndexType>;

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

  virtual ~AbstractGeometry();

  /**
   * @brief
   * @return float
   */
  float getTimeValue() const;

  /**
   * @brief
   * @param value
   */
  void setTimeValue(float value);

  /**
   * @brief
   * @return bool
   */
  bool getTimeSeriesEnabled() const;

  /**
   * @brief
   * @param value
   */
  void setTimeSeriesEnabled(bool value);

  /**
   * @brief
   * @return ITransformContainer*
   */
  // ITransformContainer* getTransformContainer() const;

  /**
   * @brief
   * @param container
   */
  // void setTransformContainer(const ITransformContainer* container);

  /**
   * @brief
   * @return LengthUnit
   */
  LengthUnit getUnits() const;

  /**
   * @brief
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
   * @return size_t
   */
  virtual size_t getNumberOfElements() const = 0;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementSizes() = 0;

  /**
   * @brief
   * @return const FloatArray*
   */
  virtual const FloatArray* getElementSizes() const = 0;

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
   * @return const FloatArray*
   */
  virtual const FloatArray* getElementCentroids() const = 0;

  /**
   * @brief
   */
  virtual void deleteElementCentroids() = 0;

  /**
   * @brief
   * @return complex::Point3D<double>
   */
  virtual complex::Point3D<double> getParametricCenter() const = 0;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  virtual void getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const = 0;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  virtual void findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable) const = 0;

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
   * @return uint32_t
   */
  virtual uint32_t getXdmfGridType() const = 0;

  /**
   * @brief
   * @return uint32_t
   */
  uint32_t getUnitDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setUnitDimensionality(uint32_t value);

  /**
   * @brief
   * @return uint32_t
   */
  uint32_t getSpatialDimensionality() const;

  /**
   * @brief
   * @param value
   */
  void setSpatialDimensionality(uint32_t value);

  //////////
  // HDF5 //
  //////////

  ///**
  // * @brief
  // * @param parentId
  // * @param generateXdmfText
  // * @return StatusCode
  // */
  // virtual StatusCode writeGeometryToHDF5(hid_t parentId, bool generateXdmfText) const = 0;

  ///**
  // * @brief
  // * @param ostream& out
  // * @param string dcName
  // * @param string hdfFileName
  // * @return StatusCode
  // */
  // virtual StatusCode generateXdmfText() const = 0;

  ///**
  // * @brief
  // * @param parentId
  // * @param hid_t
  // * @param preflight
  // * @return StatusCode
  // */
  // virtual StatusCode readGeometryFromHDF5(bool preflight) = 0;

  //////////
  // HDF5 //
  //////////

  /**
   * @brief
   */
  virtual void initializeWithZeros() = 0;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  AbstractGeometry(DataStructure* ds, const std::string& name);

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
  virtual void setElementCentroids(const FloatArray* elementCentroids) = 0;

  /**
   * @brief
   * @param elementSizes
   */
  virtual void setElementSizes(const FloatArray* elementSizes) = 0;

  /**
   * @brief
   * @param numEdges
   * @return SharedEdgeList*
   */
  SharedEdgeList* createSharedEdgeList(size_t numEdges);

  /**
   * @brief
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

private:
  LengthUnit m_Units = LengthUnit::Meter;
  bool m_IsTimeSeriesEnabled = false;
  float m_TimeValue = 0.0f;
  uint32_t m_UnitDimensionality;
  uint32_t m_SpacialDimensionality;
};
} // namespace complex
