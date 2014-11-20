Frederic Marchand
ID# 100817579
Comp4009A3
Game of Life Simulation MPI

Run 'make' to build the code
To run the program, call ./main <N> <k> <m> "<inputFile>"

For example:

mpiexec -n 8 main 10 100 0 "test 1 input.txt"

mpirun -np 8 --hostfile hostfile main 10 100 0 "test 1 input.txt"

The output files are saved as: Iteration # of <k> output.txt
