/**@file   cmain.c
 * @brief  Main file for OAAR
 * @author He Xingqiu
 *
 *  This the file contains the \ref main() main function of the projects. This includes all the default plugins of
 *  \SCIP and others implemented for this project. 
 */
#include <stdio.h>

#include "scip/scip.h"
#include "scip/scipshell.h"
#include "scip/scipdefplugins.h"

#include "branch_originalvar.h" 
#include "cons_zeroone.h" 
#include "pricer_OAAR.h"
#include "reader_OAAR.h"

/** creates a SCIP instance with default plugins, evaluates command line parameters, runs SCIP appropriately,
 *  and frees the SCIP instance
 */
static
SCIP_RETCODE runShell(
   int                        argc,               /**< number of shell parameters */
   char**                     argv,               /**< array with shell parameters */
   const char*                defaultsetname      /**< name of default settings file */
   )
{
   SCIP* scip = NULL;

   /*********
    * Setup *
    *********/

   /* initialize SCIP */
   SCIP_CALL( SCIPcreate(&scip) );
   
   /* include OAAR reader */
   SCIP_CALL( SCIPincludeReaderOAAR(scip) );

   /* include binpacking branching and branching data */
   SCIP_CALL( SCIPincludeBranchruleOriginalvar(scip) );
   SCIP_CALL( SCIPincludeConshdlrZeroone(scip) );
   
  /* include OAAR pricer  */
   SCIP_CALL( SCIPincludePricerOAAR(scip) );

   /* include default SCIP plugins */
   SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
 
   /* for column generation instances, disable restarts */
   SCIP_CALL( SCIPsetIntParam(scip,"presolving/maxrestarts",0) );

   /* turn off all separation algorithms */
   SCIP_CALL( SCIPsetSeparating(scip, SCIP_PARAMSETTING_OFF, TRUE) );
   
   /**********************************
    * Process command line arguments *
    **********************************/
   SCIP_CALL( SCIPprocessShellArguments(scip, argc, argv, defaultsetname) );

   /********************
    * Deinitialization *
    ********************/

   SCIP_CALL( SCIPfree(&scip) );

   BMScheckEmptyMemory();

   return SCIP_OKAY;
}

int
main(
   int                        argc,
   char**                     argv
   )
{
   SCIP_RETCODE retcode;

   retcode = runShell(argc, argv, "scip.set");
   if( retcode != SCIP_OKAY )
   {
      SCIPprintError(retcode);
      return -1;
   }

   return 0;
}

