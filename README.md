# Neuro-OS

A bare-metal, i686 operating system built from scratch featuring a kernel-level Liquid State Machine (LSM) that biologically drives OS process scheduling.

### Architecture & Features
- **Neuromorphic Lottery Scheduler**: The traditional Round-Robin scheduler is replaced. OS tasks are mapped to the Output Neurons of the LSM. CPU time is probabilistically allocated based on real-time membrane potentials, meaning the OS dynamically thinks about which task to schedule.
- **Kernel-Level Liquid State Machine**: A 64-neuron, 1024-synapse spiking neural network partitioned into Input, Liquid/Reservoir, and Output layers. It employs Spike-Timing-Dependent Plasticity (STDP), membrane leak, refractory periods, and homeostatic regulation.
- **SHA-256 Authentication**: A cryptographic login system built from scratch.
- **Hierarchical VFS & Isolation**: A RAM-based file system with strict UID-based file isolation and a TUI Finder (Text-Mode Window Manager).

### Build & Execution

Requires `i686-elf-gcc` and `qemu`.

```bash
make
qemu-system-i386 -kernel neuro-os.bin
```

### Core Commands

Once authenticated, use the shell to interact with the system:
- `neuro`: Step the Liquid State Machine and visualize the total liquid spikes and output membrane voltages.
- `neuro_reset`: Reset the LSM weights, topology, and state.
- `ps`: View the tasks currently being scheduled by the Neuromorphic Lottery Scheduler.
- `finder`: Launch the TUI file manager.
- `help`: List all available standard OS commands (`mkdir`, `touch`, `write`, `cat`, etc.).
