#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT GenerateGBCDPoleFigureInputValues
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
class ORIENTATIONANALYSIS_EXPORT GenerateGBCDPoleFigure
{
public:
  GenerateGBCDPoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateGBCDPoleFigureInputValues* inputValues);
  ~GenerateGBCDPoleFigure() noexcept;

  GenerateGBCDPoleFigure(const GenerateGBCDPoleFigure&) = delete;
  GenerateGBCDPoleFigure(GenerateGBCDPoleFigure&&) noexcept = delete;
  GenerateGBCDPoleFigure& operator=(const GenerateGBCDPoleFigure&) = delete;
  GenerateGBCDPoleFigure& operator=(GenerateGBCDPoleFigure&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateGBCDPoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
