#pragma once

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT IGridGeometry : public IGeometry
{
public:
  static inline constexpr StringLiteral k_CellDataName = "Cell Data";

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

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getCellDataId() const
  {
    return m_CellDataId;
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getCellData()
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getCellData() const
  {
    return getDataStructureRef().getDataAs<AttributeMatrix>(m_CellDataId);
  }

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getCellDataRef()
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
  }

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getCellDataRef() const
  {
    return getDataStructureRef().getDataRefAs<AttributeMatrix>(m_CellDataId.value());
  }

  /**
   * @brief
   * @return
   */
  DataPath getCellDataPath() const
  {
    return getCellDataRef().getDataPaths().at(0);
  }

  /**
   * @brief
   * @param attributeMatrix
   */
  void setCellData(const AttributeMatrix& attributeMatrix)
  {
    m_CellDataId = attributeMatrix.getId();
  }

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override
  {
    H5::ErrorType error = IGeometry::readHdf5(dataStructureReader, groupReader, preflight);
    if(error < 0)
    {
      return error;
    }

    m_CellDataId = ReadH5DataId(groupReader, H5Constants::k_CellDataTag);

    return error;
  }

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override
  {
    H5::ErrorType error = IGeometry::writeHdf5(dataStructureWriter, parentGroupWriter, importable);
    if(error < 0)
    {
      return error;
    }

    H5::GroupWriter groupWriter = parentGroupWriter.createGroupWriter(getName());
    error = WriteH5DataId(groupWriter, m_CellDataId, H5Constants::k_CellDataTag);
    if(error < 0)
    {
      return error;
    }

    return error;
  }

protected:
  IGridGeometry() = delete;

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

  IGridGeometry& operator=(const IGridGeometry&) = delete;
  IGridGeometry& operator=(IGridGeometry&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override
  {
    IGeometry::checkUpdatedIdsImpl(updatedIds);

    for(const auto& updatedId : updatedIds)
    {
      if(m_CellDataId == updatedId.first)
      {
        m_CellDataId = updatedId.second;
      }
    }
  }

private:
  std::optional<IdType> m_CellDataId;
};
} // namespace complex
