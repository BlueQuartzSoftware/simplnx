| Plugin::ClassName | Human Label | Ready to Port |
|---|---|---|
| ComplexCore::ConditionalSetValue | "Replace Value in Array (Conditional)" | YES  |
| ComplexCore::CreateDataArray | "Create Data Array" | YES |
| ComplexCore::MultiThresholdObjects2 | "Threshold Objects" | NO |
| ComplexCore::ReadDream3DFile | "Read DREAM.3D Data File" | YES |
| ComplexCore::RemoveArrays | "Delete Data" | NO |
| ComplexCore::WriteDream3DFile | "Write DREAM.3D Data File" | YES |
| DREAM3DReview::ApproximatePointCloudHull | "Approximate Point Cloud Hull" | YES |
| DREAM3DReview::ExtractInternalSurfacesFromTriangleGeometry | "Extract Internal Surfaces from Triangle" | YES |
| DREAM3DReview::FindNeighborListStatistics | "Find NeighborList Statistics" | YES |
| DREAM3DReview::InterpolatePointCloudToRegularGrid | "Interpolate Point Cloud to Regular Grid" | YES |
| DREAM3DReview::IterativeClosestPoint | "Iterative Closest Point" | YES |
| DREAM3DReview::MapPointCloudToRegularGrid | "Map Point Cloud to Regular Grid" | YES |
| DREAM3DReview::PointSampleTriangleGeometry | "Point Sample Triangle Geometry" | YES |
| DREAM3DReview::RemoveFlaggedVertices | "Remove Flagged Vertices" | YES |
| DREAM3DReview::RobustAutomaticThreshold | "Robust Automatic Threshold" | YES |
| DREAM3DReview::ApplyTransformationToGeometry | "Apply Transformation to Geometry" | NO |
| ImportExport::ReadStlFile | "Import STL File" | YES  |
| ITKImageProcessing::ITKBinaryThresholdImage | "ITK::Binary Threshold Image Filter" | YES |
| ITKImageProcessing::ITKGradientMagnitudeImage | "ITK::Gradient Magnitude Image Filter" | YES |
| ITKImageProcessing::ITKGrayscaleFillholeImage | "ITK::Grayscale Fillhole Image Filter" | YES |
| ITKImageProcessing::ITKMaskImage | "ITK::Mask Image Filter" | YES |
| ITKImageProcessing::ITKNormalizeImage | "ITK::Normalize Image Filter" | YES |
| ITKImageProcessing::ITKImportImageStack | | YES |
| ITKImageProcessing::ITKImageReader | | YES |
| ITKImageProcessing::ITKImageWriter | | YES |
| Processing::MinSize | "Minimum Size" | YES |
| Reconstruction::ScalarSegmentFeatures | "Segment Features (Scalar)" | YES |
| StatsToolbox::FindDifferenceMap | "Find Difference Map" | YES |
| StatsToolbox::FindSizes | "Find Feature Sizes" |YES  |
| SurfaceMeshing::LaplacianSmoothing | "Laplacian Smoothing" | YES |
| SurfaceMeshing::QuickSurfaceMesh | "Quick Surface Mesh" | YES |
| SurfaceMeshing::TriangleAreaFilter | "Generate Triangle Areas" | YES  |


## Pipeline 1 ##

    "Filter_Name": "DataContainerReader",
    "Filter_Name": "ITKGradientMagnitudeImage",
    "Filter_Name": "RobustAutomaticThreshold",
    "Filter_Name": "CreateDataArray",
    "Filter_Name": "ConditionalSetValue",
    "Filter_Name": "RemoveArrays",
    "Filter_Name": "ITKMaskImage",
    "Filter_Name": "ITKGrayscaleFillholeImage",
    "Filter_Name": "FindDifferenceMap",
    "Filter_Name": "ITKNormalizeImage",
    "Filter_Name": "ITKBinaryThresholdImage",
    "Filter_Name": "MultiThresholdObjects",
    "Filter_Name": "ScalarSegmentFeatures",
    "Filter_Name": "RemoveArrays",
    "Filter_Name": "FindSizes",
    "Filter_Name": "MinSize",
    "Filter_Name": "DataContainerWriter",


## Pipeline 2 ##

    "Filter_Name": "DataContainerReader",
    "Filter_Name": "ITKGradientMagnitudeImage",
    "Filter_Name": "RobustAutomaticThreshold",
    "Filter_Name": "CreateDataArray",
    "Filter_Name": "ConditionalSetValue",
    "Filter_Name": "RemoveArrays",
    "Filter_Name": "QuickSurfaceMesh",
    "Filter_Name": "ExtractInternalSurfacesFromTriangleGeometry",
    "Filter_Name": "RemoveArrays",
    "Filter_Name": "LaplacianSmoothing",
    "Filter_Name": "TriangleAreaFilter",
    "Filter_Name": "PointSampleTriangleGeometry",
    "Filter_Name": "DataContainerWriter",

## Pipeline 3 ##

    "Filter_Name": "ReadStlFile",
    "Filter_Name": "TriangleAreaFilter",
    "Filter_Name": "PointSampleTriangleGeometry",
    "Filter_Name": "DataContainerWriter",

## Pipeline 4 ##

    "Filter_Name": "DataContainerReader",
    "Filter_Name": "MultiThresholdObjects",
    "Filter_Name": "RemoveFlaggedVertices",
    "Filter_Name": "RemoveArrays",
    "Filter_Name": "ApproximatePointCloudHull",
    "Filter_Name": "DataContainerWriter",

## Pipeline 5 ##

    "Filter_Name": "DataContainerReader",
    "Filter_Name": "DataContainerReader",
    "Filter_Name": "DataContainerReader",
    "Filter_Name": "IterativeClosestPoint",
    "Filter_Name": "IterativeClosestPoint",
    "Filter_Name": "ApplyTransformationToGeometry",
    "Filter_Name": "ApplyTransformationToGeometry",
    "Filter_Name": "MapPointCloudToRegularGrid",
    "Filter_Name": "InterpolatePointCloudToRegularGrid",
    "Filter_Name": "FindNeighborListStatistics",
    "Filter_Name": "FindNeighborListStatistics",
    "Filter_Name": "DataContainerWriter",