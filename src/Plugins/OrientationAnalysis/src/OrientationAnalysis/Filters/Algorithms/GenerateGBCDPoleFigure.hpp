#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
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

} // namespace complex
