#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <cmath>

using namespace std;

#include "mpi.h"

#define DEBUG 0 

void freeMem(int **inputArray, int **outputArray, int n)
{
    /// De-allocate the two dimensional arrays
    for (int i = 0; i < n; ++i)
    {
        delete [] inputArray[i];
        delete [] outputArray[i];
    }
    delete [] inputArray;
    delete [] outputArray;
}

void printArray(int **array, int n)
{
    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
            printf("%1d", array[x][y]);
        cout << "\n";
    }
}

void getRowsForProcessor(int procId, int *row, int *totalRows, int n, int p)
{
    //If there are less (or equal) rows than processors, distribute a row to each
    if (n <= p)
    {
        int rows = n;
        for (int i = 0; i < p; ++i, rows--)
        {
            if (i == procId)
            {
                *totalRows = (rows > 0 ? 1 : 0);
                *row = (rows > 0 ? i : 0);
                return;
            }
        }
    }
    else //If there are more rows than processors
    {
        int per = floor(n/p);
        int rem = n%p;
        *row = 0;
        int lastTotal = 0;
        for (int i = 0; i < p; ++i)
        {
            *totalRows = (per + (rem > 0 ? 1 : 0));
            rem--;
            *row = lastTotal;
            lastTotal = lastTotal + *totalRows;
            if (i == procId)
                return;
        }
    }
}

void twoDimensionalCopy(int **input, int **output, int n)
{
    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
        {
            output[x][y] = input[x][y];
        }
    }
}

int main(int argc, char *argv[])
{
    int id;
    int p;
    double wtime, wtime2;

    string inputFilepath = "test 3 input.txt";
    char outputFilepath[64];
    int n;
    int m;
    int k;
    string buffer;
    int x = 0;
    int y = 0; 
    MPI_Status stat;

    ifstream input;

    int **inputArray;
    int **outputArray;

    MPI::Init(argc, argv); //  Initialize MPI.
    p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
    id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.
   
    if (id == 0)
    {
        //cout << "N: ";
        //cin >> buffer;
        //n = atoi(buffer.c_str());
        //cout << "k: ";
        //cin >> buffer;
        //k = atoi(buffer.c_str());
        //cout << "m: ";
        //cin >> buffer;
        //m = atoi(buffer.c_str());
        //cout << "Input File: ";
        //cin >> inputFilepath;
        
        n = 20;
        k = 10;
        m = 1;

        int a, b;
        for (int i = 0; i < 8; ++i)
            getRowsForProcessor(i, &a, &b, 14, 8);

        input.open(inputFilepath.c_str());

        if (!input.is_open())
        {
            cerr << "Could not open file " << inputFilepath << endl;
            return 1;   /// Error
        }

        inputArray = new int*[n];
        outputArray = new int*[n];

        for (int i = 0; i < n; ++i)
        {
            inputArray[i] = new int[n];
            outputArray[i] = new int[n];
        }   
    
        x = 0;
        while (getline(input, buffer))
        {   
            for (y = 0; y < n; ++y)
            {
                char temp[2] = { buffer[y], '\0' };
                int val = atoi(temp);
                inputArray[x][y] = val;
                outputArray[x][y] = val;
            }
            x++;
        }

        input.close();

        for (int i = 1; i < p; ++i)
        {
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&m, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        wtime = MPI::Wtime();
    }

    if (id != 0)
    {
        MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        MPI_Recv(&k, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        MPI_Recv(&m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

        inputArray = new int*[n];
        outputArray = new int*[n];

        for (int i = 0; i < n; ++i)
        {
            inputArray[i] = new int[n];
            outputArray[i] = new int[n];
        }
    }

    //Game of Life logic
    for (int itr = 0; itr < k; ++itr)
    {
        if (id == 0)
        {
            //Split and send data to other processors
            for (int i = 1; i < p; ++i)
            {
                for (int j = 0; j < n; ++j)
                {
                    MPI_Send(inputArray[j], n, MPI_INT, i, 0, MPI_COMM_WORLD);
                }
            }
        }

        if (id != 0)
        {
            for (int j = 0; j < n; ++j)
            {
                MPI_Recv(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
                for (int i = 0; i < n; ++i)
                    outputArray[j][i] = inputArray[j][i];
            }
        }

        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                int neighbors = 0;

                //Count the neighbors of each tile in the matrix
                for (int x = i-1; x <= i+1; ++x)
                {
                    for (int y = j-1; y <= j+1; ++y)
                    {
                        if (x < 0 || y < 0 || x >= n || y >= n)
                            continue;

                        if (inputArray[x][y] == 1)
                        {
                            if (!(x == i && y == j))
                                neighbors += 1;
                        }
                    }
                }
#if DEBUG == 1 
                cout << "(" << i << "," << j << ") = " << inputArray[i][j] << " with neighbors: " << neighbors << endl;
#endif

                if (inputArray[i][j] == 1)
                {
                    //An organism must have 2 or 3 neighbors to survive to the next iteration
                    if (neighbors >= 4 || neighbors <= 1)
                    {
#if DEBUG == 1
                        cout << i << ", " << j << " -> 0" << endl; 
#endif
                        outputArray[i][j] = 0;
                    }
                }
                else if (inputArray[i][j] == 0)
                {
                    //An empty slot with 3 neighboring organisms spawns a new organism
                    if (neighbors == 3)
                    {
#if DEBUG == 1
                        cout << i << ", " << j << " -> 1" << endl; 
#endif
                        outputArray[i][j] = 1;
                    }
                }
            }
        }

        twoDimensionalCopy(outputArray, inputArray, n);
        if (id == 7) printArray(inputArray, n); 

        //Processors request updates from processor 0

        //Every m iteration print the time and print the matrix to an output file
        if ((m != 0) && ((itr % m) == 0))
        {
            //Send configuration to processor 0
            if (id != 0)
            {
                for (int j = 0; j < n; ++j)
                {
                    MPI_Send(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
            }

            if (id == 0)
            {
                //Gather information from other processors 
                for (int q = 1; q < p; ++q)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        MPI_Recv(inputArray[j], n, MPI_INT, q, 0, MPI_COMM_WORLD, &stat);
                    }
                }
        
                //Print time
                wtime2 = MPI::Wtime() - wtime;
                cout << "  Elapsed wall clock time = " << wtime2 << " seconds.\n";

                /// Write to output file
                sprintf(outputFilepath, "Iteration %d of %d output.txt", itr+1, k); 
                ofstream outputFile (outputFilepath);
                if (!outputFile.is_open())
                {
                    cerr << "Failed to open output file" << endl;
                    return 1;   /// Error
                }
    
                for (int x = 0; x < n; ++x)
                {
                    for (int y = 0; y < n; ++y)
                    {
                        outputFile << inputArray[x][y];
                    }
                    outputFile << "\n";
                }

                outputFile.close();
            }
        }
        else if (m == 0)
        {
            if (id == 0)
            {
                //Print time
                wtime2 = MPI::Wtime() - wtime;
                cout << "  Elapsed wall clock time = " << wtime2 << " seconds.\n";
            }
        }
    }

    freeMem(inputArray, outputArray, n);

    // Terminate MPI.
    MPI::Finalize();

    return 0;
}

