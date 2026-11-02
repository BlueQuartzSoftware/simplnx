# Unreferenced images

This directory holds the illustrations that came from the removed ITKImageProcessing plugin.
The move stripped the `ITK` prefix from each file name. 31 of the 70 images appear in a filter
documentation page. The other 39 do not. This file records why they stay here.

Keep these images. They cost little, and a person or a tool that implements one of these
filters natively can use the illustration immediately. To use one, add a Markdown reference
to the filter page, directly above the parameter-table marker:

```markdown
![Short caption.](Images/<file name>.png)
```

## Images for a filter that no plugin implements

No DREAM3D-NX plugin contains these filters. Neither the ImageProcessing plugin nor the
removed ITKImageProcessing plugin implemented them. Each image describes the ITK original.

| Image | ITK filter |
|---|---|
| `BilateralFiltering.png` | Bilateral |
| `BinomialBlurImageFilter.png` | BinomialBlur |
| `CastImageFilter.png` | Cast |
| `FrangisVesselness.png`, `FrangisVesselness_2.png`, `FrangisVesselness_3.png` | FrangiVesselness |
| `GrayscaleClosingByReconstruction.png` | GrayscaleClosingByReconstruction |
| `GrayscaleOpeningByReconstruction.png` | GrayscaleOpeningByReconstruction |
| `HessianBasedObjectness.png`, `HessianBasedObjectness_2.png`, `HessianBasedObjectness_3.png` | HessianBasedObjectness |
| `HistogramMatchingImageFilter_2.png` | HistogramMatching |
| `MDimensionalObjectnessInNDimensions.png`, `MDimensionsObjectnessInNDimensions_2.png` | HessianBasedObjectness, supporting figure |
| `NeighborhoodMean.png` | NeighborhoodMean |
| `NonLocalMeans.png` | NonLocalMeans |
| `RGBToLuminanceImageFilter_2.png` | RGBToLuminance |

## Images for a filter that this plugin does implement

Each of these filters has a page in this plugin. The page does not show the image yet. Add the
reference when you next edit the page.

| Image | Page |
|---|---|
| `CurvatureAnisotropicDiffusion.png` | `CurvatureAnisotropicDiffusionImageFilter.md` |
| `CurvatureFlow.png` | `CurvatureFlowImageFilter.md` |
| `DoubleThresholdImageFilter.png` | `DoubleThresholdImageFilter.md` |
| `GradientAnisotropicDiffusion.png` | `GradientAnisotropicDiffusionImageFilter.md` |
| `ImportFijiConfigFile.png` | `ImportFijiMontageFilter.md` |
| `MaximumConnectedComponents.png` | `ThresholdMaximumConnectedComponentsImageFilter.md` |
| `MedianFiltering.png` | `MedianImageFilter.md` |
| `MinMaxCurvatureFlow.png` | `MinMaxCurvatureFlowImageFilter.md` |
| `SigmoidImage_Equation.png` | `SigmoidImageFilter.md` |
| `SmoothingRecursiveGaussianImageFilter.png` | `SmoothingRecursiveGaussianImageFilter.md` |

## Images that explain a concept

These images describe an idea, not one filter. A morphology page can use
`StructuringElement.png`. A watershed page can use the watershed figures.

| Image | Subject |
|---|---|
| `StructuringElement.png` | The structuring element that each morphology filter takes |
| `Parameters.png` | Generic parameter figure |
| `HessianMatrix.png` | The Hessian matrix |
| `Watersheds.png`, `MorphologicalWatersheds.png` | Watershed segmentation |
| `WatershedDistanceMapPipeline.png` | The distance-map watershed pipeline |
| `LevelMinimumBasinHeightForSegmentation.png` | The watershed Level parameter |
| `MaskWatershedWithIndicator.png` | A masked watershed |
| `Small_IN100_1-0.png` … `Small_IN100_1-3.png` | The Small IN100 example data set |
