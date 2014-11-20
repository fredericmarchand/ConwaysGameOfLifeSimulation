#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <cmath>

using namespace std;

#include "mpi.h"

#define DEBUG 0 

void twoDimensionalCopy(int **input, int **output, int rows, int n)
{
    for (int x = 0; x < rows; ++x)
    {
        for (int y = 0; y < n; ++y)
        {
            output[x][y] = input[x][y];
        }
    }
}

void runGameLogic(int **inputArray, int **outputArray, int *rowAbove, int *rowBelow, int totalRows, int n)
{
    for (int i = 0; i < totalRows; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            int neighbors = 0;

            //Count the neighbors of each tile in the matrix
            for (int x = i-1; x <= i+1; ++x)
            {
                for (int y = j-1; y <= j+1; ++y)
                {
                    if (y < 0 || y >= n)
                        continue;

                    if (x < 0)
                    {
                        if (rowAbove[y] == 1)
                        {
                            neighbors += 1;
                        }
                    }
                    else if (x >= totalRows)
                    {
                        if (rowBelow[y] == 1)
                        {
                            neighbors += 1;
                        }
                    }
                    else
                    {
                        if (inputArray[x][y] == 1)
                        {
                            if (!(x == i && y == j))
                                neighbors += 1;
                        }
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

    twoDimensionalCopy(outputArray, inputArray, totalRows, n);
}

void freeMainArray(int **array, int n)
{
    for (int i = 0; i < n; ++i)
    {
        delete [] array[i];
    }
    delete [] array;
}

void outputArrayToFile(int **mainArray, int itr, int k, int n)
{
    char outputFilepath[64]; 
    sprintf(outputFilepath, "Iteration %d of %d output.txt", itr+1, k); 
    ofstream outputFile (outputFilepath);
    if (!outputFile.is_open())
    {
        cerr << "Failed to open output file" << endl;
        exit(1);
    }

    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
        {
            outputFile << mainArray[x][y];
        }
        outputFile << "\n";
    }
    outputFile.close();
}

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
            {
                return;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int id;
    int p;
    double wtime, wtime2;

    char *inputFilepath;
    int n;
    int m;
    int k;
    string buffer;
    int x = 0;
    int y = 0; 
    MPI_Status stat;

    ifstream input;

    int **mainArray;
    int **inputArray = NULL;
    int **outputArray = NULL;
    int *rowAbove = NULL;
    int *rowBelow = NULL;

    int startRow = 0;
    int totalRows = 0;

    int sendRow;
    int sendTotalRows;

    MPI::Init(argc, argv); //  Initialize MPI.
    p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
    id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.
   
    if (id == 0)
    {
        if (argc != 5)
        {
            cerr << "Missing parameters" << endl << "./main <N> <k> <m> <inputFile>" << endl;
            return 1;
        }

        n = atoi(argv[1]);
        k = atoi(argv[2]);
        m = atoi(argv[3]);
        inputFilepath = argv[4];
        
        /*n = 10;
        k = 100;
        m = 0; */

        mainArray = new int*[n];

        for (int i = 0; i < n; ++i)
        {
            mainArray[i] = new int[n];
        }   
    
        input.open(inputFilepath);

        if (!input.is_open())
        {
            cerr << "Could not open file " << inputFilepath << endl;
            return 1;   /// Error
        }

        x = 0;
        while (getline(input, buffer))
        {   
            for (y = 0; y < n; ++y)
            {
                char temp[2] = { buffer[y], '\0' };
                int val = atoi(temp);
                mainArray[x][y] = val;
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
    }

    getRowsForProcessor(id, &startRow, &totalRows, n, p);

    inputArray = new int*[totalRows];
    outputArray = new int*[totalRows];

    for (int i = 0; i < totalRows; ++i)
    {
        inputArray[i] = new int[n];
        outputArray[i] = new int[n];
    }

    rowAbove = new int[n];
    rowBelow = new int[n];
   
    if (id == 0)
    {
        int emptyRow[n];
        memset(emptyRow, 0, n);

        //Split and send data to other processors
        for (int i = 0; i < p; ++i)
        {
            getRowsForProcessor(i, &sendRow, &sendTotalRows, n, p);

            if (sendTotalRows == 0)
                continue;

            if (i == 0)
            {
                MPI_Send(emptyRow, n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            else
            {
                MPI_Send(mainArray[sendRow-1], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            
            for (int j = sendRow; j < (sendRow + sendTotalRows); ++j)
            {
                MPI_Send(mainArray[j], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }

            if (i == p-1)
            {
                MPI_Send(emptyRow, n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
            else
            {
                MPI_Send(mainArray[sendRow + sendTotalRows - 1], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
    }

    if (totalRows > 0)
    {
        MPI_Recv(rowAbove, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        for (int j = 0; j < totalRows; ++j)
        {
            MPI_Recv(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
            for (int i = 0; i < n; ++i)
                outputArray[j][i] = inputArray[j][i];
        }
        MPI_Recv(rowBelow, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
    }
    else
    {
        freeMem(inputArray, outputArray, totalRows);
        delete [] rowBelow;
        delete [] rowAbove;
        if (id == 0)
        {
            freeMainArray(mainArray, n);
        } 
        MPI::Finalize(); 
        return 0;
    }

    //Game of Life logic
    for (int itr = 0; itr < k; ++itr)
    {
        //Refresh above and below
        int emptyRow[n];
        memset(emptyRow, 0, n);

        //Transfer row above and row below for each respective processor
        if (id != p-1)
            MPI_Send(inputArray[totalRows-1], n, MPI_INT, id+1, 0, MPI_COMM_WORLD);

        if (id != 0)
            MPI_Send(inputArray[0], n, MPI_INT, id-1, 0, MPI_COMM_WORLD);

        if (id != 0)
            MPI_Recv(rowAbove, n, MPI_INT, id-1, 0, MPI_COMM_WORLD, &stat);

        if (id != p-1)
            MPI_Recv(rowBelow, n, MPI_INT, id+1, 0, MPI_COMM_WORLD, &stat);

        //Game of Life stuff
        runGameLogic(inputArray, outputArray, rowAbove, rowBelow, totalRows, n);                

        //Every m iteration print the time and print the matrix to an output file
        if ((m != 0) && ((itr % m) == 0))
        {
            //Send configuration to processor 0
            for (int j = 0; j < totalRows; ++j)
            {
                MPI_Send(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }

            if (id == 0)
            {
                //Gather information from other processors 
                for (int q = 0; q < p; ++q)
                {
                    getRowsForProcessor(q, &sendRow, &sendTotalRows, n, p);
                    for (int j = sendRow; j < (sendRow + sendTotalRows); ++j)
                    {
                        MPI_Recv(mainArray[j], n, MPI_INT, q, 0, MPI_COMM_WORLD, &stat);
                    }
                }
#if DEBUG == 1
                printArray(mainArray, n); 
#endif
        
                //Print time
                wtime2 = MPI::Wtime() - wtime;
                cout << "  Elapsed wall clock time = " << wtime2 << " seconds.\n";

                /// Write to output file
                outputArrayToFile(mainArray, itr, k, n);
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

    freeMem(inputArray, outputArray, totalRows);
    delete [] rowBelow;
    delete [] rowAbove;
    if (id == 0)
    {
        freeMainArray(mainArray, n);
    }
    // Terminate MPI.
    MPI::Finalize();

    return 0;
}

