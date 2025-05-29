#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeCoordinatesImageGeomInputValues
{
  ChoicesParameter::ValueType CoordinateOption;
  DataPath ImageGeomPath;
  DataPath CoordArrayPath;
  DataPath IndexArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeCoordinatesImageGeom
{
public:
  ComputeCoordinatesImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCoordinatesImageGeomInputValues* inputValues);
  ~ComputeCoordinatesImageGeom() noexcept;

  ComputeCoordinatesImageGeom(const ComputeCoordinatesImageGeom&) = delete;
  ComputeCoordinatesImageGeom(ComputeCoordinatesImageGeom&&) noexcept = delete;
  ComputeCoordinatesImageGeom& operator=(const ComputeCoordinatesImageGeom&) = delete;
  ComputeCoordinatesImageGeom& operator=(ComputeCoordinatesImageGeom&&) noexcept = delete;

  enum OutputType : uint8
  {
    Physical = 0,
    Index = 1,
    Both = 2
  };

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinatesImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
