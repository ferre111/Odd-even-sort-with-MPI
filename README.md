# Odd-even-sort-with-MPI
Implementation of odd even sort algorithm with MPI.

The amount of data must be divisible by the number of processes. If not, some data will not be sorted.

# Example of usage
Load MPI module:

`module load mpi/openmpi-x86_64`

Compile:

`mpicc main.c -o main`

Run with selected number of processes:

`mpiexec -np 8 --oversubscribe ./main`
