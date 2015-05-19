//#ifdef USER_DEBUG

#include "OAARdataStructure.h"

void printNode(OAARNode Node)
{
   int i;
   printf("ProcDleay:%f, QueueDelay:%f, Jitter:%f, IsOptical:%d, nConnLinks:%d\n",
      Node.ProcDelay, Node.QueueDelay, Node.Jitter, Node.IsOptical, Node.nConnLinks);
   printf("ConnLinks:");
   for(i = 0; i < Node.nConnLinks; i++)
   {
      printf("%d ", Node.ConnLinks[i]);
   }
   printf("\n");
}
void printLink(OAARLink Link)
{
   printf("Capacity:%d, PropDelay:%f, TransDelay:%f, BandCost:%f, IsOptical:%d, Head:%d, Tail:%d\n", 
      Link.Capacity, Link.PropDelay, Link.TransDelay,
      Link.BandCost, Link.IsOptical, Link.Head, Link.Tail);
}
void printFlow(OAARFlow Flow)
{
   printf("Source:%d, Destination:%d, Priority:%f, BandWidth:%d, DelayPrice:%f, JitterPrice:%f\n", 
      Flow.Source, Flow.Destination, Flow.Priority, Flow.BandWidth,
      Flow.DelayPrice, Flow.JitterPrice);
}
void printNodes(OAARNode* Nodes, int nNodes)
{
   int i;
   printf("Print Nodes:\n");
   for(i = 0; i < nNodes; i++) printNode(Nodes[i]);
}
void printLinks(OAARLink* Links, int nLinks)
{
   int i;
   printf("Print Links:\n");
   for(i = 0; i < nLinks; i++) printLink(Links[i]);
}
void printFlows(OAARFlow* Flows, int nFlows)
{
   int i;
   printf("Print Flows:\n");
   for(i = 0; i < nFlows; i++) printFlow(Flows[i]);
}
//#endif

