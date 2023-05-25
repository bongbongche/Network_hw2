#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int **initializeGraph(int numOfNodes)
{
    int **graph = (int **)malloc(sizeof(int *) * numOfNodes);
    for (int i = 0; i < numOfNodes; i++)
    {
        graph[i] = (int *)malloc(sizeof(int) * numOfNodes);
    }

    for (int i = 0; i < numOfNodes; i++)
    {
        for (int j = 0; j < numOfNodes; j++)
        {
            graph[i][j] = 999;
        }
        graph[i][i] = 0;
    }

    return graph;
}

void fillGraph(int **graph, int src, int dst, int cost)
{
    if (cost == -999)
    {
        graph[src][dst] = 999;
        graph[dst][src] = 999;
        return;
    }

    graph[src][dst] = cost;
    graph[dst][src] = cost;
}

void freeGraph(int **graph, int numOfNodes)
{
    for (int i = 0; i < numOfNodes; i++)
    {
        free(graph[i]);
    }
    free(graph);
}

void distanceVector(int **graph, int numOfNodes, int ***routingTable)
{
    // first routing table
    for (int i = 0; i < numOfNodes; i++)
    {
        for (int j = 0; j < numOfNodes; j++)
        {
            routingTable[i][j][0] = j;
            routingTable[i][j][1] = j;
            routingTable[i][j][2] = graph[i][j];
        }
    }

    // all nodes exchange their routing table until no change
    bool change = true;
    while (change)
    {
        change = false;
        for (int i = 0; i < numOfNodes; i++)
        {
            for (int j = 0; j < numOfNodes; j++)
            {
                for (int k = 0; k < numOfNodes; k++)
                {
                    if (routingTable[i][j][2] > routingTable[i][k][2] + routingTable[k][j][2])
                    {
                        routingTable[i][j][2] = routingTable[i][k][2] + routingTable[k][j][2];
                        routingTable[i][j][1] = routingTable[i][k][1];
                        change = true;
                    }
                }
            }
        }
    }
}

// write routing table to output file
void writeRoutingTable(int ***routingTable, int numOfNodes, FILE *outputfile)
{
    for (int i = 0; i < numOfNodes; i++)
    {
        for (int j = 0; j < numOfNodes; j++)
        {
            if (routingTable[i][j][2] == 999)
                continue;
            fprintf(outputfile, "%d %d %d\n", routingTable[i][j][0], routingTable[i][j][1], routingTable[i][j][2]);
        }
        fprintf(outputfile, "\n");
    }
}

// free routing table
void freeRoutingTable(int ***routingTable, int numOfNodes)
{
    for (int i = 0; i < numOfNodes; i++)
    {
        for (int j = 0; j < numOfNodes; j++)
        {
            free(routingTable[i][j]);
        }
        free(routingTable[i]);
    }
    free(routingTable);
}

// message transmission simulation
void messageTransmission(FILE *outputfile, FILE *messagesfile, int ***routingTable)
{
    int src = 0;
    int dst = 0;
    char message[1000];
    while (fscanf(messagesfile, "%d %d %[^\n]", &src, &dst, message) != EOF)
    {
        // when path does not exist
        if (routingTable[src][dst][2] == 999)
        {
            fprintf(outputfile, "from %d to %d cost infinite hops unreachable message %s\n", src, dst, message);
            continue;
        }

        fprintf(outputfile, "from %d to %d cost %d hops ", src, dst, routingTable[src][dst][2]);
        fprintf(outputfile, "%d ", src);
        int next = routingTable[src][dst][1];
        while (next != dst)
        {
            fprintf(outputfile, "%d ", next);
            next = routingTable[next][dst][1];
        }
        fprintf(outputfile, "message %s\n", message);
    }
    fprintf(outputfile, "\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: distvec topologyfile messagesfile changesfile");
        exit(1);
    }

    FILE *topologyfile = fopen(argv[1], "r");
    FILE *messagesfile = fopen(argv[2], "r");
    FILE *changesfile = fopen(argv[3], "r");
    FILE *outputfile = fopen("output_dv.txt", "w");

    if (topologyfile == NULL || messagesfile == NULL || changesfile == NULL)
    {
        fprintf(stderr, "Error: open file failed.");
        exit(1);
    }

    // initialize graph
    int numOfNodes = 0;
    fscanf(topologyfile, "%d", &numOfNodes);
    int **graph = initializeGraph(numOfNodes);

    // routing tables. [0]: destination, [1]: next, [2]: cost
    int ***routingTable = (int ***)malloc(sizeof(int **) * numOfNodes);
    for (int i = 0; i < numOfNodes; i++)
    {
        routingTable[i] = (int **)malloc(sizeof(int *) * numOfNodes);
        for (int j = 0; j < numOfNodes; j++)
        {
            routingTable[i][j] = (int *)malloc(sizeof(int) * 3);
        }
    }

    // fill graph
    while (1)
    {
        int src = 0;
        int dst = 0;
        int cost = 0;
        fscanf(topologyfile, "%d %d %d", &src, &dst, &cost);
        fillGraph(graph, src, dst, cost);
        if (feof(topologyfile))
            break;
    }

    // run distance vector algorithm
    distanceVector(graph, numOfNodes, routingTable);

    // write routing table to output file
    writeRoutingTable(routingTable, numOfNodes, outputfile);

    // message transmission simulation.
    messageTransmission(outputfile, messagesfile, routingTable);

    // change path and cost
    while (1)
    {
        int src = 0;
        int dst = 0;
        int cost = 0;
        fscanf(changesfile, "%d %d %d", &src, &dst, &cost);
        if (feof(changesfile))
            break;
        fillGraph(graph, src, dst, cost);

        // run distance vector algorithm
        distanceVector(graph, numOfNodes, routingTable);

        // write routing table to output file
        writeRoutingTable(routingTable, numOfNodes, outputfile);

        // message transmission simulation
        fseek(messagesfile, 0, SEEK_SET);
        messageTransmission(outputfile, messagesfile, routingTable);
    }

    freeGraph(graph, numOfNodes);
    freeRoutingTable(routingTable, numOfNodes);
    fclose(topologyfile);
    fclose(messagesfile);
    fclose(changesfile);

    printf("Complete. Output file written to output_dv.txt.\n");

    return 0;
}