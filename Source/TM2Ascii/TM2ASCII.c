#include "struct.c"

#ifdef _DEBUG
#define MWDEBUG 1
#include "memwatch.h"

__stdargs void kputs(char *);
__stdargs char kgetc(void);
__stdargs void kprintf(char *,...);

#define DEBUG_PUTSTR(a) kputs(a);
#define DEBUG_GETCHR    kgetc();
#define DEBUG_PRINTF(a,b)  kprintf(a,b);
#else
#define DEBUG_PUTSTR(a)
#define DEBUG_GETCHR
#define DEBUG_PRINTF(a,b)
#endif

#define BUFSIZE 65536

static char Version[] = "$VER: TM2Ascii 1.0   "__DATE__" "__TIME__;


struct List ObjectLists[TMOBJTYPES];

ULONG stopchunks[]={ID_PREF,ID_TMEX,
                    ID_PREF,ID_TMIM,
                    ID_PREF,ID_TMSO,
                    ID_PREF,ID_TMMO,
                    ID_PREF,ID_TMIC,
                    ID_PREF,ID_TMDO,
                    ID_PREF,ID_TMAC};
struct PrefHeader PrefHdrChunk={TMPREFSVERSION,0,0};

/* Exec node */
struct ExecNode {
                 struct Node  en_Node;
                 ULONG        en_Flags;
                 UWORD        en_ExecType;
                 WORD         en_Priority;
                 LONG         en_Delay;
                 ULONG        en_Stack;
                 char        *en_Command;
                 char        *en_CurrentDir;
                 char        *en_HotKey;
                 char        *en_Output;
                 char        *en_Path;
                 char        *en_PubScreen;
                };

/* Image node */
struct ImageNode {
                  struct Node  in_Node;
                  char        *in_File;
                 };

/* Sound node */
struct SoundNode {
                  struct Node  sn_Node;
                  char        *sn_Command;
                  char        *sn_Port;
                 };

/* Menu node */
struct MenuNode {
                  struct Node  mn_Node;
                  char        *mn_Exec;
                  char        *mn_Sound;
                  char        *mn_Title;
                  char        *mn_CommandKey;
                  ULONG       mn_Flags;
                  LONG        mn_ParentIndex;
                 };

/* Icon node */
struct IconNode {
                 struct Node  in_Node;
                 ULONG        in_Flags;
                 char        *in_Exec;
                 char        *in_Image;
                 char        *in_Sound;
                 LONG         in_XPos;
                 LONG         in_YPos;
                };

/* Dock node */
struct DockNode {
                 struct Node      dn_Node;
                 ULONG            dn_Flags;
                 char            *dn_HotKey;
                 char            *dn_PubScreen;
                 char            *dn_Title;
                 struct TextAttr  dn_Font;
                 char            *dn_FontDesc;
                 LONG             dn_XPos;
                 LONG             dn_YPos;
                 ULONG            dn_Columns;
                 struct List     *dn_ToolsList;
                };

/* Access node */
struct AccessNode {
                   struct Node an_Node;
                   struct List an_Entries;
                  };

/* Tool node */
struct ToolNode {
                 struct Node  tn_Node;
                 char        *tn_Image;
                 char        *tn_Sound;
                };

struct Node *GetHead(struct List *List)
{
 if (List->lh_Head->ln_Succ)
  return List->lh_Head;
 else
  return NULL;
}

struct Node *GetSucc(struct Node *Node)
{
 if (Node->ln_Succ->ln_Succ)
  return Node->ln_Succ;
 else
  return NULL;
}

char *GetConfigStr(UBYTE **buf)
{
 char *s=*buf;
 char *new;
 ULONG len=strlen(s)+1;

 /* Allocate string buffer */
 if (new=malloc(len)) {
  /* Copy string */
  strcpy(new,s);

  /* Correct pointer */
  *buf+=len;
 }
 return(new);
}

/* Write one config string and correct pointer */
BOOL PutConfigStr(char *s, UBYTE **buf)
{
 /* string valid? */
 if (s) {
  /* Copy string to buffer */
  strcpy(*buf,s);

  /* Correct pointer */
  *buf+=strlen(s)+1;
  return(TRUE);
 }
 return(FALSE);
}

/* Free exec node */
void FreeExecNode(struct Node *node)
{
 struct ExecNode *en=(struct ExecNode *) node;
 char *s;

 if (s=en->en_Node.ln_Name) free(s);
 if (s=en->en_Command) free(s);
 if (s=en->en_CurrentDir) free(s);
 if (s=en->en_Output) free(s);
 if (s=en->en_Path) free(s);
 if (s=en->en_HotKey) free(s);
 if (s=en->en_PubScreen) free(s);

 /* Free node */
 FreeMem(en,sizeof(struct ExecNode));
}

/* Free image node */
void FreeImageNode(struct Node *node)
{
 struct ImageNode *in=(struct ImageNode *) node;
 char *s;

 if (s=in->in_Node.ln_Name) free(s);
 if (s=in->in_File) free(s);

 /* Free node */
 FreeMem(in,sizeof(struct ImageNode));
}

/* Free sound node */
void FreeSoundNode(struct Node *node)
{
 struct SoundNode *sn=(struct SoundNode *) node;
 char *s;

 if (s=sn->sn_Node.ln_Name) free(s);
 if (s=sn->sn_Command) free(s);
 if (s=sn->sn_Port) free(s);

 /* Free node */
 FreeMem(sn,sizeof(struct SoundNode));
}

/* Free menu node */
void FreeMenuNode(struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 char *s;

 if (s=mn->mn_Node.ln_Name) free(s);
 if (s=mn->mn_Exec) free(s);
 if (s=mn->mn_Sound) free(s);
 if (s=mn->mn_Title) free(s);
 if (s=mn->mn_CommandKey) free(s);
 FreeMem(mn,sizeof(struct MenuNode));
}

/* Free icon node */
void FreeIconNode(struct Node *node)
{
 struct IconNode *in=(struct IconNode *) node;
 char *s;

 if (s=in->in_Node.ln_Name) free(s);
 if (s=in->in_Exec) free(s);
 if (s=in->in_Image) free(s);
 if (s=in->in_Sound) free(s);

 /* Free node */
 FreeMem(in,sizeof(struct IconNode));
}

/* Free tool list */
void FreeToolsList(struct List *toollist)
{
 struct ToolNode *tn1,*tn2=(struct ToolNode *)GetHead(toollist);

 /* Free tool nodes */
 while (tn1=tn2) {
  char *s;

  /* Get next node */
  tn2=(struct ToolNode *)GetSucc(tn1);

  /* Remove node */
  Remove((struct Node *) tn1);

  /* Free node */
  if (s=tn1->tn_Node.ln_Name) free(s);
  if (s=tn1->tn_Image) free(s);
  if (s=tn1->tn_Sound) free(s);
  FreeMem(tn1,sizeof(struct ToolNode));
 }
 free(toollist);
}

/* Free dock node */
void FreeDockNode(struct Node *node)
{
 struct DockNode *dn=(struct DockNode *) node;
 char *s;

 if (s=dn->dn_Node.ln_Name) free(s);
 if (s=dn->dn_HotKey) free(s);
 if (s=dn->dn_PubScreen) free(s);
 if (s=dn->dn_Title) free(s);
 if (s=dn->dn_Font.ta_Name) free(s);
 if (s=dn->dn_FontDesc) free(s);

 /* Free tool list */
 if (dn->dn_ToolsList) FreeToolsList(dn->dn_ToolsList);

 /* Free node */
 FreeMem(dn,sizeof(struct DockNode));
}

/* Free Access node */
void FreeAccessNode(struct Node *node)
{
 struct AccessNode *an=(struct AccessNode *) node;
 char *s;

 if (s=an->an_Node.ln_Name) free(s);

 /* Free entries */
 {
  struct Node *aen1,*aen2=GetHead(&an->an_Entries);

  /* Scan list */
  while (aen1=aen2) {
   /* Get next node */
   aen2=GetSucc(aen1);

   /* Remove node */
   Remove(aen1);

   /* Free node */
   if (s=aen1->ln_Name) free(s);
   FreeMem(aen1,sizeof(struct Node));
  }
 }
 /* Free Node */
 FreeMem(an,sizeof(struct AccessNode));
}

/* Read TMEX IFF chunk into Exec node */
struct Node *ReadExecNode(UBYTE *buf, ULONG size)
{
 struct ExecNode *en;
 (void)size;

 /* Allocate memory for node */
 if (en=AllocMem(sizeof(struct ExecNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct ExecPrefsObject *epo=(struct ExecPrefsObject *) buf;
  ULONG sbits=epo->epo_StringBits;
  UBYTE *ptr=(UBYTE *) &epo[1];

  if ((!(sbits & EXPO_NAME) || (en->en_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_COMMAND) || (en->en_Command=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_CURDIR) || (en->en_CurrentDir=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_HOTKEY) || (en->en_HotKey=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_OUTPUT) || (en->en_Output=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_PATH) || (en->en_Path=GetConfigStr(&ptr))) &&
      (!(sbits & EXPO_PSCREEN) || (en->en_PubScreen=GetConfigStr(&ptr)))) {
   /* Copy flags & values */
   en->en_Flags=epo->epo_Flags;
   en->en_ExecType=epo->epo_ExecType;
   en->en_Priority=epo->epo_Priority;
   en->en_Delay=epo->epo_Delay;
   en->en_Stack=epo->epo_Stack;

   /* All OK. */
   return(en);
  }

  /* Call failed */
  FreeExecNode((struct Node *) en);
 }
 return(NULL);
}

/* Write Exec node to TMEX IFF chunk */
BOOL WriteExecNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct ExecNode *en=(struct ExecNode *) node;
 struct ExecPrefsObject *epo=(struct ExecPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &epo[1];

 /* Copy strings */
 if (PutConfigStr(en->en_Node.ln_Name,&ptr)) sbits|=EXPO_NAME;
 if (PutConfigStr(en->en_Command,&ptr)) sbits|=EXPO_COMMAND;
 if (PutConfigStr(en->en_CurrentDir,&ptr)) sbits|=EXPO_CURDIR;
 if (PutConfigStr(en->en_HotKey,&ptr)) sbits|=EXPO_HOTKEY;
 if (PutConfigStr(en->en_Output,&ptr)) sbits|=EXPO_OUTPUT;
 if (PutConfigStr(en->en_Path,&ptr)) sbits|=EXPO_PATH;
 if (PutConfigStr(en->en_PubScreen,&ptr)) sbits|=EXPO_PSCREEN;

 /* set string bits */
 epo->epo_StringBits=sbits;

 /* Copy flags & values */
 epo->epo_Flags=en->en_Flags;
 epo->epo_ExecType=en->en_ExecType;
 epo->epo_Priority=en->en_Priority;
 epo->epo_Delay=en->en_Delay;
 epo->epo_Stack=en->en_Stack;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMEX,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Read TMIM IFF chunk into Image node */
struct Node *ReadImageNode(UBYTE *buf, ULONG size)
{
 struct ImageNode *in;
 (void)size;

 /* Allocate memory for node */
 if (in=AllocMem(sizeof(struct ImageNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct ImagePrefsObject *ipo=(struct ImagePrefsObject *) buf;
  ULONG sbits=ipo->ipo_StringBits;
  UBYTE *ptr=(UBYTE *) &ipo[1];

  if ((!(sbits & IMPO_NAME) || (in->in_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & IMPO_FILE) || (in->in_File=GetConfigStr(&ptr))))
   /* All OK. */
   return(in);

  /* Call failed */
  FreeImageNode((struct Node *) in);
 }
 return(NULL);
}

/* Write Image node to TMIM IFF chunk */
BOOL WriteImageNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct ImageNode *in=(struct ImageNode *) node;
 struct ImagePrefsObject *ipo=(struct ImagePrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &ipo[1];

 /* Copy strings */
 if (PutConfigStr(in->in_Node.ln_Name,&ptr)) sbits|=IMPO_NAME;
 if (PutConfigStr(in->in_File,&ptr)) sbits|=IMPO_FILE;

 /* set string bits */
 ipo->ipo_StringBits=sbits;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMIM,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Read TMSO IFF chunk into Sound node */
struct Node *ReadSoundNode(UBYTE *buf, ULONG size)
{
 struct SoundNode *sn;
 (void)size;

 /* Allocate memory for node */
 if (sn=AllocMem(sizeof(struct SoundNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct SoundPrefsObject *spo=(struct SoundPrefsObject *) buf;
  ULONG sbits=spo->spo_StringBits;
  UBYTE *ptr=(UBYTE *) &spo[1];

  if ((!(sbits & SOPO_NAME) || (sn->sn_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & SOPO_COMMAND) || (sn->sn_Command=GetConfigStr(&ptr))) &&
      (!(sbits & SOPO_PORT) || (sn->sn_Port=GetConfigStr(&ptr))))
   /* All OK. */
   return(sn);

  /* Call failed */
  FreeSoundNode((struct Node *) sn);
 }
 return(NULL);
}

/* Write Sound node to TMSO IFF chunk */
BOOL WriteSoundNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct SoundNode *sn=(struct SoundNode *) node;
 struct SoundPrefsObject *spo=(struct SoundPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &spo[1];

 /* Copy strings */
 if (PutConfigStr(sn->sn_Node.ln_Name,&ptr)) sbits|=SOPO_NAME;
 if (PutConfigStr(sn->sn_Command,&ptr)) sbits|=SOPO_COMMAND;
 if (PutConfigStr(sn->sn_Port,&ptr)) sbits|=SOPO_PORT;

 /* set string bits */
 spo->spo_StringBits=sbits;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMSO,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Read TMMO IFF chunk into Menu node */
struct Node *ReadMenuNode(UBYTE *buf, ULONG chunk_size)
{
 struct MenuNode *mn;
 struct MenuPrefsObject *mpo;
 ULONG sbits;
 UBYTE *ptr;
 UBYTE *chunk_start;
 ULONG used;
 UBYTE ext_len;

 if (!(mn=AllocMem(sizeof(struct MenuNode),MEMF_PUBLIC|MEMF_CLEAR)))
  return NULL;
 mpo=(struct MenuPrefsObject *)buf;
 sbits=mpo->mpo_StringBits;
 ptr=(UBYTE *)&mpo[1];
 chunk_start=(UBYTE *)buf;
 mn->mn_Flags=0;
 mn->mn_ParentIndex=-1;

 if (!(sbits & MOPO_NAME) || (mn->mn_Node.ln_Name=GetConfigStr(&ptr))) {
  if (!(sbits & MOPO_EXEC) || (mn->mn_Exec=GetConfigStr(&ptr))) {
   if (!(sbits & MOPO_SOUND) || (mn->mn_Sound=GetConfigStr(&ptr))) {
    if (!(sbits & MOPO_TITLE) || (mn->mn_Title=GetConfigStr(&ptr))) {
     if (!(sbits & MOPO_CMDKEY) || (mn->mn_CommandKey=GetConfigStr(&ptr))) {
      used=(ULONG)(ptr-chunk_start);
      if (chunk_size>=used+1u) {
       ext_len=*ptr++;
       if (ext_len>=8 && chunk_size>=used+9u) {
        mn->mn_Flags=*(ULONG *)ptr;
        ptr+=4;
        mn->mn_ParentIndex=*(LONG *)ptr;
       }
      }
      return (struct Node *)mn;
     }
    }
   }
  }
 }
 FreeMenuNode((struct Node *)mn);
 return NULL;
}

/* Write Menu node to TMMO IFF chunk */
BOOL WriteMenuNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct MenuNode *mn=(struct MenuNode *) node;
 struct MenuPrefsObject *mpo=(struct MenuPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &mpo[1];
 ULONG chunk_len;

 if (PutConfigStr(mn->mn_Node.ln_Name,&ptr)) sbits|=MOPO_NAME;
 if (PutConfigStr(mn->mn_Exec,&ptr)) sbits|=MOPO_EXEC;
 if (PutConfigStr(mn->mn_Sound,&ptr)) sbits|=MOPO_SOUND;
 if (mn->mn_Title && *mn->mn_Title && PutConfigStr(mn->mn_Title,&ptr))
  sbits|=MOPO_TITLE;
 if (mn->mn_CommandKey && *mn->mn_CommandKey && PutConfigStr(mn->mn_CommandKey,&ptr))
  sbits|=MOPO_CMDKEY;
 mpo->mpo_StringBits=sbits;

 if (mn->mn_Flags || mn->mn_ParentIndex!=-1) {
  *ptr++=(UBYTE)8;
  *(ULONG *)ptr=mn->mn_Flags;
  ptr+=4;
  *(LONG *)ptr=mn->mn_ParentIndex;
  ptr+=4;
 }
 chunk_len=(ULONG)(ptr-buf);
 DEBUG_PRINTF("chunk size %ld\n",(long)chunk_len);
 if (PushChunk(iff,0,ID_TMMO,chunk_len)) return FALSE;
 if (WriteChunkBytes(iff,buf,chunk_len)!=chunk_len) return FALSE;
 if (PopChunk(iff)) return FALSE;
 return TRUE;
}

/* Read TMIC IFF chunk into Icon node */
struct Node *ReadIconNode(UBYTE *buf, ULONG size)
{
 struct IconNode *in;
 (void)size;

 /* Allocate memory for node */
 if (in=AllocMem(sizeof(struct IconNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct IconPrefsObject *ipo=(struct IconPrefsObject *) buf;
  ULONG sbits=ipo->ipo_StringBits;
  UBYTE *ptr=(UBYTE *) &ipo[1];

  if ((!(sbits & ICPO_NAME) || (in->in_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_EXEC) || (in->in_Exec=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_IMAGE) || (in->in_Image=GetConfigStr(&ptr))) &&
      (!(sbits & ICPO_SOUND) || (in->in_Sound=GetConfigStr(&ptr)))) {
   /* Copy flags & values */
   in->in_Flags=ipo->ipo_Flags;
   in->in_XPos=ipo->ipo_XPos;
   in->in_YPos=ipo->ipo_YPos;

   /* All OK. */
   return(in);
  }

  /* Call failed */
  FreeIconNode((struct Node *) in);
 }
 return(NULL);
}

/* Write Icon node to TMIC IFF chunk */
BOOL WriteIconNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct IconNode *in=(struct IconNode *) node;
 struct IconPrefsObject *ipo=(struct IconPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &ipo[1];

 /* Copy strings */
 if (PutConfigStr(in->in_Node.ln_Name,&ptr)) sbits|=ICPO_NAME;
 if (PutConfigStr(in->in_Exec,&ptr)) sbits|=ICPO_EXEC;
 if (PutConfigStr(in->in_Image,&ptr)) sbits|=ICPO_IMAGE;
 if (PutConfigStr(in->in_Sound,&ptr)) sbits|=ICPO_SOUND;

 /* set string bits */
 ipo->ipo_StringBits=sbits;

 /* Copy flags & values */
 ipo->ipo_Flags=in->in_Flags;
 ipo->ipo_XPos=in->in_XPos;
 ipo->ipo_YPos=in->in_YPos;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMIC,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Read TMDO IFF chunk into Dock node */
struct Node *ReadDockNode(UBYTE *buf, ULONG size)
{
 struct DockNode *dn;
 (void)size;

 /* Allocate memory for node */
 if (dn=AllocMem(sizeof(struct DockNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct DockPrefsObject *dpo=(struct DockPrefsObject *) buf;
  ULONG sbits=dpo->dpo_StringBits;
  UBYTE *ptr=(UBYTE *) &dpo[1];
  struct List *toolslist;

  if ((!(sbits & DOPO_NAME) || (dn->dn_Node.ln_Name=GetConfigStr(&ptr))) &&
      (!(sbits & DOPO_HOTKEY) || (dn->dn_HotKey=GetConfigStr(&ptr))) &&
      (!(sbits & DOPO_PSCREEN) || (dn->dn_PubScreen=GetConfigStr(&ptr))) &&
      (!(sbits & DOPO_TITLE) || (dn->dn_Title=GetConfigStr(&ptr))) &&
      (!(sbits & DOPO_FONTNAME) || (dn->dn_Font.ta_Name=GetConfigStr(&ptr))) &&
      (toolslist=malloc(sizeof(struct List)))) {
   LONG tools=0;
   UBYTE tlflags;

   /* Init list */
   NewList(toolslist);
   dn->dn_ToolsList=toolslist;

   /* Get tools */
   while ((tlflags=*ptr++) & DOPOT_CONTINUE) {
    struct ToolNode *tn;

    if (tn=AllocMem(sizeof(struct ToolNode),MEMF_PUBLIC|MEMF_CLEAR)) {
     /* Add tool to list */
     AddTail(toolslist,(struct Node *) tn);

     if ((!(tlflags & DOPOT_EXEC) || (tn->tn_Node.ln_Name=
                                       GetConfigStr(&ptr))) &&
         (!(tlflags & DOPOT_IMAGE) || (tn->tn_Image=GetConfigStr(&ptr))) &&
         (!(tlflags & DOPOT_SOUND) || (tn->tn_Sound=GetConfigStr(&ptr))))
      /* All OK. */
      tools++;
     else {
      /* Error */
      tools=-1;
      break;
     }
    } else {
     /* No memory. */
     tools=-1;
     break;
    }
   }

   /* Error? */
   if (tools!=-1) {
    /* Got tools? */
    if (tools==0) {
     /* No, free list structure */
     free(toolslist);
     dn->dn_ToolsList=NULL;
    }

    /* Copy flags & values */
    dn->dn_Flags=dpo->dpo_Flags;
    dn->dn_XPos=dpo->dpo_XPos;
    dn->dn_YPos=dpo->dpo_YPos;
    dn->dn_Columns=dpo->dpo_Columns;
    dn->dn_Font.ta_YSize=dpo->dpo_Font.ta_YSize;
    dn->dn_Font.ta_Style=dpo->dpo_Font.ta_Style;
    dn->dn_Font.ta_Flags=dpo->dpo_Font.ta_Flags;

    /* All OK. */
    return(dn);
   }
  }

  /* Call failed */
  FreeDockNode((struct Node *) dn);
 }
 return(NULL);
}

/* Write Dock node to TMDO IFF chunk */
BOOL WriteDockNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct DockNode *dn=(struct DockNode *) node;
 struct DockPrefsObject *dpo=(struct DockPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &dpo[1];

 /* Copy strings */
 if (PutConfigStr(dn->dn_Node.ln_Name,&ptr)) sbits|=DOPO_NAME;
 if (PutConfigStr(dn->dn_HotKey,&ptr)) sbits|=DOPO_HOTKEY;
 if (PutConfigStr(dn->dn_PubScreen,&ptr)) sbits|=DOPO_PSCREEN;
 if (PutConfigStr(dn->dn_Title,&ptr)) sbits|=DOPO_TITLE;
 if (PutConfigStr(dn->dn_Font.ta_Name,&ptr)) sbits|=DOPO_FONTNAME;

 /* set string bits */
 dpo->dpo_StringBits=sbits;

 /* Write tool list */
 if (dn->dn_ToolsList) {
  struct ToolNode *tn=(struct ToolNode *)GetHead(dn->dn_ToolsList);

  while (tn) {
   UBYTE *flptr=ptr++;
   UBYTE tfl=DOPOT_CONTINUE;

   if (PutConfigStr(tn->tn_Node.ln_Name,&ptr)) tfl|=DOPOT_EXEC;
   if (PutConfigStr(tn->tn_Image,&ptr)) tfl|=DOPOT_IMAGE;
   if (PutConfigStr(tn->tn_Sound,&ptr)) tfl|=DOPOT_SOUND;

   /* Put flags */
   *flptr=tfl;

   /* Get next node */
   tn=(struct ToolNode *)GetSucc(tn);
  }
 }

 /* Append terminator */
 *ptr++=0;

 /* Copy flags & values */
 dpo->dpo_Flags=dn->dn_Flags;
 dpo->dpo_XPos=dn->dn_XPos;
 dpo->dpo_YPos=dn->dn_YPos;
 dpo->dpo_Columns=dn->dn_Columns;
 dpo->dpo_Font.ta_YSize=dn->dn_Font.ta_YSize;
 dpo->dpo_Font.ta_Style=dn->dn_Font.ta_Style;
 dpo->dpo_Font.ta_Flags=dn->dn_Font.ta_Flags;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMDO,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Read TMAC IFF chunk into Access node */
struct Node *ReadAccessNode(UBYTE *buf, ULONG size)
{
 struct AccessNode *an;
 (void)size;

 /* Allocate memory for node */
 if (an=AllocMem(sizeof(struct AccessNode),MEMF_PUBLIC|MEMF_CLEAR)) {
  struct AccessPrefsObject *apo=(struct AccessPrefsObject *) buf;
  ULONG sbits=apo->apo_StringBits;
  UBYTE *ptr=(UBYTE *) &apo[1];

  if (!(sbits & DOPO_NAME) || (an->an_Node.ln_Name=GetConfigStr(&ptr))) {
   LONG entries=0;
   UBYTE enflags;

   /* Init list */
   NewList(&an->an_Entries);

   /* Get tools */
   while ((enflags=*ptr++) & AOPOE_CONTINUE) {
    struct Node *aen;

    if (aen=AllocMem(sizeof(struct Node),MEMF_PUBLIC|MEMF_CLEAR)) {
     /* Add tool to list */
     AddTail(&an->an_Entries,aen);

     if (!(enflags & AOPOE_EXEC) || (aen->ln_Name=GetConfigStr(&ptr)))
      /* All OK. */
      entries++;
     else {
      /* Error */
      entries=-1;
      break;
     }
    } else {
     /* No memory. */
     entries=-1;
     break;
    }
   }

   /* Error? All OK. */
   if (entries!=-1) return(an);
  }

  /* Call failed */
  FreeAccessNode((struct Node *) an);
 }
 return(NULL);
}

/* Write Access node to TMAC IFF chunk */
BOOL WriteAccessNode(struct IFFHandle *iff, UBYTE *buf, struct Node *node)
{
 struct AccessNode *an=(struct AccessNode *) node;
 struct AccessPrefsObject *apo=(struct AccessPrefsObject *) buf;
 ULONG sbits=0;
 UBYTE *ptr=(UBYTE *) &apo[1];

 /* Copy strings */
 if (PutConfigStr(an->an_Node.ln_Name,&ptr)) sbits|=AOPO_NAME;

 /* set string bits */
 apo->apo_StringBits=sbits;

 /* Write entry list */
 {
  struct Node *aen=GetHead(&an->an_Entries);

  while (aen) {
   UBYTE *flptr=ptr++;
   UBYTE aefl=AOPOE_CONTINUE;

   if (PutConfigStr(aen->ln_Name,&ptr)) aefl|=AOPOE_EXEC;

   /* Put flags */
   *flptr=aefl;

   /* Get next node */
   aen=GetSucc(aen);
  }
 }

 /* Append terminator */
 *ptr++=0;

 /* calculate length */
 sbits=ptr-buf;

 DEBUG_PRINTF("chunk size %ld\n",sbits);

 /* Open chunk */
 if (PushChunk(iff,0,ID_TMAC,sbits)) return(FALSE);

 /* Write chunk */
 if (WriteChunkBytes(iff,buf,sbits)!=sbits) return(FALSE);

 /* Close chunk */
 if (PopChunk(iff)) return(FALSE);

 /* All OK. */
 return(TRUE);
}

/* Function tables */
ReadNodeFuncPtr ReadNodeFunctions[TMOBJTYPES]={
                                               ReadExecNode,
                                               ReadImageNode,
                                               ReadSoundNode,
                                               ReadMenuNode,
                                               ReadIconNode,
                                               ReadDockNode,
                                               ReadAccessNode
                                              };

WriteNodeFuncPtr WriteNodeFunctions[TMOBJTYPES]={
                                                 WriteExecNode,
                                                 WriteImageNode,
                                                 WriteSoundNode,
                                                 WriteMenuNode,
                                                 WriteIconNode,
                                                 WriteDockNode,
                                                 WriteAccessNode
                                                };

FreeNodeFuncPtr FreeNodeFunctions[TMOBJTYPES]={FreeExecNode,
                                               FreeImageNode,
                                               FreeSoundNode,
                                               FreeMenuNode,
                                               FreeIconNode,
                                               FreeDockNode,
                                               FreeAccessNode};

/* Read config file */

BOOL ReadConfigFile(char *filename)
{
 UBYTE *configbuf;
 BOOL rc=FALSE;

 DEBUG_PRINTF("read config from '%s'\n",filename);

 /* Allocate memory for config buffer */
 if (configbuf=malloc(BUFSIZE)) {
  struct IFFHandle *iff;

  DEBUG_PRINTF("config buffer: 0x%08lx\n",configbuf)

  /* Allocate IFF handle */
  if (iff=AllocIFF()) {
   /* Every error will be ignored after this point! */
   rc=TRUE;

   DEBUG_PRINTF("IFF Handle: 0x%08lx\n",iff);

   /* Open IFF File */
   if (iff->iff_Stream=Open(filename,MODE_OLDFILE)) {
    /* Init IFF handle */
    InitIFFasDOS(iff);

    DEBUG_PRINTF("IFF Stream: 0x%08lx\n",iff->iff_Stream);

    /* Open IFF handle */
    if (!OpenIFF(iff,IFFF_READ)) {

     DEBUG_PUTSTR("IFF open\n");

     /* Start IFF parsing */
     if (!ParseIFF(iff,IFFPARSE_STEP)) {
      struct ContextNode *cn;

      DEBUG_PUTSTR("First IFF scan step\n");

      /* Check IFF type and set IFF chunk types */
      if ((cn=CurrentChunk(iff)) && (cn->cn_ID==ID_FORM) &&
          (cn->cn_Type==ID_PREF) &&
          !PropChunk(iff,ID_PREF,ID_PRHD) &&
          !StopChunks(iff,stopchunks,TMOBJTYPES) &&
          !StopOnExit(iff,ID_PREF,ID_FORM) &&
          !ParseIFF(iff,IFFPARSE_SCAN)) {
       /* First stop chunk encountered */
       struct StoredProperty *sp;

       /* Get pointer to PRHD chunk */
       if (sp=FindProp(iff,ID_PREF,ID_PRHD)) {
        struct PrefHeader *ph=(struct PrefHeader *) sp->sp_Data;

        /* Check file version number */
        if (ph->ph_Version==TMPREFSVERSION) {

         /* Parse IFF chunks */
         do {
          /* Get current chunk */
          if (cn=CurrentChunk(iff)) {
           LONG type;

           DEBUG_PRINTF("chunk ID: 0x%08lx",cn->cn_ID);
           DEBUG_PRINTF(" size: 0x%08lx",cn->cn_Size);

           /* Read IFF chunk according to chunk ID */
           switch(cn->cn_ID) {
            case ID_TMEX: type=TMOBJTYPE_EXEC;
                          break;
            case ID_TMIM: type=TMOBJTYPE_IMAGE;
                          break;
            case ID_TMSO: type=TMOBJTYPE_SOUND;
                          break;
            case ID_TMMO: type=TMOBJTYPE_MENU;
                          break;
            case ID_TMIC: type=TMOBJTYPE_ICON;
                          break;
            case ID_TMDO: type=TMOBJTYPE_DOCK;
                          break;
            case ID_TMAC: type=TMOBJTYPE_ACCESS;
                          break;
            default:      type=-1;
                          break;
           }

           DEBUG_PRINTF(" type: %ld\n",type);

           /* valid type? */
           if (type!=-1) {
            ULONG size=cn->cn_Size;

            /* Read chunk */
            if (ReadChunkBytes(iff,configbuf,size)==size) {
             struct Node *node;

             DEBUG_PUTSTR("chunk read\n");

             /* Interpret chunk contents */
             if (node=(*ReadNodeFunctions[type])(configbuf,size)) {

              DEBUG_PRINTF("new node: 0x%08lx\n",node);

              /* Make sure ln_Name is valid */
              if (!node->ln_Name) node->ln_Name=strdup("");

              /* Append new node to list */
              AddTail(&ObjectLists[type],node);
             }
            }
           }
          }
         /* Next parse step */
         } while (!ParseIFF(iff,IFFPARSE_SCAN));
        }
       }
      }
     }
     CloseIFF(iff);
    }
    Close(iff->iff_Stream);
   }
   FreeIFF(iff);
  }
  free(configbuf);
 }
 return(rc);
}

/* Write config file */
BOOL WriteConfigFile(char *filename)
{
 UBYTE *configbuf;
 BOOL rc=FALSE;

 DEBUG_PRINTF("write config to '%s'\n",filename);

 /* Allocate memory for config buffer */
 if (configbuf=malloc(BUFSIZE)) {
  struct IFFHandle *iff;

  DEBUG_PRINTF("config buffer: 0x%08lx\n",configbuf)

  /* Allocate IFF handle */
  if (iff=AllocIFF()) {

   DEBUG_PRINTF("IFF Handle: 0x%08lx\n",iff);

   /* Open IFF File */
   if (iff->iff_Stream=Open(filename,MODE_NEWFILE)) {
    /* Init IFF handle */
    InitIFFasDOS(iff);

    DEBUG_PRINTF("IFF Stream: 0x%08lx\n",iff->iff_Stream);

    /* Open IFF handle */
    if (!OpenIFF(iff,IFFF_WRITE)) {

     DEBUG_PUTSTR("IFF open\n");

     /* Push FORM IFF chunk */
     if (!PushChunk(iff,ID_PREF,ID_FORM,IFFSIZE_UNKNOWN)) {

      /* Write PRHD IFF chunk */
      if (!PushChunk(iff,0,ID_PRHD,sizeof(struct PrefHeader)) &&
          (WriteChunkBytes(iff,(UBYTE *) &PrefHdrChunk,
                           sizeof(struct PrefHeader))
                            ==sizeof(struct PrefHeader)) &&
          !PopChunk(iff))
       /* Set return code */
       rc=TRUE;

      /* error? */
      if (rc) {
       ULONG i;

       /* No, scan all object lists */
       for (i=0; (i<TMOBJTYPES) && rc; i++) {
        struct Node *node=GetHead(&ObjectLists[i]);
        WriteNodeFuncPtr wnfp=WriteNodeFunctions[i];

        /* Scan list */
        while (node && rc) {
         /* Convert node into IFF chunk */
         rc=(*wnfp)(iff,configbuf,node);

         /* Get next node */
         node=GetSucc(node);
        }
       }
      }
     }
     CloseIFF(iff);
    }
    Close(iff->iff_Stream);

    /* Clear execution flag */
    SetProtection(filename,FIBF_EXECUTE);
   }
   FreeIFF(iff);
  }
  free(configbuf);
 }
 return(rc);
}

/* Free all preferences objects */
void FreeAllObjects(void)
{
 int i;

 for (i=0; i<TMOBJTYPES; i++) {
  struct List *list=&ObjectLists[i];
  FreeNodeFuncPtr freefunc=FreeNodeFunctions[i];
  struct Node *node;

  /* Scan list and free nodes */
  while (node=RemHead(list)) (*freefunc)(node);
 }
}

void WriteAscii(char *FileName)
{
 FILE *f;

 if (f = fopen(FileName, "w"))
  {
   struct Node *Node;

   fprintf(f, "Exec Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_EXEC].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct ExecNode *en = (struct ExecNode *)Node;

     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%lu %ld %d %ld %lu\n", en->en_Flags, en->en_ExecType, en->en_Priority,
	                                en->en_Delay, en->en_Stack);
     fprintf(f, "%s\n%s\n%s\n%s\n%s\n%s\n", en->en_Command, en->en_CurrentDir,
	                                    en->en_HotKey, en->en_Output,
                                            en->en_Path, en->en_PubScreen);
    }
   fprintf(f, "---\n");

   fprintf(f, "Image Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_IMAGE].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct ImageNode *in = (struct ImageNode *)Node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%s\n", in->in_File);
    }
   fprintf(f, "---\n");
   
   fprintf(f, "Sound Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_SOUND].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct SoundNode *sn = (struct SoundNode *)Node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%s\n", sn->sn_Command);
     fprintf(f, "%s\n", sn->sn_Port);
    }
   fprintf(f, "---\n");
   
   fprintf(f, "Menu Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_MENU].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct MenuNode *mn = (struct MenuNode *)Node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%s\n", mn->mn_Exec);
     fprintf(f, "%s\n", mn->mn_Sound);
    }
   fprintf(f, "---\n");

   fprintf(f, "Icon Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_ICON].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct IconNode *in = (struct IconNode *)Node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%lu\n", in->in_Flags);
     fprintf(f, "%s\n%s\n%s\n", in->in_Exec, in->in_Image, in->in_Sound);
     fprintf(f, "%ld %ld\n", in->in_XPos, in->in_YPos);
    }
   fprintf(f, "---\n");

   fprintf(f, "Dock Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_DOCK].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct DockNode *dn = (struct DockNode *)Node;
     struct Node     *node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "%lu\n", dn->dn_Flags);
     fprintf(f, "%s\n%s\n%s\n", dn->dn_HotKey, dn->dn_PubScreen, dn->dn_Title);
     fprintf(f, "%s\n%u %u %u\n", dn->dn_Font.ta_Name, dn->dn_Font.ta_YSize,
	                             dn->dn_Font.ta_Style, dn->dn_Font.ta_Flags);
     fprintf(f, "%ld %ld\n", dn->dn_XPos, dn->dn_YPos);
     fprintf(f, "%lu\n", dn->dn_Columns);

     fprintf(f, "Tools\n");
     for(node = dn->dn_ToolsList->lh_Head; node->ln_Succ; node = node->ln_Succ)
      {
       struct ToolNode *tn = (struct ToolNode *)node;

       fprintf(f, "%s\n", node->ln_Name);
       fprintf(f, "%s\n%s\n", tn->tn_Image, tn->tn_Sound);
      }
     fprintf(f, "---\n");
    }
   fprintf(f, "---\n");

   fprintf(f, "Access Objects\n");
   for(Node = ObjectLists[TMOBJTYPE_ACCESS].lh_Head; Node->ln_Succ; Node = Node->ln_Succ)
    {
     struct AccessNode *an = (struct AccessNode *)Node;
     struct Node     *node;
     
     fprintf(f, "%s\n", Node->ln_Name);
     fprintf(f, "Entries\n");
     for(node = an->an_Entries.lh_Head; node->ln_Succ; node = node->ln_Succ)
      fprintf(f, "%s\n", node->ln_Name);
     fprintf(f, "---\n");
    }
   fprintf(f, "---\n");

   fclose(f);
  }
 else
  fprintf(stderr, "can't open %s\n", FileName);
}

char *AllocStr(char *s)
{
 int   len;
 char *Buffer, *t;
 
 t = s; len = 0;
 while ((*t != '\0') && (*t != '\n'))
  {
   t++;
   len++;
  }
 s[len] = '\0';
 
 if (len == 0)
  Buffer = NULL;
 else
  {
   Buffer = malloc(len+1);
   strcpy(Buffer, s);
  } 
 
 return Buffer;
}


#define MLEN 1024

void ReadAscii(char *FileName)
{
 FILE *f;

 if (f = fopen(FileName, "r"))
  {
   char s[MLEN];

   while (fgets(s, MLEN, f))
    {
     if (Stricmp(s, "Exec Objects\n") == 0)
      {
       struct ExecNode *en;

       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 en = AllocMem(sizeof(struct ExecNode), MEMF_PUBLIC|MEMF_CLEAR);
	 en->en_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 sscanf(s, "%lu %hd %hd %ld %lu\n", &en->en_Flags, &en->en_ExecType,
		                            &en->en_Priority, &en->en_Delay,
		                            &en->en_Stack);
	 fgets(s, MLEN, f);
	 en->en_Command = AllocStr(s);
	 fgets(s, MLEN, f);
	 en->en_CurrentDir = AllocStr(s);
	 fgets(s, MLEN, f);
	 en->en_HotKey = AllocStr(s);
	 fgets(s, MLEN, f);
	 en->en_Output = AllocStr(s);
	 fgets(s, MLEN, f);
	 en->en_Path = AllocStr(s);
	 fgets(s, MLEN, f);
	 en->en_PubScreen = AllocStr(s);
	 fgets(s, MLEN, f);

	 AddTail(&ObjectLists[TMOBJTYPE_EXEC], en);
	}
      }
     else if (Stricmp(s, "Image Objects\n") == 0)
      {
       struct ImageNode *in;

       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 in = AllocMem(sizeof(struct ImageNode), MEMF_PUBLIC|MEMF_CLEAR);
	 in->in_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 in->in_File = AllocStr(s);
	 fgets(s, MLEN, f);

	 AddTail(&ObjectLists[TMOBJTYPE_IMAGE], in);
	}
      }
     else if (Stricmp(s, "Sound Objects\n") == 0)
      {
       struct SoundNode *sn;
       
       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 sn = AllocMem(sizeof(struct SoundNode), MEMF_PUBLIC|MEMF_CLEAR);
	 sn->sn_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 sn->sn_Command = AllocStr(s);
	 fgets(s, MLEN, f);
	 sn->sn_Port = AllocStr(s);
	 fgets(s, MLEN, f);

	 AddTail(&ObjectLists[TMOBJTYPE_SOUND], sn);
	}
      }
     else if (Stricmp(s, "Menu Objects\n") == 0)
      {
       struct MenuNode *mn;
       
       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 mn = AllocMem(sizeof(struct MenuNode), MEMF_PUBLIC|MEMF_CLEAR);
	 mn->mn_ParentIndex = -1;
	 mn->mn_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 mn->mn_Exec = AllocStr(s);
	 fgets(s, MLEN, f);
	 mn->mn_Sound = AllocStr(s);
	 fgets(s, MLEN, f);

	 AddTail(&ObjectLists[TMOBJTYPE_MENU], mn);
	}
      }
     else if (Stricmp(s, "Icon Objects\n") == 0)
      {
       struct IconNode *in;

       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 in = AllocMem(sizeof(struct IconNode), MEMF_PUBLIC|MEMF_CLEAR);
	 in->in_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 sscanf(s, "%lu\n", &in->in_Flags);
	 fgets(s, MLEN, f);
	 in->in_Exec = AllocStr(s);
	 fgets(s, MLEN, f);
	 in->in_Image = AllocStr(s);
	 fgets(s, MLEN, f);
	 in->in_Sound = AllocStr(s);
	 fgets(s, MLEN, f);
	 sscanf(s, "%ld %ld\n", &in->in_XPos, &in->in_YPos);
	 fgets(s, MLEN, f);

	 AddTail(&ObjectLists[TMOBJTYPE_ICON], in);
	}
      }
     else if (Stricmp(s, "Dock Objects\n") == 0)
      {
       struct DockNode *dn;

       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 UWORD h1,h2;

	 dn = AllocMem(sizeof(struct DockNode), MEMF_PUBLIC|MEMF_CLEAR);
	 dn->dn_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 sscanf(s, "%lu\n", &dn->dn_Flags);
	 fgets(s, MLEN, f);
	 dn->dn_HotKey = AllocStr(s);
	 fgets(s, MLEN, f);
	 dn->dn_PubScreen = AllocStr(s);
	 fgets(s, MLEN, f);
	 dn->dn_Title = AllocStr(s);
	 fgets(s, MLEN, f);
	 dn->dn_Font.ta_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 sscanf(s, "%hu %hu %hu\n", &dn->dn_Font.ta_YSize, &h1, &h2);
	 dn->dn_Font.ta_Style = h1;
	 dn->dn_Font.ta_Flags = h2;
	 dn->dn_FontDesc = NULL;
	 fgets(s, MLEN, f);
	 sscanf(s, "%ld %ld\n", &dn->dn_XPos, &dn->dn_YPos);
	 fgets(s, MLEN, f);
	 sscanf(s, "%lu\n", &dn->dn_Columns);
	 fgets(s, MLEN, f);
	 dn->dn_ToolsList = AllocMem(sizeof(struct List), MEMF_PUBLIC|MEMF_CLEAR);
	 NewList(dn->dn_ToolsList);
	 if (Stricmp(s, "Tools\n") == 0)
	  {
	   struct ToolNode *tn;
	   
	   fgets(s, MLEN, f);
	   while (Stricmp(s, "---\n") != 0)
	    {
	     tn = AllocMem(sizeof(struct ToolNode), MEMF_PUBLIC|MEMF_CLEAR);
	     tn->tn_Node.ln_Name = AllocStr(s);
	     fgets(s, MLEN, f);
	     tn->tn_Image = AllocStr(s);
	     fgets(s, MLEN, f);
	     tn->tn_Sound = AllocStr(s);
	     fgets(s, MLEN, f);

	     AddTail(dn->dn_ToolsList, tn);
	    }
	   fgets(s, MLEN, f);
	  }
	 else
	  fprintf(stderr, "Error 2 : %s", s);

	 AddTail(&ObjectLists[TMOBJTYPE_DOCK], dn);
	}
      }
     else if (Stricmp(s, "Access Objects\n") == 0)
      {
       struct AccessNode *an;
       
       fgets(s, MLEN, f);
       while (Stricmp(s, "---\n") != 0)
	{
	 an = AllocMem(sizeof(struct AccessNode), MEMF_PUBLIC|MEMF_CLEAR);
	 an->an_Node.ln_Name = AllocStr(s);
	 fgets(s, MLEN, f);
	 NewList(&an->an_Entries);
	 if (Stricmp(s, "Entries\n") == 0)
	  {
	   struct Node *node;

	   fgets(s, MLEN, f);
	   while (Stricmp(s, "---\n") != 0)
	    {
	     node = AllocMem(sizeof(struct Node), MEMF_PUBLIC|MEMF_CLEAR);
	     node->ln_Name = AllocStr(s);
	     fgets(s, MLEN, f);

	     AddTail(&an->an_Entries, node);
	    }
	   fgets(s, MLEN, f);
	  }
	 else
	  fprintf(stderr, "Error 3 : %s", s);
	 
	 AddTail(&ObjectLists[TMOBJTYPE_ACCESS], an);
	}
      }
     else
      fprintf(stderr, "Error 1 : %s", s);
    }

   fclose(f);
  }
 else
  fprintf(stderr, "can't open %s\n", FileName);
}


#define TMPrefs   (char *)ArgArray[0]
#define TMAscii   (char *)ArgArray[1]
#define WriteFlag (BOOL)  ArgArray[2]

void main(int argc, char *argv[])
{
 struct RDArgs     *rd;
 ULONG              ArgArray[3];
 ULONG              rc = NULL;
 char               Template[] = "TMPrefs/A,TMAscii/A,WRITE/S";

 if (argc == 0)
  {
   printf("TM2Ascii cannot run from WorkBench !!\n");
   printf("Please use CLI instead\n");
  }
 else
  {
   ArgArray[0] = NULL;
   ArgArray[1] = NULL;
   ArgArray[2] = FALSE;
   
   rd = ReadArgs(Template, ArgArray, NULL);
   
   if (rd)
    {
     BPTR lock;
     int  i;
     
     for (i=0; i<TMOBJTYPES; i++) NewList(&ObjectLists[i]);
     
     if (WriteFlag)
      if (lock = Lock(TMAscii, ACCESS_READ))
       {
	UnLock(lock);
	
	ReadAscii(TMAscii);
	WriteConfigFile(TMPrefs);
       }
      else
       {
	PrintFault(205, TMAscii); SetIoErr(205);
	rc = 20;
       }
     else
      if (lock = Lock(TMPrefs, ACCESS_READ))
       {
	UnLock(lock);
	
	ReadConfigFile(TMPrefs);
	WriteAscii(TMAscii);
       }
      else
       {
	PrintFault(205, TMPrefs); SetIoErr(205);
	rc = 20;
       }
     
     FreeAllObjects();
    }
   else
    {
     PrintFault(116, "");
     SetIoErr(116);
     rc = 20;
    }
   if (rd) FreeArgs(rd);

   exit(rc);
  }
}
