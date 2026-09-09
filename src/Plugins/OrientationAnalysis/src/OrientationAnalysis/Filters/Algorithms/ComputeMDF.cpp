#include "ComputeMDF.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/LaueOps.h>
#include <EbsdLib/Orientation/Euler.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>
#include <EbsdLib/Texture/MisorientationKDE.h>

#include <memory>
#include <vector>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeMDF::ComputeMDF(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMDFInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeMDF::~ComputeMDF() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeMDF::operator()()
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();

  const auto& eulerAnglesRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath).getDataStoreRef();
  const auto& cellPhasesRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& crystalStructuresRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  const usize numEnsembles = crystalStructuresRef.getNumberOfTuples();
  const double halfwidth = static_cast<double>(m_InputValues->HalfwidthDegrees) * Constants::k_PiOver180D;

  // One misorientation KDE per valid ensemble/phase (ensemble 0 is the unused "invalid" phase).
  std::vector<std::unique_ptr<ebsdlib::MisorientationKDE>> kdes(numEnsembles);
  for(usize ensembleIndex = 1; ensembleIndex < numEnsembles; ensembleIndex++)
  {
    uint32 structure = crystalStructuresRef[ensembleIndex];
    if(structure > ebsdlib::CrystalStructure::Trigonal_High)
    {
      return MakeErrorResult(-96510, fmt::format("Ensemble {} has unsupported crystal structure {}", ensembleIndex, structure));
    }
    kdes[ensembleIndex] = std::make_unique<ebsdlib::MisorientationKDE>(orientationOps[structure], structure, halfwidth);
  }

  // Boundary sweep: only the forward (+x, +y, +z) neighbor of each cell is examined so that each
  // interior face is visited exactly once.
  const double faceAreas[3] = {static_cast<double>(spacing[1]) * spacing[2], static_cast<double>(spacing[0]) * spacing[2], static_cast<double>(spacing[0]) * spacing[1]};
  for(usize z = 0; z < dims[2]; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(usize y = 0; y < dims[1]; y++)
    {
      for(usize x = 0; x < dims[0]; x++)
      {
        const usize cellIndex = (z * dims[1] + y) * dims[0] + x;
        const usize neighborOffsets[3] = {1, dims[0], dims[0] * dims[1]};
        const bool hasNeighbor[3] = {x + 1 < dims[0], y + 1 < dims[1], z + 1 < dims[2]};
        for(usize axis = 0; axis < 3; axis++)
        {
          if(!hasNeighbor[axis])
          {
            continue;
          }
          const usize neighborIndex = cellIndex + neighborOffsets[axis];
          int32 feature1 = featureIdsRef[cellIndex];
          int32 feature2 = featureIdsRef[neighborIndex];
          if(feature1 == feature2 || feature1 <= 0 || feature2 <= 0)
          {
            continue;
          }
          int32 phase = cellPhasesRef[cellIndex];
          if(phase != cellPhasesRef[neighborIndex] || phase < 1 || static_cast<usize>(phase) >= numEnsembles)
          {
            continue;
          }
          ebsdlib::QuatD quat1 = ebsdlib::EulerDType(eulerAnglesRef[cellIndex * 3], eulerAnglesRef[cellIndex * 3 + 1], eulerAnglesRef[cellIndex * 3 + 2]).toQuaternion();
          ebsdlib::QuatD quat2 = ebsdlib::EulerDType(eulerAnglesRef[neighborIndex * 3], eulerAnglesRef[neighborIndex * 3 + 1], eulerAnglesRef[neighborIndex * 3 + 2]).toQuaternion();
          // Crystal-to-crystal misorientation in EbsdLib's passive (epsijk=+1) convention: q1 * conj(q2),
          // matching LaueOps::calculateMisorientation. This is the passive-quaternion equivalent of MTEX's
          // active-orientation inv(o1).*o2 (MTEX orientations are the conjugate of EbsdLib's passive quats).
          // Using conj(q1)*q2 here computes the misorientation in the crystal frame instead, which the MDF
          // fundamental-zone folding (getMDFFZRod/getMisoBin) mis-reduces, smearing the true correlation.
          ebsdlib::QuatD misoQuat = quat1 * quat2.conjugate();
          double weight = m_InputValues->UseAreaWeights ? faceAreas[axis] : 1.0;
          kdes[phase]->addMisorientation(misoQuat, weight);
        }
      }
    }
  }

  // Per-phase evaluation and output.
  for(usize ensembleIndex = 1; ensembleIndex < numEnsembles; ensembleIndex++)
  {
    DataPath phaseGroupPath = m_InputValues->OutputGroupPath.createChildPath(PhaseGroupName(static_cast<int32>(ensembleIndex)));
    auto& mdfArrayRef = m_DataStructure.getDataRefAs<Float64Array>(phaseGroupPath.createChildPath(k_MDFArrayName));
    ebsdlib::MisorientationKDE& kde = *kdes[ensembleIndex];
    kde.finalize();

    const usize mdfSize = orientationOps[crystalStructuresRef[ensembleIndex]]->getMDFSize();
    mdfArrayRef.getIDataStoreRef().resizeTuples({mdfSize});

    DataPath angleAMPath = phaseGroupPath.createChildPath(k_AngleDistributionAMName);
    auto& anglesRef = m_DataStructure.getDataRefAs<Float64Array>(angleAMPath.createChildPath(k_AnglesArrayName)).getDataStoreRef();
    auto& mdfDensityRef = m_DataStructure.getDataRefAs<Float64Array>(angleAMPath.createChildPath(k_MDFDensityArrayName)).getDataStoreRef();
    auto& randomDensityRef = m_DataStructure.getDataRefAs<Float64Array>(angleAMPath.createChildPath(k_RandomDensityArrayName)).getDataStoreRef();

    if(kde.totalWeight() <= 0.0)
    {
      m_MessageHandler(IFilter::Message::Type::Warning, fmt::format("Phase {}: no same-phase feature boundaries found; MDF left as zeros", ensembleIndex));
      mdfArrayRef.fill(0.0);
      // Still emit the angle axis and the Mackenzie (random) reference, with a zero measured density.
      auto curve = kde.computeAngleCurve(static_cast<usize>(m_InputValues->NumCurvePoints));
      for(usize i = 0; i < curve.Angles.size(); i++)
      {
        anglesRef[i] = curve.Angles[i] * Constants::k_180OverPiD;
        randomDensityRef[i] = curve.RandomDensity[i];
        mdfDensityRef[i] = 0.0;
      }
      continue;
    }

    m_MessageHandler(fmt::format("Phase {}: evaluating MDF on {} bins", ensembleIndex, mdfSize));
    // Evaluate the KDE at every bin center in parallel into a plain std::vector. KDE::evaluate() and
    // binCenter() are const and safe to call concurrently; the DataStore is written serially below
    // (DataArray/DataStore are NOT thread-safe).
    std::vector<double> mdfValues(mdfSize, 0.0);
    ParallelDataAlgorithm parallelAlgorithm;
    parallelAlgorithm.setRange(0, mdfSize);
    parallelAlgorithm.execute([&kde, &mdfValues](const Range& range) {
      for(usize binIndex = range.min(); binIndex < range.max(); binIndex++)
      {
        mdfValues[binIndex] = kde.evaluate(kde.binCenter(static_cast<int>(binIndex)));
      }
    });
    for(usize binIndex = 0; binIndex < mdfSize; binIndex++)
    {
      mdfArrayRef[binIndex] = mdfValues[binIndex];
    }

    m_MessageHandler(fmt::format("Phase {}: computing angle distribution curve", ensembleIndex));
    auto curve = kde.computeAngleCurve(static_cast<usize>(m_InputValues->NumCurvePoints));
    for(usize i = 0; i < curve.Angles.size(); i++)
    {
      anglesRef[i] = curve.Angles[i] * Constants::k_180OverPiD;
      mdfDensityRef[i] = curve.Density[i];
      randomDensityRef[i] = curve.RandomDensity[i];
    }
  }

  return {};
}
