/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/
/*
 * job_attr_def is the array of pbs_attribute definitions for jobs.
 * Each legal job pbs_attribute is defined here.
 */

#include <stdlib.h>
#include <string.h>
#include <pbs_config.h>		/* the master config generated by configure */

#include "pbs_ifl.h"
#include "pbs_error.h"
#include "list_link.h"
#include "attribute.h"
#include "server_limits.h"

/* Extern functions (at_action) called */

extern int job_set_wait (pbs_attribute *, void *, int);
extern int action_resc (pbs_attribute *, void *, int);
extern int ck_checkpoint (pbs_attribute *, void *, int);
extern int depend_on_que (pbs_attribute *, void *, int);
extern int comp_checkpoint (pbs_attribute *, pbs_attribute *);

#define ATR_DFLAG_SSET  (ATR_DFLAG_SvWR | ATR_DFLAG_SvRD)

int encode_exec_host(

  pbs_attribute  *attr,    /* ptr to pbs_attribute */
  tlist_head     *phead,   /* head of attrlist */
  const char    *atname,  /* name of pbs_attribute */
  const char    *rsname,  /* resource name or null */
  int             mode,    /* encode mode, unused here */
  int             perm)    /* only used for resources */

  {
  char *pipe;
  int   rc;
  pbs_attribute  tmp;

  if (attr->at_val.at_str == NULL)
    return(PBSE_NONE);

  memcpy(&tmp,attr,sizeof(pbs_attribute));
  if ((tmp.at_val.at_str = strdup(attr->at_val.at_str)) == NULL)
    return(PBSE_SYSTEM);

  while ((pipe = strchr(tmp.at_val.at_str, '|')) != NULL)
    *pipe = '+';

  rc = encode_str(&tmp, phead, atname, rsname, mode, perm);
  free(tmp.at_val.at_str);

  return(rc);
  } /* END encode_exec_host() */




/*
 * The entries for each pbs_attribute are (see attribute.h):
 * name,
 * decode function,
 * encode function,
 * set function,
 * compare function,
 * free value space function,
 * action function,
 * access permission flags,
 * value type,
 * parent object type
 */

/* sync w/enum job_atr in src/include/pbs_job.h */
/* sync w/ TJobAttr[] in src/resmom/requests.c */

attribute_def job_attr_def[] =
  {

  /* JOB_ATR_jobname */
    { (char *)ATTR_N,   /* "Job_Name" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
    },
  /* JOB_ATR_job_owner */
  { (char *)ATTR_owner,  /* "Job_Owner" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_resc_used */
  { (char *)ATTR_used,  /* "Resources_Used" */
    decode_resc,
    encode_resc,
    set_resc,
    comp_resc,
    free_resc,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvWR,
    ATR_TYPE_RESC,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_state */
  { (char *)ATTR_state,  /* "job_state" */
    decode_c,
    encode_c,
    set_c,
    comp_c,
    free_null,
    NULL_FUNC,
    READ_ONLY,
    ATR_TYPE_CHAR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_in_queue */
  { (char *)ATTR_queue,  /* "Queue" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM /* | ATR_DFLAG_ALTRUN */,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_at_server */
  { (char *)ATTR_server,  /* "Server" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_account */
  { (char *)ATTR_A,   /* "Account_Name" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_checkpoint */
  { (char *)ATTR_c,   /* "Checkpoint" */
    decode_str,
    encode_str,
    set_str,
#ifdef PBS_MOM
    comp_str,
#else /* PBS_MOM - server side */
    comp_checkpoint,
#endif /* PBS_MOM */
    free_str,
#ifdef PBS_MOM
    NULL_FUNC,
#else /* PBS_MOM - server side */
#if 0
    ck_checkpoint,
#else
    0,
#endif
#endif /* PBS_MOM */
    READ_WRITE | ATR_DFLAG_MOM | ATR_DFLAG_ALTRUN,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_ctime */    /* create time, set when the job is queued */
  { (char *)ATTR_ctime,  /* "ctime" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_depend */
  { (char *)ATTR_depend,   /* "depend" */
#ifndef PBS_MOM
    decode_depend,
    encode_depend,
    set_depend,
    comp_depend,
    free_depend,
    depend_on_que,
#else
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
#endif /* PBS_MOM */
    READ_WRITE,
    ATR_TYPE_LIST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_errpath */
  { (char *)ATTR_e,   /* "Error_Path" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_exec_host */
  { (char *)ATTR_exechost,  /* "exec_host" */
    decode_str,
    encode_exec_host,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,  /* allow operator/scheduler to modify exec_host */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_exec_port */
  { (char *)ATTR_execport,  /* "exec_port" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,  /* allow operator/scheduler to modify exec_port */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_exec_gpus */
  { (char *)ATTR_exec_gpus,  /* exec_gpus */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,  /* allow operator/scheduler to modify exec_host */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB,
  },

  /* JOB_ATR_exectime */
  { (char *)ATTR_a,   /* "Execution_Time" (aka release_date) */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
#ifndef PBS_MOM
    job_set_wait,
#else
    NULL_FUNC,
#endif
    READ_WRITE | ATR_DFLAG_ALTRUN,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_grouplst */
  { (char *)ATTR_g,   /* "group_list" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_hold */
  { (char *)ATTR_h,   /* "Hold_Types" */
    decode_hold,
    encode_hold,
    set_hold,
    comp_hold,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_interactive */
  { (char *)ATTR_inter,  /* "interactive" */
    decode_l,
    encode_inter,
    set_l,
    comp_b,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvRD | ATR_DFLAG_Creat | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_join */
  { (char *)ATTR_j,   /* "Join_Path" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_keep */
  { (char *)ATTR_k,   /* "Keep_Files" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_mailpnts */
  { (char *)ATTR_m,   /* "Mail_Points" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_mailuser */
  { (char *)ATTR_M,   /* "Mail_Users" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_mtime */
  { (char *)ATTR_mtime,  /* "mtime" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_outpath */
  { (char *)ATTR_o,   /* "Output_Path" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_priority */
  { (char *)ATTR_p,   /* "Priority" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_qtime */
  { (char *)ATTR_qtime,  /* "qtime"  (time entered queue) */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_rerunable */
  { (char *)ATTR_r,   /* "Rerunable" */
    decode_b,
    encode_b,
    set_b,
    comp_b,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ,
    ATR_TYPE_BOOL,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_resource */
  { (char *)ATTR_l,   /* "Resource_List" */
    decode_resc,
    encode_resc,
    set_resc,
    comp_resc,
    free_resc,
    action_resc,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_MOM,
    ATR_TYPE_RESC,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_session_id */
  { (char *)ATTR_session,  /* "session_id" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvWR,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_shell */
  { (char *)ATTR_S,   /* "Shell_Path_List" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_stagein */
  { (char *)ATTR_stagein,  /* "stagein" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_stageout */
  { (char *)ATTR_stageout,  /* "stageout" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_substate */
  { (char *)ATTR_substate,  /* "substate" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    ATR_DFLAG_OPRD | ATR_DFLAG_MGRD,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_userlst */
  { (char *)ATTR_u,   /* "User_List" */
    decode_arst,
    encode_arst,
#ifndef PBS_MOM
    set_uacl,
#else
    set_arst,
#endif
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_variables (allow to be dynamically modifiable) */
  { (char *)ATTR_v,   /* "Variable_List" */
    decode_arst,
    encode_arst,
    set_arst,
    comp_arst,
    free_arst,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM | ATR_DFLAG_PRIVR | ATR_DFLAG_ALTRUN,
    ATR_TYPE_ARST,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_euser    */
  { (char *)ATTR_euser,  /* "euser" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_egroup  */
  { (char *)ATTR_egroup,  /* "egroup" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_hashname */
  { (char *)ATTR_hashname,  /* "hashname" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_MGRD | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_hopcount */
  { (char *)ATTR_hopcount,  /* "hop_count" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_qrank */
  { (char *)ATTR_qrank,  /* "queue_rank" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    ATR_DFLAG_MGRD,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_queuetype */
  { (char *)ATTR_qtype,  /*"queue_type" - exists for Scheduler select */
    decode_c,
    encode_c,
    set_c,
    comp_c,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SELEQ,
    ATR_TYPE_CHAR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_sched_hint */
  { (char *)ATTR_sched_hint,        /* "sched_hint" - inform scheduler re sync */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_SSET | NO_USER_SET | ATR_DFLAG_ALTRUN,  /*ATR_DFLAG_MGRD | ATR_DFLAG_MGWR */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_security */
  { (char *)ATTR_security,  /* "security" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_SSET,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_Comment */
  { (char *)ATTR_comment,  /* "comment" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    NO_USER_SET | ATR_DFLAG_SvWR | ATR_DFLAG_ALTRUN,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_Cookie */
  { (char *)ATTR_cookie,  /* "cookie" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_SvRD | ATR_DFLAG_SvWR | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_altid */
  { (char *)ATTR_altid,  /* "alt_id" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvWR,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_etime */
  { (char *)ATTR_etime,  /* "etime" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_exitstat */
  { (char *)ATTR_exitstat,  /* "exit_status" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },
  /* JOB_ATR_forwardx11 */
  { (char *)ATTR_forwardx11, /* "forward_x11" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_USWR | ATR_DFLAG_MGRD | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_submit_args */
  { (char *)ATTR_submit_args,
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvRD | ATR_DFLAG_Creat,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_job_array_id */
  { (char *)ATTR_array_id,
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    ATR_DFLAG_SvRD | READ_ONLY,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_job_array_request */
  { (char *)ATTR_t,
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_Creat | ATR_DFLAG_SvRD | READ_ONLY,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_umask */
  { (char *)ATTR_umask,
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_start_time */
  { (char *)ATTR_start_time,
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET | ATR_DFLAG_MOM,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_start_count */
  { (char *)ATTR_start_count,
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_checkpoint_dir */
  { (char *)ATTR_checkpoint_dir,  /* "checkpoint_dir" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_checkpoint_name */
  { (char *)ATTR_checkpoint_name,  /* "checkpoint_name" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM | ATR_DFLAG_ALTRUN,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_checkpoint_time */
  { (char *)ATTR_checkpoint_time,  /* "checkpoint_time" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    NO_USER_SET | ATR_DFLAG_SvWR | ATR_DFLAG_ALTRUN,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_checkpoint_restart_status */
  { (char *)ATTR_checkpoint_restart_status,  /* "checkpoint_restart_status" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SvWR | ATR_DFLAG_ALTRUN,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_restart_name */
  { (char *)ATTR_restart_name,  /* "restart_name" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    NO_USER_SET | ATR_DFLAG_SvWR | ATR_DFLAG_ALTRUN | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_f (fault_tolerant)*/
  { (char *)ATTR_f,
    decode_b,
    encode_b,
    set_b,
    comp_b,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_ALTRUN | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_BOOL,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_comp_time */
  { (char *)ATTR_comp_time, /* "comp_time" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_reported */
  {	(char *)ATTR_reported,			/* "Reported" */
  	decode_b,
  	encode_b,
  	set_b,
  	comp_b,
  	free_null,
  	NULL_FUNC,
  	READ_ONLY | ATR_DFLAG_SSET,
  	ATR_TYPE_BOOL,
  	PARENT_TYPE_JOB
  },

  /* JOB_ATR_jobtype */
  { (char *)ATTR_jobtype,   /* "Job_Type" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_inter_cmd */
  { (char *)ATTR_intcmd,   /* "Interactive_Cmd" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_job_radix */
  { (char *)ATTR_job_radix,  /* "job_radix" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SSET | ATR_DFLAG_MOM,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_total_runtime */
  { (char *)ATTR_total_runtime, /* total time from start_time to comp_time in milliseconds */
    decode_tv,
    encode_tv,
    set_tv,
    comp_tv,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SSET,
    ATR_TYPE_TV,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_sister_list */
  { (char *)ATTR_sister_list, /* sister_list */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,  /* allow operator/scheduler to modify exec_host */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_proxy_user */
  { (char *)ATTR_P, /* "proxy_user" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    ATR_DFLAG_Creat | ATR_DFLAG_MGRD | ATR_DFLAG_USRD | ATR_DFLAG_OPRD,
    ATR_TYPE_STR,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_node_exclusive */
  { (char *)ATTR_node_exclusive, /* node_exclusive */
    decode_b,
    encode_b,
    set_b,
    comp_b,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_BOOL,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_submit_host */
  {(char *)ATTR_submit_host,		/* "submit_host - undocumented */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_SvRD | ATR_DFLAG_Creat,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_init_work_dir */
  {(char *)ATTR_init_work_dir,		/* "init_work_dir - undocumented */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_SvRD | ATR_DFLAG_Creat,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_pagg_id */
  { (char *)ATTR_pagg,  /* "pagg_id" */
    decode_ll,
    encode_ll,
    set_ll,
    comp_ll,
    free_null,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_SvWR,
    ATR_TYPE_LL,
    PARENT_TYPE_JOB
  },

  /* JOB_ATR_gpu_flags */
  {(char *)ATTR_gpu_flags,		/* "gpu_flags" - mode and reset flags */
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},

  /* JOB_ATR_job_id */
  {(char *)ATTR_J,      /* "job_id" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   ATR_DFLAG_Creat | ATR_DFLAG_MGRD | ATR_DFLAG_USRD | ATR_DFLAG_OPRD,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_arguments*/
  {(char *)ATTR_args,		/* "job_arguments" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE | ATR_DFLAG_SELEQ | ATR_DFLAG_MOM,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_reservation_id */
  {(char *)ATTR_reservation_id, /* "reservation_id" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE |  ATR_DFLAG_MOM,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_login_node_id */
  {(char *)ATTR_login_node_id,  /* "login_node_id" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_login_prop */
  {(char *)ATTR_login_prop, /* "login_property" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_external_nodes */
  {(char *)ATTR_external_nodes, /* external_nodes */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_multi_req_alps */
  {(char *)ATTR_multi_req_alps, /* "multi_req_alps" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE | ATR_DFLAG_MOM,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB},

  /* JOB_ATR_exec_mics */
  { (char *)ATTR_exec_mics,  /* "exec_mics" */
    decode_str,
    encode_str,
    set_str,
    comp_str,
    free_str,
    NULL_FUNC,
    READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,  /* allow operator/scheduler to modify exec_host */
    ATR_TYPE_STR,
    PARENT_TYPE_JOB,
  },

  /* JOB_ATR_system_start_time */
  {ATTR_system_start_time, /* start time as encoded in the proc/pid directory */
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},

  /* JOB_ATR_nppcu */
  {ATTR_nppcu, /* how to handle compute units (on Cray system) */
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_SvWR,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},

  /* JOB_ATR_login_node_key */
  {ATTR_login_node_key, /* "login_node_key" */
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},

  /* JOB_ATR_request_version */
  {ATTR_request_version, /* "request_version" */
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_WRITE | ATR_DFLAG_MOM,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},

  /* JOB_ATR_req_information */
  {ATTR_req_information, /* "req_information" */
   decode_complete_req,
   encode_complete_req,
   set_complete_req,
   comp_complete_req,
   free_complete_req,
   NULL_FUNC,
   READ_WRITE | ATR_DFLAG_MOM,
   ATR_TYPE_REQ,
   PARENT_TYPE_JOB},

/* Site defined attributes if any, see site_job_attr_*.h  */
#include "site_job_attr_def.h"

  /* JOB_ATR_copystd_on_rerun */
  {(char *)ATTR_copy_on_rerun,   /* "copy_on_rerun" */
    decode_b,
    encode_b,
    set_b,
    comp_b,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_BOOL,
    PARENT_TYPE_JOB},

  // JOB_ATR_cpuset_string
  {(char *)ATTR_cpustr,   /* "cpuset_string" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB,
  },

  // JOB_ATR_memset_string
  {(char *)ATTR_memstr,   /* "memset_string" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB,
  },

  /* JOB_ATR_user_kill_delay */
  {(char *)ATTR_user_kill_delay, /* "user_kill_delay" */
    decode_l,
    encode_l,
    set_l,
    comp_l,
    free_null,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_MOM,
    ATR_TYPE_LONG,
    PARENT_TYPE_JOB},

  // JOB_ATR_idle_slot_limit
  {(char *)ATTR_idle_slot_limit, // "idle_slot_limit"
   decode_l,
   encode_l,
   set_l,
   comp_l,
   free_null,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_LONG,
   PARENT_TYPE_JOB},
  
  // JOB_ATR_LRequest
  {(char *)ATTR_L_request, // "L_Request"
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_WRITE,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB,
  },
  
  // JOB_ATR_gpus_reserved
  {(char *)ATTR_gpus_reserved,   /* "gpus_reserved" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB,
  },
  
  // JOB_ATR_mics_reserved
  {(char *)ATTR_mics_reserved,   /* "mics_reserved" */
   decode_str,
   encode_str,
   set_str,
   comp_str,
   free_str,
   NULL_FUNC,
   READ_ONLY | ATR_DFLAG_MOM | ATR_DFLAG_OPWR | ATR_DFLAG_SvWR,
   ATR_TYPE_STR,
   PARENT_TYPE_JOB,
  },

  /* JOB_ATR_UNKN - THIS MUST BE THE LAST ENTRY */
  { (char *)"_other_",
    decode_unkn,
    encode_unkn,
    set_unkn,
    comp_unkn,
    free_unkn,
    NULL_FUNC,
    READ_WRITE | ATR_DFLAG_SELEQ,
    ATR_TYPE_LIST,
    PARENT_TYPE_JOB
  }
  };
