#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureInputValues
{
  int32 PhaseOfInterest;
  VectorFloat32Parameter::ValueType MisorientationRotation;
  DataPath GBCDArrayPath;
  DataPath CrystalStructuresArrayPath;
  int32 OutputImageDimension;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string CellIntensityArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigure
{
public:
  ComputeGBCDPoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);
  ~ComputeGBCDPoleFigure() noexcept;

  ComputeGBCDPoleFigure(const ComputeGBCDPoleFigure&) = delete;
  ComputeGBCDPoleFigure(ComputeGBCDPoleFigure&&) noexcept = delete;
  ComputeGBCDPoleFigure& operator=(const ComputeGBCDPoleFigure&) = delete;
  ComputeGBCDPoleFigure& operator=(ComputeGBCDPoleFigure&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDPoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
