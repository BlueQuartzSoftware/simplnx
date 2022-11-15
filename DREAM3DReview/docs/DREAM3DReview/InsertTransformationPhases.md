# InsertTransformationPhases

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | ParentPhase | Parent Phase | int32 | Int32Parameter |
| YES | TransCrystalStruct | Transformation Phase Crystal Structure | ChoicesParameter::ValueType | ChoicesParameter |
| YES | TransformationPhaseMisorientation | Transformation Phase Misorientation | float32 | Float32Parameter |
| YES | DefineHabitPlane | Define Habit Plane | bool | BoolParameter |
| YES | TransformationPhaseHabitPlane | Transformation Phase Habit Plane | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | UseAllVariants | Use All Variants | bool | BoolParameter |
| YES | CoherentFrac | Coherent Fraction | float32 | Float32Parameter |
| YES | TransformationPhaseThickness | Transformation Phase Thickness | float32 | Float32Parameter |
| YES | NumTransformationPhasesPerFeature | Average Number Of Transformation Phases Per Feature | int32 | Int32Parameter |
| YES | PeninsulaFrac | Peninsula Transformation Phase Fraction | float32 | Float32Parameter |
| YES | FeatureIdsArrayPath | Feature Ids | DataPath | ArraySelectionParameter |
| YES | CellEulerAnglesArrayPath | Euler Angles | DataPath | ArraySelectionParameter |
| YES | CellPhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | CellFeatureAttributeMatrixName | Cell Feature Attribute Matrix | DataPath | DataGroupSelectionParameter |
| YES | FeatureEulerAnglesArrayPath | Average Euler Angles | DataPath | ArraySelectionParameter |
| YES | AvgQuatsArrayPath | Average Quaternions | DataPath | ArraySelectionParameter |
| YES | CentroidsArrayPath | Centroids | DataPath | ArraySelectionParameter |
| YES | EquivalentDiametersArrayPath | Equivalent Diameters | DataPath | ArraySelectionParameter |
| YES | FeaturePhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | StatsGenCellEnsembleAttributeMatrixPath | Cell Ensemble Attribute Matrix | DataPath | DataGroupSelectionParameter |
| YES | CrystalStructuresArrayPath | Crystal Structures | DataPath | ArraySelectionParameter |
| YES | PhaseTypesArrayPath | Phase Types | DataPath | ArraySelectionParameter |
| YES | ShapeTypesArrayPath | Shape Types | DataPath | ArraySelectionParameter |
| YES | NumFeaturesArrayPath | Number of Features | DataPath | ArraySelectionParameter |
| YES | FeatureParentIdsArrayName | Parent Ids | DataPath | ArrayCreationParameter |
| YES | NumFeaturesPerParentArrayPath | Number of Features Per Parent | DataPath | ArrayCreationParameter |
