#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ResampleRectGridToImageGeomInputValues
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

class COMPLEXCORE_EXPORT ResampleRectGridToImageGeom
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

} // namespace complex
