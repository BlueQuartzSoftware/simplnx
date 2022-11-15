# ComputeFeatureEigenstrains

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | PoissonRatio | Poisson's Ratio | float32 | Float32Parameter |
| YES | UseEllipsoidalGrains | Use Ellipsoidal Grains (versus spherical assumption) | bool | BoolParameter |
| YES | UseCorrectionalMatrix | Use Correctional Matrix | bool | BoolParameter |
| YES | Beta11 | Beta11 | float32 | Float32Parameter |
| YES | Beta22 | Beta22 | float32 | Float32Parameter |
| YES | Beta33 | Beta33 | float32 | Float32Parameter |
| YES | Beta23 | Beta23 | float32 | Float32Parameter |
| YES | Beta13 | Beta13 | float32 | Float32Parameter |
| YES | Beta12 | Beta12 | float32 | Float32Parameter |
| YES | AxisLengthsArrayPath | Axis Lengths | DataPath | ArraySelectionParameter |
| YES | AxisEulerAnglesArrayPath | Axis Euler Angles | DataPath | ArraySelectionParameter |
| YES | ElasticStrainsArrayPath | Elastic Strains (Voigt Notation) | DataPath | ArraySelectionParameter |
| YES | EigenstrainsArrayName | Eigenstrains | DataPath | ArrayCreationParameter |
