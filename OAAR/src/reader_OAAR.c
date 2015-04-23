/**@file   reader_bpa.c
 * @brief  OAAR problem reader file reader
 * @author He Xingqiu
 *
 * Read data from oaar format files and pass all the data to function SCIPprobdataCreate, which
 * initialize the master problem.
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

//#include "scip/cons_setppc.h"

#include "probdata_OAAR.h"
#include "reader_OAAR.h"
#include "OAARdataStructure.h"

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
   OAARTopology* Topology;
   OAARFlow** Flow;
   int nFLow;

   // alloc memory for Topology
   SCIP_CALL( SCIPallocBuffer(scip, &Topology) );

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
      } while ( buffer[0] == '#' )

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
      } while ( buffer[0] == '#' )

      nread = sscanf(buffer, "%d %d\n", &(Topology->nNodes), &(Topology->nOpticalNodes));
      if( nread < 2 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }

      SCIPdebugMessage("nNodes = <%d>, nOpticalNodes = <%d>\n", Topology->nNodes, Topology->nOpticalNodes);
   }


   /* allocate memory for Nodes */
   SCIP_CALL( SCIPallocBufferArray(scip, Topology->Nodes, Topology->nNodes) );

   error = FALSE;

   for( i = 0; i < Topology->nNodes; i++ )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' )

      /* parse the line */
      nread = sscanf(buffer, "%f %f %f %d\n", &(Topology->Nodes[i]->ProcDelay), 
         &(Topology->Nodes[i]->QueueDelay), &(Topology->Nodes[i]->Jitter), &(Topology->Nodes[i]->IsOptical));
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
      } while ( buffer[0] == '#' )

      // parse the line
      nread = sscanf(buffer, "%d %s\n", &(Topology->Nodes[i]->nConnLinks), buffer);
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
      SCIP_CALL( SCIPallocBufferArray(scip, &(Topology->Nodes[i]->ConnLinks), Topology->Nodes[i]->nConnLinks) );
      for( j = 0; j < Topology->Nodes[i]->nConnLinks; j++ )
      {
         nread == sscanf(buffer, "%d %s", &Topology->Nodes[i]->ConnLinks[j], buffer);
	 if( nread == 0 )
	 {
	    SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	    return SCIP_READERROR;
	 }
      }
      
      SCIPdebugMessage("Read node %d\n", i);
   }

   /* read nLinks and nOpticalLinks */
   if( !SCIPfeof(file) )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' )

      nread = sscanf(buffer, "%d %d\n", &(Topology->nLinks), &(Topology->nOpticalLinks));
      if( nread < 2 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
         return SCIP_READERROR;
      }

      SCIPdebugMessage("nLinks = <%d>, nOpticalLinks = <%d>\n", Topology->nLinks, Topology->nOpticalLinks);
   }


   /* allocate memory for Links */
   SCIP_CALL( SCIPallocBufferArray(scip, Topology->Links, Topology->nLinks) );

   error = FALSE;

   for( i = 0; i < Topology->nLinks; i++ )
   {
      /* get next line */
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
            return SCIP_READERROR;
         lineno++;
      } while ( buffer[0] == '#' )

      /* parse the line */
      nread = sscanf(buffer, "%f %f %f %d\n", &(Topology->Links[i]->Capacity), 
         &(Topology->Links[i]->PropDelay), &(Topology->Links[i]->BandCost), &(Topology->Links[i]->IsOptical));
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
      } while ( buffer[0] == '#' )

      // parse the line
      nread = sscanf(buffer, "%d %d\n", &(Topology->Links[i]->Head), &(Topology->Links[i]->Tail));
      if( nread == 0 )
      {
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
            
      SCIPdebugMessage("Read link %d\n", i);
   }

   // read nFlow
   do{ 
      if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
         return SCIP_READERROR;
      lineno++;
   } while ( buffer[0] == '#' )
   nread = sscanf(buffer, "%d\n", &nFlow)
   if( nread == 0 )
   {
      SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
      return SCIP_READERROR;
   }

   // alloc memory for Flow
   SCIP_CALL( SCIPallocBufferArray(scip, Flow, nFlow) );

   // read Flow
   for( i = 0; i < nFlow; i++ )
   {
      do{
         if( SCIPfgets(buffer, sizeof(buffer), file) == NULL )
	    return SCIP_READERROR;
	 lineno++;
      } while ( buffer[0] == '#' )

      nread = sscanf(buffer, "%d %d %f %f %f %f\n", &(Flow[i]->Source), &(Flow[i]->Destination),
         &(Flow[i]->Priority), &(Flow[i]->BandWidth), &(Flow[i]->DelayPrice), &(Flow[i]->JitterPrice));
      if( nread == 0 )
      { 
         SCIPwarningMessage(scip, "invalid input line %d in file <%s>: <%s>\n", lineno, filename, buffer);
	 return SCIP_READERROR;
      }
   }

   /* create a new problem in SCIP */
   SCIP_CALL( SCIPprobdataCreate(scip, probName, Topology, Flow, nFlow) );
  
   (void)SCIPfclose(file);
   //SCIPfreeBufferArray(scip, &ids);
   //SCIPfreeBufferArray(scip, &weights);
   SCIPfreeBufferArray(scip, Flow, nFlow);
   SCIPfreeBufferArray(scip, Topology->Links, Topology->nLinks);
   for( i = 0; i < Topology->nNodes; i++ )
   {
      SCIPfreeBufferArray(scip, &(Topology->Nodes[i]->ConnLinks), Topology->Nodes[i]->nConnLinks);
   }
   SCIPfreeBufferArray(scip, Topology->Nodes, Topology->nNodes);
   SCIPfreeBuffer(scip, &Topology);


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
