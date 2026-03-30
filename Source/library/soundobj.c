/*
 * soundobj.c  V2.1
 *
 * TMObject, Type: Sound
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerLib.h"

/* Internal only: optional datatypes sound load/play (dtsound.c); not part of public API. */
struct TMSoundData;
struct TMSoundData *ReadSoundViaDataTypes(char *name);
void FreeSoundData(struct TMSoundData *tsd);
void PlaySoundData(struct TMSoundData *tsd);

/* extended TMObject structure for TMOBJTYPE_SOUND objects */
struct TMObjectSound {
                      struct TMObject  so_Object;
                      char            *so_Command;
                      char            *so_Port;
                      char            *so_ARexxCmd;
                      ULONG            so_CmdLen;
                     };

/* Create a Sound object */
struct TMObject *CreateTMObjectSound(struct TMHandle *handle, char *name,
                                     struct TagItem *tags)
{
 struct TMObjectSound *tmobj;

 /* allocate memory for object */
 if (tmobj=(struct TMObjectSound *)
            AllocateTMObject(sizeof(struct TMObjectSound))) {
  struct TagItem *ti,*tstate;

  /* Set object defaults */
  tmobj->so_Port=DefaultPortName;
  tmobj->so_Command=DefaultNoName;

  /* Scan tag list */
  tstate=tags;
  while (ti=NextTagItem(&tstate)) {

   DEBUG_PRINTF("Got Tag (0x%08lx)\n",ti->ti_Tag);

   switch (ti->ti_Tag) {
    case TMOP_Command: {
                        char *s=(char *) ti->ti_Data;
                        if (s) tmobj->so_Command=s;
                       }
                       break;
    case TMOP_Port:    {
                        char *s=(char *) ti->ti_Data;
                        if (s) tmobj->so_Port=s;
                       }
                       break;
   }
  }

  /* Calculate command line length */
  tmobj->so_CmdLen=15+strlen(tmobj->so_Port)+strlen(tmobj->so_Command);

  /* Get memory for command line */
  if (tmobj->so_ARexxCmd=AllocMem(tmobj->so_CmdLen+1,MEMF_PUBLIC)) {
   char *cp=tmobj->so_ARexxCmd;

   /* Build ARexx command */
   strcpy(cp,"'address \"");
   strcat(cp,tmobj->so_Port);
   strcat(cp,"\" \"");
   strcat(cp,tmobj->so_Command);
   strcat(cp,"\"'");

   /* All OK */
   return((struct TMObject *)tmobj);
  }
  FreeMem(tmobj,sizeof(struct TMObjectSound));
 }

 /* call failed */
 return(NULL);
}

/* Delete a Sound object */
BOOL DeleteTMObjectSound(struct TMObjectSound *tmobj)
{
 DEBUG_PRINTF("Delete/Sound (0x%08lx)\n",tmobj);

 /* Remove links */
 DeleteAllLinksTMObject((struct TMObject *) tmobj);

 /* Remove object from list */
 Remove((struct Node *) tmobj);

 /* Free resources */
 FreeMem(tmobj->so_ARexxCmd,tmobj->so_CmdLen+1);

 /* Free object */
 FreeMem(tmobj,sizeof(struct TMObjectSound));

 /* All OK. */
 return(TRUE);
}

/* Change a Sound object */
BOOL ChangeTMObjectSound(struct TMHandle *handle,
                         struct TMObjectSound *tmobj,
                         struct TagItem *tags)
{
 struct TagItem *ti,*tstate;
 char *oldcmd=tmobj->so_Command;
 char *oldport=tmobj->so_Port;
 char *oldline;
 ULONG oldlen;

 /* Scan tag list */
 tstate=tags;
 while (ti=NextTagItem(&tstate)) {

  DEBUG_PRINTF("Got Tag (0x%08lx)\n",ti->ti_Tag);

  switch (ti->ti_Tag) {
   case TMOP_Command: {
                       char *s=(char *) ti->ti_Data;
                       if (s) tmobj->so_Command=s;
                      }
                      break;
   case TMOP_Port:    {
                       char *s=(char *) ti->ti_Data;
                       if (s) tmobj->so_Port=s;
                      }
                      break;
  }
 }

 /* Get old command line parameters */
 oldline=tmobj->so_ARexxCmd;
 oldlen=tmobj->so_CmdLen;

 /* Calculate command line length */
 tmobj->so_CmdLen=15+strlen(tmobj->so_Port)+strlen(tmobj->so_Command);

 /* Calculate command line length */
 if (tmobj->so_ARexxCmd=AllocMem(tmobj->so_CmdLen+1,MEMF_PUBLIC)) {
  char *cp=tmobj->so_ARexxCmd;

  /* Free old command line */
  FreeMem(oldline,oldlen+1);

  /* Build ARexx command */
  strcpy(cp,"'address \"");
  strcat(cp,tmobj->so_Port);
  strcat(cp,"\" \"");
  strcat(cp,tmobj->so_Command);
  strcat(cp,"\"'");

  /* All OK */
  return(TRUE);
 }

 /* call failed */
 tmobj->so_Command=oldcmd;
 tmobj->so_Port=oldport;
 tmobj->so_ARexxCmd=oldline;
 tmobj->so_CmdLen=oldlen;
 return(FALSE);
}

/* Allocate & Initialize a TMLink structure */
struct TMLink *AllocLinkTMObjectSound(struct TMObjectSound *tmobj)
{
 struct TMLink *tml;

 /* Allocate memory for link structure */
 if (tml=AllocMem(sizeof(struct TMLink),MEMF_CLEAR|MEMF_PUBLIC))
  /* Initialize link structure */
  tml->tml_Size=sizeof(struct TMLink);

 return(tml);
}

/* Activate a Sound object */
void ActivateTMObjectSound(struct TMLink *tml, void *args)
{
 struct TMObjectSound *tmobj=(struct TMObjectSound *) tml->tml_Linked;
 struct TMSoundData *snd;

 DEBUG_PUTSTR("Activate/Sound\n");

 /* Try to load and play via datatypes (any format -> 8SVX in memory); else ARexx. */
 snd = ReadSoundViaDataTypes(tmobj->so_Command);
 if (snd) {
  PlaySoundData(snd);
  FreeSoundData(snd);
 } else {
  SendARexxCommand(tmobj->so_ARexxCmd,tmobj->so_CmdLen);
 }
}
