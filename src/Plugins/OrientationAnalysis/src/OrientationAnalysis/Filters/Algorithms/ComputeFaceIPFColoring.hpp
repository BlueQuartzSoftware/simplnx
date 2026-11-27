#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

namespace nx::core
{

/**
 * @struct ComputeFaceIPFColoringInputValues
 * @brief Identifies the arrays and color key for face IPF coloring.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFaceIPFColoringInputValues
{
  DataPath surface_mesh_face_labels_array_path;
  DataPath surface_mesh_face_normals_array_path;
  DataPath feature_euler_angles_array_path;
  DataPath feature_phases_array_path;
  DataPath crystal_structures_array_path;
  std::string first_face_ipf_colors_array_name;
  std::string second_face_ipf_colors_array_name;
  ebsdlib::ColorKeyKind color_key = ebsdlib::ColorKeyKind::TSL;
};

/**
 * @class ComputeFaceIPFColoring
 * @brief Calculates an IPF color for each side of every surface-mesh face.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFaceIPFColoring
{
public:
  /**
   * @brief Initializes face IPF coloring.
   * @param dataStructure Provides the selected arrays and output colors.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and color key.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this executor.
   */
  ComputeFaceIPFColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFaceIPFColoringInputValues* inputValues);

  /**
   * @brief Destroys the face IPF coloring executor.
   */
  ~ComputeFaceIPFColoring() noexcept = default;

  ComputeFaceIPFColoring(const ComputeFaceIPFColoring&) = delete;
  ComputeFaceIPFColoring(ComputeFaceIPFColoring&&) noexcept = delete;
  ComputeFaceIPFColoring& operator=(const ComputeFaceIPFColoring&) = delete;
  ComputeFaceIPFColoring& operator=(ComputeFaceIPFColoring&&) noexcept = delete;

  /**
   * @brief Calculates face IPF colors.
   * @return Success, or an error for an invalid referenced Phase index or bulk I/O.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFaceIPFColoringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
