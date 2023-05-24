#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NEW_ALB (8)
#define NEW_NEGRU (9)

#define STANGA (0)
#define SUS (1)
#define DREAPTA (2)
#define JOS (3)

#define CEAS (1)
#define TRIGONOMETRIC (0)

char in[256] = "";
char out[256] = "";
int *vect = NULL;
int *antVect = NULL;
int cAnt = 0;
int nr_ant = 0;

int N, M, IT;
int **matrix;

int start = 0, end = 0;
int sens;

void getArgs(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Not enough paramters: ./program IN_FILENAME OUT_FILENAME\n");
        exit(1);
    }

    strcpy(in, argv[1]);
    strcpy(out, argv[2]);
}

void init(int N, int M)
{
    matrix = malloc(sizeof(int *) * N);
    if (matrix == NULL)
    {
        printf("malloc failed!");
        exit(1);
    }

    int i, j;
    for (i = 0; i < N; i++)
    {
        matrix[i] = malloc(sizeof(int) * M);
        if (matrix[i] == NULL)
        {
            printf("malloc failed!");
            exit(1);
        }
    }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            matrix[i][j] = 0;
        }
    }
}

int testColor(char *c)
{
    if (c[0] == '0')
    {
        c[0] = '9';
    }
    else if (c[0] == '1')
    {
        c[0] = '8';
    }

    int aux = atoi(c);
    return aux;
}

void readFromFile()
{
    FILE *fp;
    int test;

    fp = fopen(in, "r");
    if (fp == NULL)
    {
        printf("Eroare descriptor READ.\n");
    }

    test = fscanf(fp, "%d %d %d", &N, &M, &IT);
    if (test == EOF)
    {
        printf("Eroare citire fisier.\n");
    }

    init(N, M);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            char c[20] = "";
            test = fscanf(fp, "%s", c);
            if (test == EOF)
            {
                printf("Eroare citire fisier.\n");
            }
            int a = testColor(c);
            matrix[i][j] = a;
        }
    }

    fclose(fp);
}

int *sendVector(int start, int end)
{
    int *v = (int *)malloc(sizeof(int) * M);
    int nr_elem = 0;

    for (int i = start; i < end; i++)
    {
        for (int j = 0; j < M; j++)
        {
            v[nr_elem] = matrix[i][j];
            nr_elem++;
        }
        v = (int *)realloc(v, sizeof(int) * (nr_elem + M));
    }

    return v;
}

void sendLines(int nProcesses)
{
    int *v = NULL;
    for (int i = 1; i < nProcesses; i++)
    {
        int start = i * (N / nProcesses);
        int end = (i + 1) * (N / nProcesses);
        if (i == nProcesses - 1)
            end += N % nProcesses;

        v = sendVector(start, end);
        MPI_Send(v, M * (end - start), MPI_INT, i, 0, MPI_COMM_WORLD);
    }
}

void receiveLines(int *v, int start, int end)
{
    int k = 0;
    for (int i = start; i < end; i++)
        for (int j = 0; j < M; j++)
        {
            matrix[i][j] = v[k];
            k++;
        }
}

char *finalColor(int a)
{
    char *c = (char *)malloc(sizeof(char) * 20);
    strcpy(c, "");
    sprintf(c, "%d", a);
    if (c[0] == '9')
    {
        c[0] = '0';
    }
    else if (c[0] == '8')
    {
        c[0] = '1';
    }

    return c;
}

void writeToFile()
{
    FILE *fp;
    fp = fopen(out, "w");

    fprintf(fp, "%d %d\n", N, M);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            char c[20] = "";
            strcpy(c, finalColor(matrix[i][j]));
            fprintf(fp, "%s ", c);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

int countAnt(int *vect, int nr)
{
    int aux = 0;
    for (int i = 0; i < nr; i++)
    {
        int nr = vect[i];
        while (nr != 0)
        {
            if (nr % 10 != 9 && nr % 10 != 8)
            {
                aux++;
            }
            nr /= 10;
        }
    }
    return aux;
}

int changeColor(int e)
{
    int a = 0;

    if (e == NEW_ALB)
    {
        a = NEW_NEGRU;
    }
    else if (e == NEW_NEGRU)
    {
        a = NEW_ALB;
    }
    else
    {
        printf("Elementul %d are o culoare neobisnuita.\n", e);
    }

    return a;
}

void printVector(int nr)
{
    for (int i = 0; i < nr * 3; i += 3)
    {
        printf("Vector furnici: %d %d %d\n", antVect[i], antVect[i + 1], antVect[i + 2]);
    }
}

void moveCeas(int x, int y, int e)
{
    if (e == STANGA)
    {
        if (x != 0)
        {
            antVect[cAnt++] = x - 1;
            antVect[cAnt++] = y;
            antVect[cAnt++] = SUS;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == SUS)
    {
        if (y != M - 1)
        {
            antVect[cAnt++] = x;
            antVect[cAnt++] = y + 1;
            antVect[cAnt++] = DREAPTA;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == DREAPTA)
    {
        if (x != N - 1)
        {
            antVect[cAnt++] = x + 1;
            antVect[cAnt++] = y;
            antVect[cAnt++] = JOS;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == JOS)
    {
        if (y != 0)
        {
            antVect[cAnt++] = x;
            antVect[cAnt++] = y - 1;
            antVect[cAnt++] = STANGA;
        }
        else
        {
            nr_ant--;
        }
    }
}

void moveTrigonometric(int x, int y, int e)
{
    if (e == STANGA)
    {
        if (x != N - 1)
        {
            antVect[cAnt++] = x + 1;
            antVect[cAnt++] = y;
            antVect[cAnt++] = JOS;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == SUS)
    {
        if (y != 0)
        {
            antVect[cAnt++] = x;
            antVect[cAnt++] = y - 1;
            antVect[cAnt++] = STANGA;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == DREAPTA)
    {
        if (x != 0)
        {
            antVect[cAnt++] = x - 1;
            antVect[cAnt++] = y;
            antVect[cAnt++] = SUS;
        }
        else
        {
            nr_ant--;
        }
    }
    else if (e == JOS)
    {
        if (y != M - 1)
        {
            antVect[cAnt++] = x;
            antVect[cAnt++] = y + 1;
            antVect[cAnt++] = DREAPTA;
        }
        else
        {
            nr_ant--;
        }
    }
}

void move(int x, int y, int value)
{
    if (sens == CEAS)
    {
        moveCeas(x, y, value);
    }
    else if (sens == TRIGONOMETRIC)
    {
        moveTrigonometric(x, y, value);
    }
    else
    {
        printf("Tip necunoascut de deplasare.\n");
    }
}

void findSens(int e)
{
    while (e % 10 != 9 && e % 10 != 8)
    {
        e /= 10;
    }

    if (e == 8)
    {
        sens = CEAS;
    }
    else if (e == 9)
    {
        sens = TRIGONOMETRIC;
    }
    else
    {
        printf("Culoarea nu exista.\n");
    }
}

void populate(int nr_elem)
{
    int p = 0;
    for (int i = 0; i < nr_elem; i++)
    {
        int y = i % M;
        int count = 0;
        findSens(vect[i]);
        while (vect[i] % 10 != 9 && vect[i] % 10 != 8)
        {
            int x = floor(i / N) + start;
            int value = vect[i] % 10;
            move(x, y, value);
            p += 3;

            vect[i] /= 10;
            count++;
        }

        if (count != 0)
        {
            vect[i] = changeColor(vect[i]);
        }
    }
}

void moveOnMatrix(int *a, int nr_ant)
{
    for (int i = 0; i < nr_ant * 3; i += 3)
    {
        matrix[a[i]][a[i + 1]] *= 10;
        matrix[a[i]][a[i + 1]] += a[i + 2];
    }
}

int main(int argc, char *argv[])
{
    int rank;
    int nProcesses;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    MPI_Status status;

    if (rank == 0)
    {
        getArgs(argc, argv);
        readFromFile();
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&IT, 1, MPI_INT, 0, MPI_COMM_WORLD);

    while (IT > 0)
    {
        cAnt = 0;
        if (N - 1 < nProcesses)
        {
            start = rank;
            end = (rank + 1);

            if (rank == N - 1)
                end += 1;
        }
        else
        {
            start = rank * (N / nProcesses);
            end = (rank + 1) * (N / nProcesses);

            if (rank == nProcesses - 1)
                end += N % nProcesses;
        }

        if (rank > N - 1)
        {
            free(vect);
            MPI_Finalize();
            return 0;
        }

        int nr_lines = end - start;

        int nr_elem = nr_lines * M;

        if (rank == 0)
        {
            vect = sendVector(start, end);
            sendLines(nProcesses);
        }

        if (rank > 0)
        {
            vect = (int *)malloc(sizeof(int) * nr_elem);
            MPI_Recv(vect, nr_elem, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
        }

        nr_ant = countAnt(vect, nr_elem);
        antVect = malloc(sizeof(int) * nr_ant * 3);

        if (nr_ant != 0)
        {
            populate(nr_elem);
        }

        if (rank > 0)
        {
            MPI_Send(vect, M * nr_lines, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        if (rank == 0)
        {
            receiveLines(vect, start, end);
            for (int i = 1; i < nProcesses; i++)
            {
                start = i * (N / nProcesses);
                end = (i + 1) * (N / nProcesses);

                if (i == nProcesses - 1)
                    end += N % nProcesses;

                nr_lines = end - start;
                nr_elem = nr_lines * M;
                free(vect);
                vect = (int *)malloc(sizeof(int) * nr_elem);
                MPI_Recv(vect, nr_elem, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
                receiveLines(vect, start, end);
            }
        }

        if (rank > 0)
        {
            MPI_Send(&nr_ant, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            if (nr_ant != 0)
            {
                MPI_Send(antVect, nr_ant * 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }

        if (rank == 0)
        {
            if (nr_ant != 0)
            {
                moveOnMatrix(antVect, nr_ant);
            }
            for (int i = 1; i < nProcesses; i++)
            {
                MPI_Recv(&nr_ant, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                if (nr_ant != 0)
                {
                    int *a = malloc(sizeof(int) * nr_ant * 3);
                    MPI_Recv(a, nr_ant * 3, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    moveOnMatrix(a, nr_ant);
                    free(a);
                }
            }
        }

        IT--;
        free(antVect);
        free(vect);
    }

    if (rank == 0)
    {
        writeToFile();
    }

    MPI_Finalize();
    return 0;
}