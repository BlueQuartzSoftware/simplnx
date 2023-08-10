#include <CxPybind/CxPybind.hpp>

#include <OrientationAnalysis/OrientationAnalysisPlugin.hpp>

#include <OrientationAnalysis/Parameters/H5EbsdReaderParameter.h>
#include <OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h>

#include <OrientationAnalysis/Filters/AlignSectionsMisorientationFilter.hpp>
#include <OrientationAnalysis/Filters/AlignSectionsMutualInformationFilter.hpp>
#include <OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.hpp>
#include <OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.hpp>
#include <OrientationAnalysis/Filters/ConvertOrientations.hpp>
#include <OrientationAnalysis/Filters/ConvertQuaternionFilter.hpp>
#include <OrientationAnalysis/Filters/CreateEnsembleInfoFilter.hpp>
#include <OrientationAnalysis/Filters/EBSDSegmentFeaturesFilter.hpp>
#include <OrientationAnalysis/Filters/EbsdToH5EbsdFilter.hpp>
#include <OrientationAnalysis/Filters/EnsembleInfoReaderFilter.hpp>
#include <OrientationAnalysis/Filters/ExportGBCDGMTFileFilter.hpp>
#include <OrientationAnalysis/Filters/ExportGBCDTriangleDataFilter.hpp>
#include <OrientationAnalysis/Filters/FindAvgCAxesFilter.hpp>
#include <OrientationAnalysis/Filters/FindAvgOrientationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindBoundaryStrengthsFilter.hpp>
#include <OrientationAnalysis/Filters/FindCAxisLocationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindFeatureNeighborCAxisMisalignmentsFilter.hpp>
#include <OrientationAnalysis/Filters/FindFeatureReferenceCAxisMisorientationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindFeatureReferenceMisorientationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindGBCDFilter.hpp>
#include <OrientationAnalysis/Filters/FindGBCDMetricBasedFilter.hpp>
#include <OrientationAnalysis/Filters/FindGBPDMetricBasedFilter.hpp>
#include <OrientationAnalysis/Filters/FindKernelAvgMisorientationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindMisorientationsFilter.hpp>
#include <OrientationAnalysis/Filters/FindSchmidsFilter.hpp>
#include <OrientationAnalysis/Filters/FindShapesFilter.hpp>
#include <OrientationAnalysis/Filters/FindSlipTransmissionMetricsFilter.hpp>
#include <OrientationAnalysis/Filters/FindTriangleGeomShapesFilter.hpp>
#include <OrientationAnalysis/Filters/GenerateFZQuaternions.hpp>
#include <OrientationAnalysis/Filters/GenerateFaceIPFColoringFilter.hpp>
#include <OrientationAnalysis/Filters/GenerateFaceMisorientationColoringFilter.hpp>
#include <OrientationAnalysis/Filters/GenerateGBCDPoleFigureFilter.hpp>
#include <OrientationAnalysis/Filters/GenerateIPFColorsFilter.hpp>
#include <OrientationAnalysis/Filters/GenerateQuaternionConjugateFilter.hpp>
#include <OrientationAnalysis/Filters/ImportH5EspritDataFilter.hpp>
#include <OrientationAnalysis/Filters/ImportH5OimDataFilter.hpp>
#include <OrientationAnalysis/Filters/MergeTwinsFilter.hpp>
#include <OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.hpp>
#include <OrientationAnalysis/Filters/ReadAngDataFilter.hpp>
#include <OrientationAnalysis/Filters/ReadCtfDataFilter.hpp>
#include <OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp>
#include <OrientationAnalysis/Filters/RodriguesConvertorFilter.hpp>
#include <OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp>
#include <OrientationAnalysis/Filters/WritePoleFigureFilter.hpp>
#include <OrientationAnalysis/Filters/WriteStatsGenOdfAngleFileFilter.hpp>

using namespace complex;
using namespace complex::CxPybind11;
namespace py = pybind11;

using namespace pybind11::literals;

PYBIND11_MODULE(orientationanalysis, mod)
{
  py::module_::import("complex");

  auto& internals = Internals::Instance();

  auto* plugin = internals.addPlugin<OrientationAnalysisPlugin>();

  auto h5EbsdReaderParameter = COMPLEX_PY_BIND_PARAMETER(mod, H5EbsdReaderParameter);
  auto oemEbsdScanSelectionParameter = COMPLEX_PY_BIND_PARAMETER(mod, OEMEbsdScanSelectionParameter);

  py::class_<H5EbsdReaderParameter::ValueType> h5EbsdReaderValueType(h5EbsdReaderParameter, "ValueType");
  h5EbsdReaderValueType.def(py::init<>());
  h5EbsdReaderValueType.def_readwrite("input_file_path", &H5EbsdReaderParameter::ValueType::inputFilePath);
  h5EbsdReaderValueType.def_readwrite("start_slice", &H5EbsdReaderParameter::ValueType::startSlice);
  h5EbsdReaderValueType.def_readwrite("end_slice", &H5EbsdReaderParameter::ValueType::startSlice);
  h5EbsdReaderValueType.def_readwrite("euler_representation", &H5EbsdReaderParameter::ValueType::eulerRepresentation);
  h5EbsdReaderValueType.def_readwrite("hdf5_data_paths", &H5EbsdReaderParameter::ValueType::hdf5DataPaths);
  h5EbsdReaderValueType.def_readwrite("use_recommended_transform", &H5EbsdReaderParameter::ValueType::useRecommendedTransform);

  py::class_<OEMEbsdScanSelectionParameter::ValueType> oemEbsdScanSelectionValueType(oemEbsdScanSelectionParameter, "ValueType");
  oemEbsdScanSelectionValueType.def(py::init<>());
  oemEbsdScanSelectionValueType.def_readwrite("input_file_path", &OEMEbsdScanSelectionParameter::ValueType::inputFilePath);
  oemEbsdScanSelectionValueType.def_readwrite("stacking_order", &OEMEbsdScanSelectionParameter::ValueType::stackingOrder);
  oemEbsdScanSelectionValueType.def_readwrite("scan_names", &OEMEbsdScanSelectionParameter::ValueType::scanNames);

  py::enum_<OEMEbsdScanSelectionParameter::EbsdReaderType> oemEbsdScanSelectionEbsdReaderType(oemEbsdScanSelectionParameter, "EbsdReaderType");
  oemEbsdScanSelectionEbsdReaderType.value("Oim", OEMEbsdScanSelectionParameter::EbsdReaderType::Oim);
  oemEbsdScanSelectionEbsdReaderType.value("Esprit", OEMEbsdScanSelectionParameter::EbsdReaderType::Esprit);

  py::enum_<OEMEbsdScanSelectionParameter::ManufacturerType> oemEbsdScanSelectionManufacturerType(oemEbsdScanSelectionParameter, "ManufacturerType");
  oemEbsdScanSelectionManufacturerType.value("EDAX", OEMEbsdScanSelectionParameter::ManufacturerType::EDAX);
  oemEbsdScanSelectionManufacturerType.value("Oxford", OEMEbsdScanSelectionParameter::ManufacturerType::Oxford);
  oemEbsdScanSelectionManufacturerType.value("Bruker", OEMEbsdScanSelectionParameter::ManufacturerType::Bruker);
  oemEbsdScanSelectionManufacturerType.value("HEDM", OEMEbsdScanSelectionParameter::ManufacturerType::HEDM);
  oemEbsdScanSelectionManufacturerType.value("Zeiss", OEMEbsdScanSelectionParameter::ManufacturerType::Zeiss);
  oemEbsdScanSelectionManufacturerType.value("Phillips", OEMEbsdScanSelectionParameter::ManufacturerType::Phillips);
  oemEbsdScanSelectionManufacturerType.value("ThermoFisher", OEMEbsdScanSelectionParameter::ManufacturerType::ThermoFisher);
  oemEbsdScanSelectionManufacturerType.value("DREAM3D", OEMEbsdScanSelectionParameter::ManufacturerType::DREAM3D);
  oemEbsdScanSelectionManufacturerType.value("Unknown", OEMEbsdScanSelectionParameter::ManufacturerType::Unknown);

  BindParameterConstructor(h5EbsdReaderParameter);

  oemEbsdScanSelectionParameter.def(
      py::init<const std::string&, const std::string&, const std::string&, const OEMEbsdScanSelectionParameter::ValueType&, const OEMEbsdScanSelectionParameter::AllowedManufacturers&,
               OEMEbsdScanSelectionParameter::EbsdReaderType, OEMEbsdScanSelectionParameter::ExtensionsType>(),
      "name"_a, "human_name"_a, "help_text"_a, "default_value"_a, "allowed_manufacturers"_a, "reader_type"_a, "extensions_type"_a);

  internals.addConversion<H5EbsdReaderParameter>();
  internals.addConversion<OEMEbsdScanSelectionParameter>();

  BindFilter<AlignSectionsMisorientationFilter>(mod, internals);
  BindFilter<AlignSectionsMutualInformationFilter>(mod, internals);
  BindFilter<BadDataNeighborOrientationCheckFilter>(mod, internals);
  BindFilter<CAxisSegmentFeaturesFilter>(mod, internals);
  BindFilter<ConvertOrientations>(mod, internals);
  BindFilter<ConvertQuaternionFilter>(mod, internals);
  BindFilter<CreateEnsembleInfoFilter>(mod, internals);
  BindFilter<EBSDSegmentFeaturesFilter>(mod, internals);
  BindFilter<EbsdToH5EbsdFilter>(mod, internals);
  BindFilter<EnsembleInfoReaderFilter>(mod, internals);
  BindFilter<ExportGBCDGMTFileFilter>(mod, internals);
  BindFilter<ExportGBCDTriangleDataFilter>(mod, internals);
  BindFilter<FindAvgCAxesFilter>(mod, internals);
  BindFilter<FindAvgOrientationsFilter>(mod, internals);
  BindFilter<FindBoundaryStrengthsFilter>(mod, internals);
  BindFilter<FindCAxisLocationsFilter>(mod, internals);
  BindFilter<FindFeatureNeighborCAxisMisalignmentsFilter>(mod, internals);
  BindFilter<FindFeatureReferenceCAxisMisorientationsFilter>(mod, internals);
  BindFilter<FindFeatureReferenceMisorientationsFilter>(mod, internals);
  BindFilter<FindGBCDFilter>(mod, internals);
  BindFilter<FindGBCDMetricBasedFilter>(mod, internals);
  BindFilter<FindGBPDMetricBasedFilter>(mod, internals);
  BindFilter<FindKernelAvgMisorientationsFilter>(mod, internals);
  BindFilter<FindMisorientationsFilter>(mod, internals);
  BindFilter<FindSchmidsFilter>(mod, internals);
  BindFilter<FindShapesFilter>(mod, internals);
  BindFilter<FindSlipTransmissionMetricsFilter>(mod, internals);
  BindFilter<FindTriangleGeomShapesFilter>(mod, internals);
  BindFilter<GenerateFaceIPFColoringFilter>(mod, internals);
  BindFilter<GenerateFaceMisorientationColoringFilter>(mod, internals);
  BindFilter<GenerateFZQuaternions>(mod, internals);
  BindFilter<GenerateGBCDPoleFigureFilter>(mod, internals);
  BindFilter<GenerateIPFColorsFilter>(mod, internals);
  BindFilter<GenerateQuaternionConjugateFilter>(mod, internals);
  BindFilter<ImportH5EspritDataFilter>(mod, internals);
  BindFilter<ImportH5OimDataFilter>(mod, internals);
  BindFilter<MergeTwinsFilter>(mod, internals);
  BindFilter<NeighborOrientationCorrelationFilter>(mod, internals);
  BindFilter<ReadAngDataFilter>(mod, internals);
  BindFilter<ReadCtfDataFilter>(mod, internals);
  BindFilter<ReadH5EbsdFilter>(mod, internals);
  BindFilter<RodriguesConvertorFilter>(mod, internals);
  BindFilter<RotateEulerRefFrameFilter>(mod, internals);
  BindFilter<WritePoleFigureFilter>(mod, internals);
  BindFilter<WriteStatsGenOdfAngleFileFilter>(mod, internals);

  internals.registerPluginPyFilters(*plugin);
}