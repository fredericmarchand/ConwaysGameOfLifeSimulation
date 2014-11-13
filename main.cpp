# include <cstdlib>
# include <iostream>
# include <iomanip>
# include <ctime>
#include <fstream>

using namespace std;

#include "mpi.h"

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

void stringToCharArray(string s, char *a)
{
    a[s.size()] = 0;
    memcpy(a, s.c_str(), s.size());
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
    double wtime;

    char * inputFilepath;
    char * outputFilepath;
    int n;
    string buffer;
    int x = 0;
    int y = 0; 
    
    ifstream input;

    MPI::Init(argc, argv); //  Initialize MPI.
    p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
    id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.
   
    if (id == 0)
    {
        if (argc <= 2) 
        {
            cerr << "The program is missing an argument" << endl; 
            return 1;   /// Error
        }
   
        inputFilepath = argv[1]; 
        outputFilepath = argv[2];

        n = countLines(inputFilepath);

        input.open(inputFilepath);

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
                inputArray[x][y] = atoi(temp);
                outputArray[x][y] = 0;
            }
            x++;
        }

        input.close();

        printArray(n);
    

    //Game of Life logic
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            int neighbors = 0;

            for (int x = i-1; x < i+1; ++x)
            {
                for (int y = j-1; y < j+1; ++y)
                {
                    if (x < 0 || y < 0 || x >= n || y >= n)
                        continue;
                    if ((i != x && j != y) && inputArray[x][y] == 1)
                    {
                        ++neighbors;
                    }
                }
            }

            if (inputArray[i][j] == 1)
            {
                if (neighbors >= 4 || neighbors <= 1)
                {
                    inputArray[i][j] = 0;
                }
            }
            else if (inputArray[i][j] == 0)
            {
                if (neighbors == 3)
                {
                    inputArray[i][j] = 1;
                }
            }
        }
    }
    
    printArray(n);
    }

    /*if (id == 0)
    {
        wtime = MPI::Wtime() - wtime;
        cout << "  Elapsed wall clock time = " << wtime << " seconds.\n";
    }*/

    if (id == 0)
    {
        /*/// Write to output file
        ofstream outputFile (outputFilepath);
        if (!outputFile.is_open())
        {
            cerr << "Failed to open output file" << endl;
            return 1;   /// Error
        }
    
        outputFile << n << endl;
        for (int i = 0; i < n; ++i)
        {
            outputFile << inputArray[i] << endl;
        }

        outputFile.close();*/

        freeMem(n);
    }

    //  Terminate MPI.
    MPI::Finalize();

    return 0;
}

