#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct BlendImageInputValues
 * @brief Identifies blend inputs.
 *
 * The IPF color array supplies the RGB channel values. The CI array supplies
 * the per-point quality weight used to modulate color brightness.
 */
struct ORIENTATIONANALYSIS_EXPORT BlendImageInputValues
{
  DataPath cellIPFColorsArrayPath;
  DataPath ciArrayPath;
  DataPath outputArrayPath;
  // Advanced mode (reproduces the IDL makeebsdimage_ang behavior). When false the algorithm
  // performs the generic continuous blend and every field below is ignored.
  bool advancedMode = false;
  bool useBlackout = false;
  DataPath criterionArrayPath;
  float32 blackoutThreshold = 0.0f;
  bool useLeveling = false;
  DataPath imageGeometryPath;
  int32 levelingDegree = 3;
  bool useFinalRescale = false;
};

/**
 * @class BlendImage
 * @brief Multiplies a 3-channel RGB color array element-wise by a normalized weight array.
 *
 * In Basic mode each RGB tuple is scaled by the weight value normalized to [0, 1]
 * over the full weight range (a generic continuous blend of any color array).
 *
 * In Advanced mode the routine reproduces the IDL makeebsdimage_ang behavior:
 * the weight is quantized to 256 BYTSCL levels, the final color is rounded (not
 * truncated), and the optional bad-point blackout, per-slice background leveling
 * and global final rescale are applied when enabled.
 */
class ORIENTATIONANALYSIS_EXPORT BlendImage
{
public:
  /**
   * @brief Initializes the blend executor.
   * @param dataStructure Provides selected arrays.
   * @param msgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the IPF colors, CI, and output arrays.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this executor.
   */
  BlendImage(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, BlendImageInputValues* inputValues);
  /**
   * @brief Destroys the blend executor.
   */
  ~BlendImage() noexcept;

  BlendImage(const BlendImage&) = delete;
  BlendImage(BlendImage&&) noexcept = delete;
  BlendImage& operator=(const BlendImage&) = delete;
  BlendImage& operator=(BlendImage&&) noexcept = delete;

  /**
   * @brief Executes the CI-modulated IPF color blend.
   * @return Success, or an error if the CI range is degenerate.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const BlendImageInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
