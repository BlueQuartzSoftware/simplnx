#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RotateSampleRefFrameInputValues
{
  bool SliceBySlice;
  ChoicesParameter::ValueType RotationRepresentationIndex;
  DataPath SourceGeometryPath;
  DataPath DestGeometryPath;
  VectorFloat32Parameter::ValueType RotationAxisAngle;
  DynamicTableParameter::ValueType RotationMatrixTable;
  bool KeepInputGeometryOrigin;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT RotateSampleRefFrame
{
public:
  RotateSampleRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateSampleRefFrameInputValues* inputValues);
  ~RotateSampleRefFrame() noexcept;

  RotateSampleRefFrame(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame(RotateSampleRefFrame&&) noexcept = delete;
  RotateSampleRefFrame& operator=(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame& operator=(RotateSampleRefFrame&&) noexcept = delete;

  enum class RotationRepresentation : uint64
  {
    AxisAngle = 0,
    RotationMatrix = 1
  };

  Result<> operator()();
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RotateSampleRefFrameInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
