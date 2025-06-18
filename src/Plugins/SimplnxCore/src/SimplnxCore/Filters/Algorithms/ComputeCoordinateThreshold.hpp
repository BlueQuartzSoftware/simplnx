#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeCoordinateThresholdInputValues
{
  ChoicesParameter::ValueType ShapeType;
  bool Invert;
  VectorFloat32Parameter::ValueType MinCoord;
  VectorFloat32Parameter::ValueType MaxCoord;
  VectorFloat32Parameter::ValueType SphereInfo;
  DataPath GeometryPath;
  DataPath MaskArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeCoordinateThreshold
{
public:
  ComputeCoordinateThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCoordinateThresholdInputValues* inputValues);
  ~ComputeCoordinateThreshold() noexcept;

  ComputeCoordinateThreshold(const ComputeCoordinateThreshold&) = delete;
  ComputeCoordinateThreshold(ComputeCoordinateThreshold&&) noexcept = delete;
  ComputeCoordinateThreshold& operator=(const ComputeCoordinateThreshold&) = delete;
  ComputeCoordinateThreshold& operator=(ComputeCoordinateThreshold&&) noexcept = delete;

  enum BoundsType : uint8
  {
    Rectangle = 0,
    Sphere = 1
  };

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinateThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
