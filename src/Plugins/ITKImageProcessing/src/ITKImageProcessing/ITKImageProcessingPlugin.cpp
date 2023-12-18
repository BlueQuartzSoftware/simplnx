#include "ITKImageProcessingPlugin.hpp"

#include "ITKImageProcessing/ITKImageProcessing_filter_registration.hpp"
#include "ITKImageProcessingLegacyUUIDMapping.hpp"

#include <itkBMPImageIOFactory.h>
#include <itkBioRadImageIOFactory.h>
#include <itkGE4ImageIOFactory.h>
#include <itkGE5ImageIOFactory.h>
#include <itkGiplImageIOFactory.h>
#include <itkJPEGImageIOFactory.h>
#include <itkMRCImageIOFactory.h>
#include <itkMetaImageIOFactory.h>
#include <itkNiftiImageIOFactory.h>
#include <itkNrrdImageIOFactory.h>
#include <itkPNGImageIOFactory.h>
#include <itkStimulateImageIOFactory.h>
#include <itkTIFFImageIOFactory.h>
#include <itkVTKImageIOFactory.h>

using namespace complex;

namespace
{
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
} // namespace

ITKImageProcessingPlugin::ITKImageProcessingPlugin()
: AbstractPlugin(k_ID, "ITKImageProcessing", "Filters that wrap the ITK Software library. ITK is located at https://github.com/InsightSoftwareConsortium/ITK", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }
  RegisterITKImageIO();
}

ITKImageProcessingPlugin::~ITKImageProcessingPlugin() noexcept = default;

void ITKImageProcessingPlugin::RegisterITKImageIO()
{
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::NrrdImageIOFactory::RegisterOneFactory();
  itk::PNGImageIOFactory::RegisterOneFactory();
  itk::TIFFImageIOFactory::RegisterOneFactory();
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::BMPImageIOFactory::RegisterOneFactory();
  itk::MetaImageIOFactory::RegisterOneFactory();
  itk::NiftiImageIOFactory::RegisterOneFactory();
  itk::GiplImageIOFactory::RegisterOneFactory();
  itk::VTKImageIOFactory::RegisterOneFactory();
  itk::StimulateImageIOFactory::RegisterOneFactory();
  itk::BioRadImageIOFactory::RegisterOneFactory();
  itk::GE4ImageIOFactory::RegisterOneFactory();
  itk::GE5ImageIOFactory::RegisterOneFactory();
  itk::MRCImageIOFactory::RegisterOneFactory();
}

AbstractPlugin::SIMPLMapType ITKImageProcessingPlugin::getSimplToComplexMap() const
{
  return complex::k_SIMPL_to_ITKImageProcessing;
}

std::vector<std::string> ITKImageProcessingPlugin::GetList2DSupportedFileExtensions()
{
  return {".png", ".tif", ".jpg", ".jpeg", ".bmp", ".mha"};
}

COMPLEX_DEF_PLUGIN(ITKImageProcessingPlugin)
