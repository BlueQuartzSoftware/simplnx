# Building Anaconda Packages

## IMPORTANT NOTE ABOUT PACKAGING TIMES

These builds can on the order of **multiple hours** to complete on x86 systems that predate Intel 13th Generation processors. Be patient. The build looks like it is stuck but python is just taking it's time to recursively figure out the needed DLLs because ITK and VTK create a ton of DLL libraries. This is especially slow on Windows systems.

## Python Varitions

- 3.9
- 3.10
- 3.11
- 3.12

## Operating System Variations

- MacOS x64
- MacOS Arm64
- Windows x64
- Linux x64

## Python Environment Setup

Use a release of Anaconda after NOV 2023. This is because the `conda` program now uses the `libMamba` solver which makes the packaging go faster.

```shell
# Create the Python Virtual Environment
(base) [user@host] $ conda create --name nx-build python=3.10 conda-build
(base) [user@host] $ conda activate nx-build
(nx-build) [user@host] $ cd DREAM3DNX/conda
```

if you do a `conda list` from this new environment you should get the following versions:

```text
conda                     23.11.0         py310hbe9552e_1    conda-forge
conda-build               3.28.2          py310hbe9552e_0    conda-forge
```

If you are on an operating system that requires `mambabuild` you will also need to run the following command inside the `nx-build` virtual environment.

```shell
(nx-build) [user@host] $ conda install mamba boa
```

| Operating System | Build Command |
|------------------|---------------|
| MacOS Arm64      | conda build .    |
| MacOS x64        | conda mambabuild |
| Windows x64 (10/11) | conda build . |
| Linux x64 (Ubuntu 22.04) | conda mambabuild . |

