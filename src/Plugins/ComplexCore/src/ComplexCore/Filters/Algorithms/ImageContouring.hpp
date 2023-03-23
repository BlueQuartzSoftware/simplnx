#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ImageContouringInputValues
{
  DataPath imageGeomPath;
  DataPath triangleGeomPath;
  DataPath contouringArrayPath;
  DataPath normalsArrayPath;
  float64 isoVal;
};

/**
 * @class ImageContouring
 * @brief This filter...
 */
class COMPLEXCORE_EXPORT ImageContouring
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

} // namespace complex
