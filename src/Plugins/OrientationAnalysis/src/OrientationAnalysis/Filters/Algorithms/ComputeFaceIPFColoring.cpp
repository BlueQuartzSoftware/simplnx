#include "ComputeFaceIPFColoring.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/CubicLowOps.h>
#include <EbsdLib/LaueOps/CubicOps.h>
#include <EbsdLib/LaueOps/HexagonalLowOps.h>
#include <EbsdLib/LaueOps/HexagonalOps.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/LaueOps/MonoclinicOps.h>
#include <EbsdLib/LaueOps/OrthoRhombicOps.h>
#include <EbsdLib/LaueOps/TetragonalLowOps.h>
#include <EbsdLib/LaueOps/TetragonalOps.h>
#include <EbsdLib/LaueOps/TriclinicOps.h>
#include <EbsdLib/LaueOps/TrigonalLowOps.h>
#include <EbsdLib/LaueOps/TrigonalOps.h>

using namespace nx::core;

class CalculateFaceIPFColorsImpl
{
  const Int32Array& m_Labels;
  const Int32Array& m_Phases;
  const Float64Array& m_Normals;
  const Float32Array& m_Eulers;
  const UInt32Array& m_CrystalStructures;
  UInt8Array& m_FirstColors;
  UInt8Array& m_SecondColors;
  ebsdlib::ColorKeyKind m_ColorKey;

public:
  CalculateFaceIPFColorsImpl(const Int32Array& labels, const Int32Array& phases, const Float64Array& normals, const Float32Array& eulers, const UInt32Array& crystalStructures, UInt8Array& firstColors,
                             UInt8Array& secondColors, ebsdlib::ColorKeyKind colorKey)
  : m_Labels(labels)
  , m_Phases(phases)
  , m_Normals(normals)
  , m_Eulers(eulers)
  , m_CrystalStructures(crystalStructures)
  , m_FirstColors(firstColors)
  , m_SecondColors(secondColors)
  , m_ColorKey(colorKey)
  {
  }
  virtual ~CalculateFaceIPFColorsImpl() = default;

  void generate(usize start, usize end) const
  {
    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();

    double refDir[3] = {0.0, 0.0, 0.0};
    double dEuler[3] = {0.0, 0.0, 0.0};
    Rgba argb = 0x00000000;

    int32 feature1 = 0, feature2 = 0, phase1 = 0, phase2 = 0;
    for(usize i = start; i < end; i++)
    {
      feature1 = m_Labels[2 * i];
      feature2 = m_Labels[2 * i + 1];
      if(feature1 > 0)
      {
        phase1 = m_Phases[feature1];
      }
      else
      {
        phase1 = 0;
      }

      if(feature2 > 0)
      {
        phase2 = m_Phases[feature2];
      }
      else
      {
        phase2 = 0;
      }

      if(phase1 > 0)
      {
        // Make sure we are using a valid Euler Angles with valid crystal symmetry
        if(m_CrystalStructures[phase1] < ebsdlib::CrystalStructure::LaueGroupEnd)
        {
          dEuler[0] = m_Eulers[3 * feature1 + 0];
          dEuler[1] = m_Eulers[3 * feature1 + 1];
          dEuler[2] = m_Eulers[3 * feature1 + 2];
          refDir[0] = m_Normals[3 * i + 0];
          refDir[1] = m_Normals[3 * i + 1];
          refDir[2] = m_Normals[3 * i + 2];

          argb = ops[m_CrystalStructures[phase1]]->generateIPFColor(dEuler, refDir, false, m_ColorKey);
          m_FirstColors[3 * i] = RgbColor::dRed(argb);
          m_FirstColors[3 * i + 1] = RgbColor::dGreen(argb);
          m_FirstColors[3 * i + 2] = RgbColor::dBlue(argb);
        }
      }
      else // Phase 1 was Zero so assign a black color
      {
        m_FirstColors[3 * i + 0] = 0;
        m_FirstColors[3 * i + 1] = 0;
        m_FirstColors[3 * i + 2] = 0;
      }

      // Now compute for Phase 2
      if(phase2 > 0)
      {
        // KNOWN BUG — tracked at https://github.com/BlueQuartzSoftware/simplnx/issues/1635.
        // Both the validity guard and the IPF operator lookup below use phase1 instead of phase2,
        // assigning Phase 1's symmetry operator to Phase 2's Euler angles on mixed-phase faces.
        // The 2-line fix (phase1→phase2) is held pending a V&V cycle that will regenerate the
        // test exemplar (which currently encodes the bug — a circular oracle).
        if(m_CrystalStructures[phase1] < ebsdlib::CrystalStructure::LaueGroupEnd)
        {
          dEuler[0] = m_Eulers[3 * feature2 + 0];
          dEuler[1] = m_Eulers[3 * feature2 + 1];
          dEuler[2] = m_Eulers[3 * feature2 + 2];
          refDir[0] = -m_Normals[3 * i + 0];
          refDir[1] = -m_Normals[3 * i + 1];
          refDir[2] = -m_Normals[3 * i + 2];

          argb = ops[m_CrystalStructures[phase1]]->generateIPFColor(dEuler, refDir, false, m_ColorKey);
          m_SecondColors[3 * i + 0] = RgbColor::dRed(argb);
          m_SecondColors[3 * i + 1] = RgbColor::dGreen(argb);
          m_SecondColors[3 * i + 2] = RgbColor::dBlue(argb);
        }
      }
      else
      {
        m_SecondColors[3 * i + 0] = 0;
        m_SecondColors[3 * i + 1] = 0;
        m_SecondColors[3 * i + 2] = 0;
      }
    }
  }

  /**
   * @brief operator () This is called from the TBB stye of code
   * @param r The range to compute the values
   */
  void operator()(const Range& r) const
  {
    generate(r.min(), r.max());
  }
};

// -----------------------------------------------------------------------------
ComputeFaceIPFColoring::ComputeFaceIPFColoring(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeFaceIPFColoringInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFaceIPFColoring::~ComputeFaceIPFColoring() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFaceIPFColoring::operator()()
{
  auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& faceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceNormalsArrayPath);
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  DataPath firstIpfColorsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath.replaceName(m_InputValues->FirstFaceIPFColorsArrayName);
  auto& firstIpfColors = m_DataStructure.getDataRefAs<UInt8Array>(firstIpfColorsArrayPath);
  DataPath secondIpfColorsArrayPath = m_InputValues->SurfaceMeshFaceLabelsArrayPath.replaceName(m_InputValues->SecondFaceIPFColorsArrayName);
  auto& secondIpfColors = m_DataStructure.getDataRefAs<UInt8Array>(secondIpfColorsArrayPath);
  int64 numTriangles = faceLabels.getNumberOfTuples();

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&faceLabels);
  algArrays.push_back(&faceNormals);
  algArrays.push_back(&eulerAngles);
  algArrays.push_back(&phases);
  algArrays.push_back(&crystalStructures);
  algArrays.push_back(&firstIpfColors);
  algArrays.push_back(&secondIpfColors);

  ParallelDataAlgorithm parallelTask;
  parallelTask.setRange(0, numTriangles);
  parallelTask.requireArraysInMemory(algArrays);
  parallelTask.execute(CalculateFaceIPFColorsImpl(faceLabels, phases, faceNormals, eulerAngles, crystalStructures, firstIpfColors, secondIpfColors, m_InputValues->ColorKey));

  return {};
}
