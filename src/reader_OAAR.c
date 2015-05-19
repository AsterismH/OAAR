/**@file   reader_OAAR.c
 * @brief  OAAR problem reader file reader
 * @author He Xingqiu
 *
 * Read data from oaar format files and pass all the data to function SCIPprobdataCreate, which
 * initialize the master problem.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#define SCIP_DEBUG

#include <assert.h>
#include <string.h>

//#include "scip/cons_setppc.h"

#include "probdata_OAAR.h"
#include "reader_OAAR.h"

/**@name Reader properties
 *
 * @{
 */

#define READER_NAME             "oaarreader"
#define READER_DESC             "file reader for OAAR data format"
#define READER_EXTENSION        "oaar"

/**@} */


/**@name Callback methods
 *
 * @{
 */

/** problem reading method of reader */
static
SCIP_DECL_READERREAD(readerReadOAAR)
{  /*lint --e{715}*/
   SCIP_FILE* file;
   int nread;
   int lineno;
   char format[16];
   char buffer[SCIP_MAXSTRLEN];
   int i,j;
   
   char probName[SCIP_MAXSTRLEN];
   OAARNode* Nodes;
   OAARLink* Links;
   OAARFlow*  Flows;
   int nNodes;
   int nOpticalNodes;
   int nLinks;
   int nOpticalLinks;
   int nFlows;
   int nCons;

   *result = SCIP_DIDNOTRUN;

   /* open file */
   file = SCIPfopen(filename, "r");
   if( file == NULL )
   {
      SCIPerrorMessage("cannot open file <%s> for reading\n", filename);
      SCIPprintSysError(filename);
      return SCIP_NOFILE;
   }

   lineno = 0;

   /* read problem name */
   if( !SCIPfeof(file) )
   {
      /* get next line */
      do{ 
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      sprintf(format, "%%%ds\n", SCIP_MAXSTRLEN);
      nread = sscanf(buffer, format, probName);
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }

      SCIPdebugMessage("problem name <%s>\n", probName);
   }

   /* read nNodes and nOpticalNodes */
   if( !SCIPfeof(file) )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      nread = sscanf(buffer, "%d %d %d %d %d\n", &nNodes, &nOpticalNodes, &nLinks, &nOpticalLinks, &nFlows);
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }

      SCIPdebugMessage("nNodes = <%d>, nOpticalNodes = <%d>, nLinks = <%d>, nOpticalLinks = <%d>, nFlows = <%d>\n", nNodes, nOpticalNodes, nLinks, nOpticalLinks, nFlows);
   }


   /* allocate memory for Nodes */
   SCIP_CALL( SCIPallocBufferArray(scip, &Nodes, nNodes) );

   for( i = 0; i < nNodes; i++ )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      /* parse the line */
      nread = sscanf(buffer, "%lf %lf %lf %d\n", &(Nodes[i].ProcDelay), 
         &(Nodes[i].QueueDelay), &(Nodes[i].Jitter), &(Nodes[i].IsOptical));
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }

      // get next line
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      // parse the line
      nread = sscanf(buffer, "%d %s\n", &(Nodes[i].nConnLinks), buffer);
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
      SCIP_CALL( SCIPallocBufferArray(scip, &(Nodes[i].ConnLinks), Nodes[i].nConnLinks) );
      for( j = 0; j < Nodes[i].nConnLinks; j++ )
      {
         nread == sscanf(buffer, "%d %s", &Nodes[i].ConnLinks[j], buffer);
	 if( nread == 0 )
	 {
	    SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	    return SCIP_READERROR;
	 }
      }
      
      SCIPdebugMessage("Read node %d\n", i);
   }

   /* allocate memory for Links */
   // we allocate extra nFlows for artificial links
   SCIP_CALL( SCIPallocBufferArray(scip, &Links, nLinks+nFlows) );

   for( i = 0; i < nLinks; i++ )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      /* parse the line */
      nread = sscanf(buffer, "%d %lf %lf %d\n", &(Links[i].Capacity), 
         &(Links[i].PropDelay), &(Links[i].BandCost), &(Links[i].IsOptical));
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }
      if(Links[i].IsOptical)
         Links[i].TransDelay = 0;
      else
         Links[i].TransDelay = 1500 * 8 / Links[i].Capacity;

      // get next line
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' );

      // parse the line
      nread = sscanf(buffer, "%d %d\n", &(Links[i].Head), &(Links[i].Tail));
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
            
      SCIPdebugMessage("Read link %d\n", i);
   }

   // alloc memory for Flows
   SCIP_CALL( SCIPallocBufferArray(scip, &Flows, nFlows) );

   // read Flow
   for( i = 0; i < nFlows; i++ )
   {
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
	    return SCIP_READERROR;
	 lineno++;
      } while ( buffer[0] == '#' );

      nread = sscanf(buffer, "%d %d %lf %d %lf %lf\n", &(Flows[i].Source), &(Flows[i].Destination),
         &(Flows[i].Priority), &(Flows[i].BandWidth), &(Flows[i].DelayPrice), &(Flows[i].JitterPrice));
      if( nread == 0 )
      { 
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
   }

   // set artificial links
   // here, nLinks still stands for the number of original links (excluding the artificial one)
   for( i = nLinks; i < nFlows+nLinks; i++ )
   {
       Links[i].Capacity = Flows[i-nLinks].BandWidth;
       Links[i].PropDelay = MAX_PROPDELAY;
       Links[i].BandCost = MAX_BANDCOST;
       Links[i].IsOptical = 0;
       Links[i].Head = Flows[i-nLinks].Source;
       Links[i].Tail = Flows[i-nLinks].Destination;
   }
   //SCIPdebugMessage("MAX_BANDCOST:%d\n", MAX_BANDCOST);

   // update nLinks
   // from now on, nLinks will include the number of artificial links
   nLinks += nFlows;

   // Compute nCons
   // nCons = nCons1(nFlows) + nCons2(E-E') + nCons3(E'*NLAMBDA)
   nCons = nFlows + (nLinks - nOpticalLinks) + (nOpticalLinks * nWaveLength);

   /*
   printf("USER_DEBUG: read and parse:\n");
   printNodes(Nodes, nNodes);
   printLinks(Links, nLinks);
   printFlows(Flows, nFlows);
   printf("nOpticalNodes:%d, nOpticalLinks:%d, nCons:%d\n", 
      nOpticalNodes, nOpticalLinks, nCons);
   */

   /* create a new problem in SCIP */
   SCIP_CALL( SCIPprobdataCreate(scip, probName, Nodes, Links, Flows, nNodes, nOpticalNodes, 
      nLinks, nOpticalLinks, nFlows, nCons) );
  
   (void)SCIPfclose(file);
   SCIPfreeBufferArray(scip, &Flows);
   SCIPfreeBufferArray(scip, &Links);
   for( i = 0; i < nNodes; i++ )
   {
      SCIPfreeBufferArray(scip, &(Nodes[i].ConnLinks));
   }
   SCIPfreeBufferArray(scip, &Nodes);

   *result = SCIP_SUCCESS;

   return SCIP_OKAY;
}

/**@} */


/**@name Interface methods
 *
 * @{
 */

/** includes the bpa file reader in SCIP */
SCIP_RETCODE SCIPincludeReaderOAAR(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_READERDATA* readerdata;
   SCIP_READER* reader;

   /* create binpacking reader data */
   readerdata = NULL;

   /* include binpacking reader */
   SCIP_CALL( SCIPincludeReaderBasic(scip, &reader, READER_NAME, READER_DESC, READER_EXTENSION, readerdata) );
   assert(reader != NULL);

   SCIP_CALL( SCIPsetReaderRead(scip, reader, readerReadOAAR) );

   return SCIP_OKAY;
}

/**@} */
