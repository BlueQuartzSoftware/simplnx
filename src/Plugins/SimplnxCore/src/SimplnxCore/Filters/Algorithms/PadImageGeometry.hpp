#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace pad_image_geometry
{
const std::string k_TempGeometryName = ".cropped_image_geometry";

}

namespace nx::core
{

struct SIMPLNXCORE_EXPORT PadImageGeometryInputValues
{
  DataPath SelectedImageGeometryPath;
  DataPath CreatedOutputPath;
  VectorInt32Parameter::ValueType XMinMax;
  VectorInt32Parameter::ValueType YMinMax;
  VectorInt32Parameter::ValueType ZMinMax;
  bool PadInX;
  bool PadInY;
  bool PadInZ;
  int32 DefaultFillValue;
  bool UpdateOrigin;
  DataPath AttributeMatrixPath;
  bool RemoveOriginalGeometry;
};

/**
 * @class PadImageGeometry
 * @brief This algorithm implements support code for the PadImageGeometryFilter
 */

class SIMPLNXCORE_EXPORT PadImageGeometry
{
public:
  PadImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PadImageGeometryInputValues* inputValues);
  ~PadImageGeometry() noexcept;

  PadImageGeometry(const PadImageGeometry&) = delete;
  PadImageGeometry(PadImageGeometry&&) noexcept = delete;
  PadImageGeometry& operator=(const PadImageGeometry&) = delete;
  PadImageGeometry& operator=(PadImageGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PadImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
