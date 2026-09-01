#include "ComputeKernelAvgMisorientations.hpp"

#include "ComputeKernelAvgMisorientationsDirect.hpp"
#include "ComputeKernelAvgMisorientationsScanline.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeKernelAvgMisorientations::ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeKernelAvgMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeKernelAvgMisorientations::~ComputeKernelAvgMisorientations() noexcept = default;

Result<> ComputeKernelAvgMisorientations::operator()()
{
  const auto* featureIds = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto* phases = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CellPhasesArrayPath);
  const auto* quats = m_DataStructure.getDataAs<IDataArray>(m_InputValues->QuatsArrayPath);
  const auto* crystalStructures = m_DataStructure.getDataAs<IDataArray>(m_InputValues->CrystalStructuresArrayPath);
  const auto* output = m_DataStructure.getDataAs<IDataArray>(m_InputValues->KernelAverageMisorientationsArrayName);

  return DispatchAlgorithm<ComputeKernelAvgMisorientationsDirect, ComputeKernelAvgMisorientationsScanline>({featureIds, phases, quats, crystalStructures, output}, m_DataStructure, m_MessageHandler,
                                                                                                           m_ShouldCancel, m_InputValues);
}
