# stream-dataflow
Public Release of Stream-Dataflow Infrastructure

This is the location of the public release of the infrastructure for the stream dataflow architecture.  
Please keep in mind this is an early stage release, and more advanced features will follow.

Description of Folders
* softbrain-config: Library for Defining Accelerator Substrate Topology, Features, and Instructions
* softbrain-emu: Library for software emulation of softbrain
* sb-scheduler: Library for parsing and scheduling dataflow graphs to a particular topology.  (right now only includes emulator code)
* gem5: (To be included)
* workloads: Example workloads including kernels based on diannao parallelization strategy. 


# Try it:
```bash
source setup.sh
make -j8
cd workloads/diannao
make -j8
bash run-tests.sh
```
