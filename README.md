# Neuro-OS

A novel, bare-metal operating system built from scratch with a built-in Spiking Neural Network (SNN) that ticks at the hardware level alongside a cooperative multitasking scheduler. 

### Features
- **Concurrent Neural Engine**: An SNN that utilizes biologically-inspired STDP and Homeostatic learning, ticking alongside standard kernel tasks.
- **SHA-256 Authentication**: A cryptographic login system built from scratch.
- **Hierarchical VFS & Isolation**: A RAM-based file system with directory traversal and strict UID-based file isolation.
- **TUI Finder**: A Graphical Text-Mode window manager to explore and open files.

### How to Run

You will need a cross-compiler (`i686-elf-gcc`) and `qemu` installed.

1. **Build the OS**:
   ```bash
   make
   ```
2. **Run in QEMU**:
   ```bash
   qemu-system-i386 -kernel neuro-os.bin
   ```

### Quick Walkthrough

1. **Setup**: On first boot, follow the on-screen prompt to create your root account (username & password).
2. **Commands**: Once logged in, type `help` to see all available terminal commands.
3. **Neural Engine**: Type `neuro` to watch the Spiking Neural Network dynamically adjust its threshold and synaptic weights.
4. **File Management**: Type `mkdir docs` and `touch test.txt` to create folders and files. Use `write test.txt Hello!` to add content.
5. **Finder UI**: Type `finder` to launch the graphical file explorer. Use `w` and `s` to scroll, and `Enter` to open files or navigate directories!
