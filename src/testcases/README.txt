MATPOWER test cases (standard positive-sequence power-flow data)
================================================================

Source: https://github.com/idzafic/modelSolver  (Converters/old/PF/cases/)
These are standard MATPOWER Case Format v2 files (mpc.bus / mpc.gen / mpc.branch).

Used to test the short-circuit calculation:
  - case5.m    5-bus
  - case9.m    9-bus, 3 generators (classic WSCC 3-machine)
  - case30.m   IEEE 30-bus
  - case118.m  IEEE 118-bus
  - case300.m  IEEE 300-bus

For the symmetrical (balanced three-phase) short circuit we only use the
positive-sequence branch/bus data (r, x, b, tap, shift, shunts) to build Y_bus.
