# ITKPatchBasedDenoisingImage #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | NoiseModel | Noise Model | ChoicesParameter::ValueType | ChoicesParameter |
| YES | KernelBandwidthSigma | KernelBandwidthSigma | float64 | Float64Parameter |
| YES | PatchRadius | PatchRadius | float64 | Float64Parameter |
| YES | NumberOfIterations | NumberOfIterations | float64 | Float64Parameter |
| YES | NumberOfSamplePatches | NumberOfSamplePatches | float64 | Float64Parameter |
| YES | SampleVariance | SampleVariance | float64 | Float64Parameter |
| YES | NoiseSigma | NoiseSigma | float64 | Float64Parameter |
| YES | NoiseModelFidelityWeight | NoiseModelFidelityWeight | float64 | Float64Parameter |
| YES | AlwaysTreatComponentsAsEuclidean | AlwaysTreatComponentsAsEuclidean | bool | BoolParameter |
| YES | KernelBandwidthEstimation | KernelBandwidthEstimation | bool | BoolParameter |
| YES | KernelBandwidthMultiplicationFactor | KernelBandwidthMultiplicationFactor | float64 | Float64Parameter |
| YES | KernelBandwidthUpdateFrequency | KernelBandwidthUpdateFrequency | float64 | Float64Parameter |
| YES | KernelBandwidthFractionPixelsForEstimation | KernelBandwidthFractionPixelsForEstimation | float64 | Float64Parameter |
| YES | SelectedCellArrayPath | Attribute Array to filter | DataPath | ArraySelectionParameter |
| YES | NewCellArrayName | Filtered Array | StringParameter::ValueType | StringParameter |
