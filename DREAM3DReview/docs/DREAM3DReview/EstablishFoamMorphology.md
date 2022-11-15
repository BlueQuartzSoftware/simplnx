# EstablishFoamMorphology

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | PeriodicBoundaries | Periodic Boundaries | bool | BoolParameter |
| YES | DataContainerName | Data Container | DataPath | DataGroupSelectionParameter |
| YES | InputStatsArrayPath | Statistics | DataPath | ArraySelectionParameter |
| YES | InputPhaseTypesArrayPath | Phase Types | DataPath | ArraySelectionParameter |
| YES | InputPhaseNamesArrayPath | Phase Names | DataPath | ArraySelectionParameter |
| YES | InputShapeTypesArrayPath | Shape Types | DataPath | ArraySelectionParameter |
| YES | InputCellFeatureIdsArrayPath | Feature Ids | DataPath | ArraySelectionParameter |
| YES | OutputCellEnsembleAttributeMatrixName | Cell Ensemble Attribute Matrix | StringParameter::ValueType | StringParameter |
| YES | NumFeaturesArrayName | Number of Features | StringParameter::ValueType | StringParameter |
| YES | OutputCellFeatureAttributeMatrixName | Cell Feature Attribute Matrix | StringParameter::ValueType | StringParameter |
| YES | FeatureIdsArrayName | Feature Ids | StringParameter::ValueType | StringParameter |
| YES | MaskArrayName | Mask | StringParameter::ValueType | StringParameter |
| YES | CellPhasesArrayName | Phases | StringParameter::ValueType | StringParameter |
| YES | QPEuclideanDistancesArrayName | Quadruple Point Euclidean Distances | StringParameter::ValueType | StringParameter |
| YES | TJEuclideanDistancesArrayName | Triple Junction Euclidean Distances | StringParameter::ValueType | StringParameter |
| YES | GBEuclideanDistancesArrayName | Grain Boundary Euclidean Distances | StringParameter::ValueType | StringParameter |
| YES | WriteGoalAttributes | Write Goal Attributes | bool | BoolParameter |
| YES | CsvOutputFile | Goal Attribute CSV File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | HaveFeatures | Already Have Features | ChoicesParameter::ValueType | ChoicesParameter |
| YES | MinStrutThickness | Minimum Strut Thickness | float64 | Float64Parameter |
| YES | StrutThicknessVariability | Strut Thickness Variability Factor | float32 | Float32Parameter |
| YES | StrutShapeVariability | Strut Cross Section Shape Factor | float32 | Float32Parameter |
