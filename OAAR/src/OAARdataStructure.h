/***
 * @file   OAARdataStucture.h
 * @brief  Claiming OAARNode, OAARLink, OAARTopology, OAARFlow
 * @author He Xingqiu
 *
***/

typedef struct {
   int NodeIndex;
   double ProcDelay;
   double QueueDelay;
   double Jitter;
   int IsOptical;
   int nConnLinks;
   int* ConnLinks;
} OAARNode

typedef struct  {
   int LinkIndex;
   double Capacity;
   double PropDelay;
   double BandCost;
   int IsOptical;
   int Head;
   int Tail;
} OAARLink

typedef struct  {
   int nNodes;
   int nOpticalNodes;
   int nLinks;
   int nOpticalLinks;
   OAARNodes** Nodes;
   OAARLinks** Links;
} OAARTopology

typedef struct {
   int Source;
   int Destination;
   double Priority;
   double BandWidth;
   double DelayPrice;
   double JitterPrice;
} OAARFlow
