# include <cstdlib>
# include <iostream>
# include <iomanip>
# include <ctime>
#include <fstream>

using namespace std;

#include "mpi.h"

#define DEBUG 0

int **inputArray;
int **outputArray;

int countLines(const char *filename)
{
    int totalLines = 0;
    string line;
    ifstream myfile(filename);

    while (getline(myfile, line))
    {
        ++totalLines;
    }

    return totalLines;
}

void freeMem(int n)
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

void printArray(int n)
{
    for (int x = 0; x < n; ++x)
    {
        for (int y = 0; y < n; ++y)
            cout << inputArray[x][y];
        cout << "\n";
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

    string inputFilepath;
    char outputFilepath[64];
    int n;
    int m;
    int k;
    string buffer;
    int x = 0;
    int y = 0; 
    MPI_Status stat;

    ifstream input;

    MPI::Init(argc, argv); //  Initialize MPI.
    p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
    id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.
   
    if (id == 0)
    {
        cout << "N: ";
        cin >> buffer;
        n = atoi(buffer.c_str());
        cout << "k: ";
        cin >> buffer;
        k = atoi(buffer.c_str());
        cout << "m: ";
        cin >> buffer;
        m = atoi(buffer.c_str());
        cout << "Input File: ";
        cin >> inputFilepath;
        
        //n = countLines(inputFilepath.c_str());

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

#if DEBUG == 1
        printArray(n);
        cout << "==========\n"; 
#endif

        //Split and send data to other processors
        for (int i = 1; i < p; ++i)
        {
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&m, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            for (int j = 0; j < n; ++j)
            {
                MPI_Send(inputArray[j], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
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

        for (int j = 0; j < n; ++j)
        {
            MPI_Recv(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        }
    }

    //Game of Life logic
    for (int itr = 0; itr < k; ++itr)
    { 
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

        //Every m iteration print the time and print the matrix to an output file
        if ((m != 0) && ((itr % m) == 0))
        {
            //Send configuration to processor 0
            if (id != 0)
            {
                for (int j = 0; j < n; ++j)
                {
                    MPI_Send(inputArray[j], n, MPI_INT, 0, id, MPI_COMM_WORLD);
                }
            }

            if (id == 0)
            {
                //Gather information from other processors 
                for (int q = 1; q < p; ++q)
                {
                    for (int j = 0; j < n; ++j)
                    {
                        MPI_Recv(inputArray[j], n, MPI_INT, MPI_ANY_SOURCE, q, MPI_COMM_WORLD, &stat);
                    }
                }
        
                //Print time
                wtime2 = MPI::Wtime() - wtime;
                cout << "  Elapsed wall clock time = " << wtime2 << " seconds.\n";

                /// Write to output file
                sprintf(outputFilepath, "Iteration %d of %d output.txt", itr, k); 
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
#if DEBUG == 1
            printArray(n);
            cout << "==========" << endl;
#endif
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
#if DEBUG == 1
    printArray(n);
    cout << "==========\n";
#endif
    }

    if (id == 0)
    {
        freeMem(n);
    }

    //  Terminate MPI.
    MPI::Finalize();

    return 0;
}

