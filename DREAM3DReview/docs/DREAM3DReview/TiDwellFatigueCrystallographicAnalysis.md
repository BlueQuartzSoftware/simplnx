# TiDwellFatigueCrystallographicAnalysis

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | AlphaGlobPhasePresent | Alpha Glob Phase Present | bool | BoolParameter |
| YES | AlphaGlobPhase | Alpha Glob Phase Number | int32 | Int32Parameter |
| YES | MTRPhase | Microtextured Region Phase Number | int32 | Int32Parameter |
| YES | LatticeParameterA | Lattice Parameter A | float32 | Float32Parameter |
| YES | LatticeParameterC | Lattice Parameter C | float32 | Float32Parameter |
| YES | StressAxis | Stress Axis | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | SubsurfaceDistance | Subsurface Feature Distance to Consider (Microns) | int32 | Int32Parameter |
| YES | ConsiderationFraction | Fraction of Features to Consider | float32 | Float32Parameter |
| YES | DoNotAssumeInitiatorPresence | Do Not Assume Initiator Presence | bool | BoolParameter |
| YES | InitiatorLowerThreshold | Initiator Lower Threshold (Degrees) | float32 | Float32Parameter |
| YES | InitiatorUpperThreshold | Initiator Upper Threshold (Degrees) | float32 | Float32Parameter |
| YES | HardFeatureLowerThreshold | Hard Feature Lower Threshold (Degrees) | float32 | Float32Parameter |
| YES | HardFeatureUpperThreshold | Hard Feature Upper Threshold (Degrees) | float32 | Float32Parameter |
| YES | SoftFeatureLowerThreshold | Soft Feature Lower Threshold (Degrees) | float32 | Float32Parameter |
| YES | SoftFeatureUpperThreshold | Soft Feature Upper Threshold (Degrees) | float32 | Float32Parameter |
| YES | DataContainerName | Data Container | DataPath | DataGroupSelectionParameter |
| YES | FeatureIdsArrayPath | FeatureIds | DataPath | ArraySelectionParameter |
| YES | CellFeatureAttributeMatrixPath | Cell Feature Attribute Matrix | DataPath | DataGroupSelectionParameter |
| YES | FeatureEulerAnglesArrayPath | Average Euler Angles | DataPath | ArraySelectionParameter |
| YES | FeaturePhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | NeighborListArrayPath | Neighbor List | DataPath | ArraySelectionParameter |
| YES | CentroidsArrayPath | Centroids | DataPath | ArraySelectionParameter |
| YES | CrystalStructuresArrayPath | Crystal Structures | DataPath | ArraySelectionParameter |
| YES | CellParentIdsArrayName | Parent Ids | DataPath | ArrayCreationParameter |
| YES | NewCellFeatureAttributeMatrixName | Cell Feature Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | SelectedFeaturesArrayName | Selected Features | DataPath | ArrayCreationParameter |
| YES | InitiatorsArrayName | Initiators | DataPath | ArrayCreationParameter |
| YES | HardFeaturesArrayName | Hard Features | DataPath | ArrayCreationParameter |
| YES | SoftFeaturesArrayName | Soft Features | DataPath | ArrayCreationParameter |
| YES | HardSoftGroupsArrayName | Hard-Soft Groups | DataPath | ArrayCreationParameter |
| YES | FeatureParentIdsArrayName | Parent Ids | DataPath | ArrayCreationParameter |
