/**@file   cons_zeroone.h
 * @brief  Constraint handler stores the local branching decision data
 * @author He Xingqiu
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_CONS_SAMEDIFF_H__
#define __SCIP_CONS_SAMEDIFF_H__


#include "scip/scip.h"

enum ConsType
{
   ZERO = 0,                               
   ONE  = 1                               
};
typedef enum ConsType CONSTYPE;

extern
SCIP_RETCODE SCIPincludeConshdlrZeroone(
   SCIP*                 scip                /**< SCIP data structure */
   );

extern
SCIP_RETCODE SCIPcreateConsZeroone(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_CONS**           cons,               /**< pointer to hold the created constraint */
   const char*           name,               /**< name of constraint */
   int                   index1,             /**< item id one */
   int                   index2,             /**< item id two */
   CONSTYPE              type,               /**< stores type of the cons */
   SCIP_NODE*            node,               /**< the node in the B&B-tree at which the cons is sticking */
   SCIP_Bool             local               /**< is constraint only valid locally? */
   );

extern
int SCIPgetIndex1Zeroone(
   SCIP*                 scip,               
   SCIP_CONS*            cons               
   );

extern
int SCIPgetIndex2Zeroone(
   SCIP*                 scip,               
   SCIP_CONS*            cons                
   );

extern
CONSTYPE SCIPgetTypeZeroone(
   SCIP*                 scip,               
   SCIP_CONS*            cons                
   );

#endif
