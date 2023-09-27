# Computer Architecture Final Project

This repository contains my final project for a computer architecture course. In this project, I developed a simulator for a floating-point processor that utilizes a scoreboard for managing instructions.

## Project Overview

- **Language**: The project is written in the C language.
- **Application**: It is built as a command-line application named `sim.exe`. The full line is: `sim.exe cfg.t cfg.txt memin.txt memout.txt regout.txt traceinst.txt traceunit.txt`
- **Input Files**: To run this application, you will need to provide six input files (listed at the end of this README).
- **Processor Units**: The processor consists of several basic units, including:

  - **Internal Clock**: The processor operates on an internal clock where each cycle executes one instruction.
  - **Instruction Format**: Each instruction consists of 32 bits, comprising the OPCODE (mathematical operation), destination register, two source registers, and an immediate number.
  - **Supported Operations**: The processor supports six operations, including LD (load from hard disk) and ST (store to hard disk).

## Input Files

To run the simulator, you will need to provide the following files:

1. **cfg.txt (Input)**: This file specifies the number of calculation units for different types of calculations and their respective delays. You can configure these parameters to fit your needs.

2. **memin.txt (Input)**: This file specifies the initial state of the 4096 lines of the hard disk memory at the beginning of the run.

3. **memout.txt (Output)**: This file specifies the final state of the 4096 lines of the hard disk memory after the simulation run.

4. **regout.txt (Output)**: This file specifies the final state of the 16 registers of the processor after the simulation run.

5. **traceinst.txt (Output)**: This file traces all the instructions the processor operates on. It records when the instruction was fetched, read, issued, executed, and when the result was written.

6. **traceunit.txt (Output)**: This file specifies what each calculation unit executed during every cycle, according to the scoreboard protocol. Each line contains the registers that were read, the target register, and the actual values stored in those registers.

Feel free to customize the input files to test different configurations and scenarios with the simulator.

By following this README and providing the required input files, you can effectively use the `sim.exe` simulator for your computer architecture project.
