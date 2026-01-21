# Lab 05 - Process Synchronization

## Objectives
The objective of this lab is to explore process synchronization using forks, pipes, and condition variables.

## Checkpoints
1. **Checkpoint 1**: Verify that `fork` and `pipe` are functioning correctly.
2. **Checkpoint 2**: Ensure that the condition variable works by having a worker thread wait and be woken up.
3. **Checkpoint 3**: Test the graceful shutdown mechanism for the program, ensuring that `Ctrl+C` terminates all child processes and joins threads cleanly.

## How to Run the Program
1. Clone the repository if you haven't already:
   ```bash
   git clone https://github.com/moisiFerenc/lab_05.git
   cd lab_05
   ```
2. Build the program:
   ```bash
   make
   ```
3. Run the program:
   ```bash
   ./main
   ```

Follow through the checkpoints above as you run the program to observe its behavior and ensure all components are functioning as expected.

Good luck!