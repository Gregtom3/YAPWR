# YAPWR
Yet Another Partial Wave Repository: Code for extracting Dihadron Partial Waves at CLAS12


## Table of Contents

- [Description](#description)  
- [Usage](#usage)  
- [Contact](#contact)  

## Description

This repository manages a SLURM pipeline for studying dihadron partial waves at CLAS12. The goal of the pipeline goes beyond extracting the partial waves:

  - Produce kinematic plots of the datasets ($x$, $Q^2$, $p_T$, etc.)
  - Perform systematic errors analyses
  - Calculate bin centers
  - Generate LaTeX tables/figures for easy pasting into Overleaf


## Usage

The program is intended to be used on Jefferson Lab's `ifarm` , and run using their cluster manager `slurm`. 

```bash
./run_project.rb  PROJECT_NAME  RUNCARD  CONFIG1 [CONFIG2 ...]
```

`PROJECT_NAME` saves the results to `out/PROJECT_NAME`. Multiple configs can be called via wilcard. For instance,

```bash
./run_project.rb test runcards/runcard.yaml configs/*.yaml
```

Available options include

```
--append           do not overwrite existing out/<PROJECT>; skip filterTree
--maxEntries N     pass N to filterTree as entry limit
--maxFiles   M     only create M tree_info.yaml per config
--slurm            submit one Slurm job per CONFIG instead of running now
```

## Contact

Gregory Matousek (gamatousek@gmail.com)
   
