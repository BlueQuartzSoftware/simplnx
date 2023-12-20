#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ResampleRectGridToImageGeomInputValues
{
  DataPath RectilinearGridPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  VectorInt32Parameter::ValueType Dimensions;
  DataPath ImageGeometryPath;
  std::string ImageGeomCellAttributeMatrixName;
};

/**
 * @class ResampleRectGridToImageGeom
 * @brief This filter will re-sample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into the newly created Image Geometry Data Container during the process.
 */

class SIMPLNXCORE_EXPORT ResampleRectGridToImageGeom
{
public:
  ResampleRectGridToImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ResampleRectGridToImageGeomInputValues* inputValues);
  ~ResampleRectGridToImageGeom() noexcept;

  ResampleRectGridToImageGeom(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom(ResampleRectGridToImageGeom&&) noexcept = delete;
  ResampleRectGridToImageGeom& operator=(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom& operator=(ResampleRectGridToImageGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ResampleRectGridToImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
