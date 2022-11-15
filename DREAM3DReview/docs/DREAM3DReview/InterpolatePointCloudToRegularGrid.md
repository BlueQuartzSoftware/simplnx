# InterpolatePointCloudToRegularGrid

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | UseMask | Use Mask | bool | BoolParameter |
| YES | StoreKernelDistances | Store Kernel Distances | bool | BoolParameter |
| YES | InterpolationTechnique | Interpolation Technique | ChoicesParameter::ValueType | ChoicesParameter |
| YES | KernelSize | Kernel Size | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | Sigmas | Gaussian Sigmas | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | DataContainerName | Data Container to Interpolate | DataPath | DataGroupSelectionParameter |
| YES | InterpolatedDataContainerName | Interpolated Data Container | DataPath | DataGroupSelectionParameter |
| YES | VoxelIndicesArrayPath | Voxel Indices | DataPath | ArraySelectionParameter |
| YES | MaskArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | ArraysToInterpolate | Attribute Arrays to Interpolate | MultiArraySelectionParameter::ValueType | MultiArraySelectionParameter |
| YES | ArraysToCopy | Attribute Arrays to Copy | MultiArraySelectionParameter::ValueType | MultiArraySelectionParameter |
| YES | InterpolatedAttributeMatrixName | Interpolated Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | KernelDistancesArrayName | Kernel Distances | DataPath | ArrayCreationParameter |
| YES | InterpolatedSuffix | Interpolated Array Suffix | StringParameter::ValueType | StringParameter |
| YES | CopySuffix | Copied Array Suffix | StringParameter::ValueType | StringParameter |
