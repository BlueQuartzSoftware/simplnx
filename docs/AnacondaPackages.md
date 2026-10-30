# Building Anaconda Packages

## Python Variations

- 3.11
- 3.12

## Operating System Variations

- MacOS Arm64
- Windows x64
- Linux x64

## Python Environment Setup

Use a release of Anaconda after NOV 2023. This is because the `conda` program now uses the `libMamba` solver which makes the packaging go faster.

```shell
# Create the Python Virtual Environment
(base) [user@host] $ conda create --name nxbuild python=3.10 conda-build
(base) [user@host] $ conda activate nxbuild
(nxbuild) [user@host] $ cd DREAM3DNX/conda
```

if you do a `conda list` from this new environment you should get the following versions:

```text
conda                     23.11.0         py310hbe9552e_1    conda-forge
conda-build               3.28.2          py310hbe9552e_0    conda-forge
```
