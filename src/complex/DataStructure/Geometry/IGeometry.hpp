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
  const Float32Array* getElementSizes() const
  {
    return getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
  }

  /**
   * @brief
   */
  void deleteElementSizes()
  {
    getDataStructureRef().removeData(m_ElementSizesId);
    m_ElementSizesId.reset();
  }

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
  uint32 getUnitDimensionality() const
  {
    return m_UnitDimensionality;
  }

  /**
   * @brief
   * @param value
   */
  void setUnitDimensionality(uint32 value)
  {
    m_UnitDimensionality = value;
  }

  /**
   * @brief
   * @return uint32
   */
  uint32 getSpatialDimensionality() const
  {
    return m_SpacialDimensionality;
  }

  /**
   * @brief
   * @param value
   */
  void setSpatialDimensionality(uint32 value)
  {
    m_SpacialDimensionality = value;
  }

  static std::set<std::string> StringListFromGeometryType(const std::set<Type>& geomTypes)
  {
    static const std::map<Type, std::string> k_TypeToStringMap = {{Type::Image, "ImageGeom"},   {Type::RectGrid, "RectGrid"}, {Type::Vertex, "Vertex"},           {Type::Edge, "Edge"},
                                                                  {Type::Triangle, "Triangle"}, {Type::Quad, "Quad"},         {Type::Tetrahedral, "Tetrahedral"}, {Type::Hexahedral, "Hexahedral"}};

    std::set<std::string> stringValues;
    for(auto geomType : geomTypes)
    {
      stringValues.insert(k_TypeToStringMap.at(geomType));
    }
    return stringValues;
  }

  static const std::set<Type>& GetAllGeomTypes()
  {
    static const std::set<Type> types = {Type::Image, Type::RectGrid, Type::Vertex, Type::Edge, Type::Triangle, Type::Quad, Type::Tetrahedral, Type::Hexahedral};
    return types;
  }

  /**
   * @brief Returns the DataPaths of DataArrays that are associated or "Linked" to
   * a specific part of the geometry. @see the docs for LinkedGeometryPath.
   * @return
   */
  const LinkedGeometryData& getLinkedGeometryData() const
  {
    return m_LinkedGeometryData;
  }

  /**
   * @brief Returns the DataPaths of DataArrays that are associated or "Linked" to
   * a specific part of the geometry. @see the docs for LinkedGeometryPath. Non-const version
   * @return
   */
  LinkedGeometryData& getLinkedGeometryData()
  {
    return m_LinkedGeometryData;
  }

  /**
   * @brief Returns the length units used by the geometry.
   * @return LengthUnit
   */
  LengthUnit getUnits() const
  {
    return m_Units;
  }

  /**
   * @brief Sets the length units used by the geometry.
   * @param units
   */
  void setUnits(LengthUnit units)
  {
    m_Units = units;
  }

  /**
   * @brief
   * @param unit
   * @return std::string
   */
  static std::string LengthUnitToString(LengthUnit unit)
  {
    switch(unit)
    {
    case LengthUnit::Yoctometer: {
      return "Yoctometer";
    }
    case LengthUnit::Zeptometer: {
      return "Zeptometer";
    }
    case LengthUnit::Attometer: {
      return "Attometer";
    }
    case LengthUnit::Femtometer: {
      return "Femtometer";
    }
    case LengthUnit::Picometer: {
      return "Picometer";
    }
    case LengthUnit::Nanometer: {
      return "Nanometer";
    }
    case LengthUnit::Micrometer: {
      return "Micrometer";
    }
    case LengthUnit::Millimeter: {
      return "Millimeter";
    }
    case LengthUnit::Centimeter: {
      return "Centimeter";
    }
    case LengthUnit::Decimeter: {
      return "Decimeter";
    }
    case LengthUnit::Meter: {
      return "Meter";
    }
    case LengthUnit::Decameter: {
      return "Decameter";
    }
    case LengthUnit::Hectometer: {
      return "Hectometer";
    }
    case LengthUnit::Kilometer: {
      return "Kilometer";
    }
    case LengthUnit::Megameter: {
      return "Megameter";
    }
    case LengthUnit::Gigameter: {
      return "Gigameter";
    }
    case LengthUnit::Terameter: {
      return "Terameter";
    }
    case LengthUnit::Petameter: {
      return "Petameter";
    }
    case LengthUnit::Exameter: {
      return "Exameter";
    }
    case LengthUnit::Zettameter: {
      return "Zettameter";
    }
    case LengthUnit::Yottameter: {
      return "Yottameter";
    }
    case LengthUnit::Angstrom: {
      return "Angstrom";
    }
    case LengthUnit::Mil: {
      return "Mil";
    }
    case LengthUnit::Inch: {
      return "Inch";
    }
    case LengthUnit::Foot: {
      return "Foot";
    }
    case LengthUnit::Mile: {
      return "Mile";
    }
    case LengthUnit::Fathom: {
      return "Fathom";
    }
    case LengthUnit::Unspecified: {
      return "Unspecified";
    }
    case LengthUnit::Unknown: {
      return "Unknown";
    }
    }
    return "Unknown";
  }

protected:
  IGeometry() = default;

  IGeometry(DataStructure& ds, std::string name)
  : BaseGroup(ds, std::move(name))
  {
  }

  IGeometry(DataStructure& ds, std::string name, IdType importId)
  : BaseGroup(ds, std::move(name), importId)
  {
  }

  IGeometry(const IGeometry&) = default;
  IGeometry(IGeometry&&) noexcept = default;

  IGeometry& operator=(const IGeometry&) = delete;
  IGeometry& operator=(IGeometry&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    BaseGroup::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_ElementSizesId == updatedId.first)
      {
        m_ElementSizesId = updatedId.second;
      }
    }
  }

  /**
   * @brief Reads an optional DataObject ID from HDF5.
   * @param objectReader
   * @param attributeName
   * @return std::optional<DataObject::IdType>
   */
  static std::optional<IdType> ReadH5DataId(const H5::ObjectReader& objectReader, const std::string& attributeName)
  {
    if(!objectReader.isValid())
    {
      return {};
    }

    auto attribute = objectReader.getAttribute(attributeName);
    auto id = attribute.readAsValue<IdType>();
    if(id == 0)
    {
      return {};
    }
    return id;
  }

  /**
   * @brief Writes an optional DataObject ID to HDF5. Returns an error code if
   * a problem occurred. Returns 0 otherwise.
   * @param objectWriter
   * @param dataId
   * @param attributeName
   * @return H5::ErrorType
   */
  static H5::ErrorType WriteH5DataId(H5::ObjectWriter& objectWriter, const std::optional<IdType>& dataId, const std::string& attributeName)
  {
    if(!objectWriter.isValid())
    {
      return -1;
    }

    auto attribute = objectWriter.createAttribute(attributeName);
    if(dataId.has_value())
    {
      return attribute.writeValue<IdType>(dataId.value());
    }
    return attribute.writeValue<IdType>(0);
  }

  std::optional<IdType> m_ElementSizesId;

  LengthUnit m_Units = LengthUnit::Meter;
  uint32 m_UnitDimensionality = 3;
  uint32 m_SpacialDimensionality = 3;

  LinkedGeometryData m_LinkedGeometryData;
};
} // namespace complex
