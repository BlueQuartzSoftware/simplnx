#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ImageContouringInputValues
{
  DataPath imageGeomPath;
  DataPath triangleGeomPath;
  DataPath contouringArrayPath;
  DataPath normalsArrayPath;
  float64 isoVal;
};

/**
 * @class ImageContouring
 * @brief This filter draw a 3 dimensional contouring line through an Image Geometry based on an input value.
 */
class SIMPLNXCORE_EXPORT ImageContouring
{
public:
  ImageContouring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImageContouringInputValues* inputValues);
  ~ImageContouring() noexcept;

  ImageContouring(const ImageContouring&) = delete;
  ImageContouring(ImageContouring&&) noexcept = delete;
  ImageContouring& operator=(const ImageContouring&) = delete;
  ImageContouring& operator=(ImageContouring&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImageContouringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
