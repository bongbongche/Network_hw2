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

void fillGraph(int **graph, FILE *topologyfile)
{
    while (!feof(topologyfile))
    {
        int src = 0;
        int dst = 0;
        int cost = 0;
        fscanf(topologyfile, "%d %d %d", &src, &dst, &cost);
        graph[src][dst] = cost;
        graph[dst][src] = cost;
    }
}

void freeGraph(int **graph, int numOfNodes)
{
    for (int i = 0; i < numOfNodes; i++)
    {
        free(graph[i]);
    }
    free(graph);
}

int findMinDistance(int distance[], bool visited[], int numOfNodes)
{
    int min = 999;
    int min_index = 0;
    for (int i = 0; i < numOfNodes; i++)
    {
        if (visited[i] == false && distance[i] <= min)
        {
            // Tie breaking rule
            if (distance[i] == min && i > min_index)
                continue;

            min = distance[i];
            min_index = i;
        }
    }
    return min_index;
}

void recordNextHop(int parent[], int src, int dst, int **routingTable)
{
    if (parent[dst] == -1)
    {
        routingTable[dst][0] = dst;
        routingTable[dst][1] = dst;
        return;
    }

    if (parent[dst] == src)
    {
        routingTable[dst][0] = dst;
        routingTable[dst][1] = dst;
        return;
    }

    recordNextHop(parent, src, parent[dst], routingTable);
    routingTable[dst][0] = dst;
    routingTable[dst][1] = routingTable[parent[dst]][1];
}

void dijkstra(int **graph, int src, int numOfNodes, int **routingTable)
{
    int distance[numOfNodes];
    bool visited[numOfNodes];
    int parent[numOfNodes];

    for (int i = 0; i < numOfNodes; i++)
    {
        distance[i] = 999;
        visited[i] = false;
        parent[i] = -1;
    }

    distance[src] = 0;

    for (int i = 0; i < numOfNodes - 1; i++)
    {
        int idx = findMinDistance(distance, visited, numOfNodes);
        visited[idx] = true;

        for (int j = 0; j < numOfNodes; j++)
        {
            // Tie breaking rule
            if (!visited[j] && graph[idx][j] && distance[idx] != 999 && distance[idx] + graph[idx][j] < distance[j])
            {
                distance[j] = distance[idx] + graph[idx][j];
                parent[j] = idx;
            }
        }
    }

    // record next hop
    for (int i = 0; i < numOfNodes; i++)
    {
        recordNextHop(parent, src, i, routingTable);
    }

    // record total cost
    for (int i = 0; i < numOfNodes; i++)
    {
        routingTable[i][2] = distance[i];
    }
}

// write routing table to output file
void writeRoutingTable(int ***routingTable, int numOfNodes, FILE *outputfile)
{
    for (int i = 0; i < numOfNodes; i++)
    {
        for (int j = 0; j < numOfNodes; j++)
        {
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
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "usage: linkstate topologyfile messagesfile changesfile");
        exit(1);
    }

    FILE *topologyfile = fopen(argv[1], "r");
    FILE *messagesfile = fopen(argv[2], "r");
    FILE *changesfile = fopen(argv[3], "r");
    FILE *outputfile = fopen("output_ls.txt", "w");

    if (topologyfile == NULL || messagesfile == NULL || changesfile == NULL)
    {
        fprintf(stderr, "Error: open file failed.");
        exit(1);
    }

    // initialize graph
    int numOfNodes = 0;
    fscanf(topologyfile, "%d", &numOfNodes);
    int **graph = initializeGraph(numOfNodes);

    // routing tables. [0]: destination, [1]: next hop, [2]: cost
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
    fillGraph(graph, topologyfile);

    // run dijkstra algorithm
    for (int i = 0; i < numOfNodes; i++)
    {
        dijkstra(graph, i, numOfNodes, routingTable[i]);
    }

    // write routing table to output file
    writeRoutingTable(routingTable, numOfNodes, outputfile);

    // message transmission simulation
    messageTransmission(outputfile, messagesfile, routingTable);

    printf("hi");

    freeGraph(graph, numOfNodes);
    freeRoutingTable(routingTable, numOfNodes);
    fclose(topologyfile);
    fclose(messagesfile);
    fclose(changesfile);

    return 0;
}