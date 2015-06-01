#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

typedef struct {
   double ProcDelay;
   double QueueDelay;
   double Jitter;
   int IsOptical;
   int nConnLinks;
   int* ConnLinks;
} OAARNode;

typedef struct  {
   int Capacity; 
   double PropDelay;
   double BandCost;
   int IsOptical;
   int Head;
   int Tail;
} OAARLink;

typedef struct {
   int Source;
   int Destination;
   double Priority;
   int BandWidth;
   double DelayPrice;
   double JitterPrice;
} OAARFlow;

void createOpticalNode(OAARNode* Node);
void createElecNode(OAARNode* Node);
void createOpticalLink(OAARLink* Link, int Head, int Tail, int i);
void createElecLink(OAARLink* Link, int Head, int Tail, int i);
void createFlow(OAARFlow* Flow);

OAARNode* Nodes;
OAARLink* Links;
OAARFlow* Flows;
char probName[16];
int nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows;

int main()
{
   int nElecNodes, nElecNodes1, nElecNodes2, nElecLinks;
   int i,j,k;

   /* read nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows */
   printf("Set prob name:\n");
   scanf("%s", probName);
   printf("\n");
   printf("Set nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows:\n");
   scanf("%d %d %d %d %d", &nNodes, &nOpticalNodes, &nLinks, &nOpticalLinks, &nFlows);
   printf("\n");

   nElecNodes = nNodes - nOpticalNodes;
   nElecLinks = nLinks - nOpticalLinks;
   nElecNodes1 = intRandom((int)(0.2*nElecNodes), (int)(0.6*nElecNodes));
   nElecNodes2 = nElecNodes - nElecNodes1;

   /* alloc memory for Nodes, Links, Flows */
   Nodes = (OAARNode*)malloc(nNodes*sizeof(OAARNode));
   Links = (OAARLink*)malloc(nLinks*sizeof(OAARLink));
   Flows = (OAARFlow*)malloc(nFlows*sizeof(OAARFlow));

   /* create network topology
    * 1.create a tree for nOpticalNodes 
    *   including nOpticalNodes optical nodes and nOpticalNodes-1 optical links
    * 2.add nElecNodes1 (20%~60% of total nElecNodes) elec nodes to the network
    *   these elec nodes are connected to optical nodes with optical links
    *   so we will add nElecNodes1 elec nodes and nElecNodes1 optical links here
    * 3.add remaining elec nodes to the network with elec links(only connected to elec nodes)
    *   nElecNodes2 = nElecNodes - nElecNodes1
    *   including nElecNodes2 elec nodes and nElecNodes2 elec links 
    * 4.to this step, we have already added all optical nodes and elce nodes.
    *   so we need to add remaining optical links(may between any nodes)
    *   including nOpticalLinks - (nOpticalNodes-1) - (nElecNodes1) - (0)
    * 5.add remaining elec links(only between elec nodes)
    *   including nElecLinks - (0) - (0) - (nElecNodes2)
    **/
   for( i = 0; i < nOpticalNodes; i++)
   {
      createOpticalNode(&Nodes[i]);
      if(i == 0) continue;
      /* create an optical link with head=i and tail<i */
      createOpticalLink(&Links[i-1], i, intRandom(0, i-1), i-1); 
   }
   for(i = nOpticalNodes; i < nOpticalNodes+nElecNodes1; i++)
   {
      createElecNode(&Nodes[i]);
      createOpticalLink(&Links[i-1], i, intRandom(0, nOpticalNodes-1), i-1);
   }
   for(i = nOpticalNodes+nElecNodes1; i < nOpticalNodes+nElecNodes1+nElecNodes2; i++)
   {
      createElecNode(&Nodes[i]);
      createElecLink(&Links[i-1], i, intRandom(nOpticalNodes, i-1), i-1);
   }
   for(i = nOpticalNodes-1+nElecNodes; i < nOpticalLinks+nElecNodes2; i++)
   {
      createOpticalLink(&Links[i], intRandom(0, nNodes-1), intRandom(0, nNodes-1), i);
   }
   for(i = nOpticalLinks+nElecNodes2; i < nOpticalLinks+nElecLinks; i++)
   {
      createElecLink(&Links[i], intRandom(nOpticalNodes, nNodes-1), intRandom(nOpticalNodes, nNodes-1), i);
   }

   /* generate random flow */
   for(i = 0; i < nFlows; i++)
   {
      createFlow(&Flows[i]);
   }

   /* print the result */
   printf("# probname\n");
   printf("%s\n", probName);
   printf("# nNodes nOpticalNodes nLinks nOpticalLinks nFlows\n");
   printf("%d %d %d %d %d\n", nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows);
   printf("####################\n");
   k = 0;
   for(i = 0; i < nNodes; i++)
   {
      printf("# node %d\n", k); k++;
      printf("%lf %lf %lf %d\n", Nodes[i].ProcDelay, Nodes[i].QueueDelay, Nodes[i].Jitter,
         Nodes[i].IsOptical);
      printf("%d ", Nodes[i].nConnLinks);
      for(j = 0; j < Nodes[i].nConnLinks; j++) printf("%d ", Nodes[i].ConnLinks[j]);
      printf("\n");
   }
   printf("####################\n");
   j = 0;
   for(i = 0; i < nOpticalNodes-1+nElecNodes1; i++)
   {
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Head, Links[i].Tail);
      /*
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Tail, Links[i].Head);
      */
  }
  for(i = nOpticalNodes-1+nElecNodes; i < nOpticalLinks+nElecNodes2; i++)
  {
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Head, Links[i].Tail);
      /*
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Tail, Links[i].Head);
      */
  }
  for(i = nOpticalNodes-1+nElecNodes1; i < nOpticalNodes-1+nElecNodes; i++)
  {
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Head, Links[i].Tail);
      /*
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Tail, Links[i].Head);
      */

  }
  for(i = nOpticalLinks+nElecNodes2; i < nOpticalLinks+nElecNodes; i++)
  {
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Head, Links[i].Tail);
      /*
      printf("# link %d\n", j); j++;
      printf("%d %lf %lf %d\n", Links[i].Capacity, Links[i].PropDelay, Links[i].BandCost,
         Links[i].IsOptical);
      printf("%d %d\n", Links[i].Tail, Links[i].Head);
      */

  }
  printf("####################\n");
  for(i = 0; i < nFlows; i++)
  {
     printf("# flow %d\n", i);
     printf("%d %d %lf %d %lf %lf\n", Flows[i].Source, Flows[i].Destination, Flows[i].Priority,
        Flows[i].BandWidth, Flows[i].DelayPrice, Flows[i].JitterPrice);
  }

  return 0;
}

void createOpticalNode(OAARNode* Node)
{
   Node->ProcDelay = 0;
   Node->QueueDelay = 0;
   Node->Jitter = 0;
   Node->IsOptical = 1;
   Node->nConnLinks = 0;
   Node->ConnLinks = (int*)malloc(20*sizeof(int));
}

void createElecNode(OAARNode* Node)
{
   Node->ProcDelay = 1;
   Node->QueueDelay = 1;
   Node->Jitter = 1;
   Node->IsOptical = 0;
   Node->nConnLinks = 0;
   Node->ConnLinks = (int*)malloc(20*sizeof(int));
}

void createOpticalLink(OAARLink* Link, int Head, int Tail, int i)
{
   Link->Capacity = 1000;
   Link->PropDelay = 0;
   Link->BandCost = 1;
   Link->IsOptical = 1;
   Link->Head = Head;
   Link->Tail = Tail;

   Nodes[Head].ConnLinks[Nodes[Head].nConnLinks] = i;
   Nodes[Head].nConnLinks++;
}

void createElecLink(OAARLink* Link, int Head, int Tail, int i)
{
   Link->Capacity = 1000;
   Link->PropDelay = 0.5;
   Link->BandCost = 2;
   Link->IsOptical = 0;
   Link->Head = Head;
   Link->Tail = Tail;

   Nodes[Head].ConnLinks[Nodes[Head].nConnLinks] = i;
   Nodes[Head].nConnLinks++;
}

void createFlow(OAARFlow* Flow)
{
   Flow->Source = intRandom(0, nNodes-1);
   Flow->Destination = intRandom(0, nNodes-1);
   Flow->Priority = 1.0;
   Flow->BandWidth = 400;
   Flow->DelayPrice = 200;
   Flow->JitterPrice = 500;
}

int intRandom(int low, int high)
{
   srand((int)time(0));
   return low+(int)( (double)(high-low) * (double)rand()/(double)RAND_MAX );
}
