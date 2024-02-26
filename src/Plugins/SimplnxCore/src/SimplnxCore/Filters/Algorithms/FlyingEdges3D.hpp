#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FlyingEdges3DInputValues
{
  DataPath imageGeomPath;
  DataPath triangleGeomPath;
  DataPath contouringArrayPath;
  DataPath normalsArrayPath;
  float64 isoVal;
};

/**
 * @class FlyingEdges3D
 * @brief This filter draw a 3 dimensional contouring line through an Image Geometry based on an input value.
 */
class SIMPLNXCORE_EXPORT FlyingEdges3D
{
public:
  FlyingEdges3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FlyingEdges3DInputValues* inputValues);
  ~FlyingEdges3D() noexcept;

  FlyingEdges3D(const FlyingEdges3D&) = delete;
  FlyingEdges3D(FlyingEdges3D&&) noexcept = delete;
  FlyingEdges3D& operator=(const FlyingEdges3D&) = delete;
  FlyingEdges3D& operator=(FlyingEdges3D&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FlyingEdges3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
