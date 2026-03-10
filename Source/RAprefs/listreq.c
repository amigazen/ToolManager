/*
 * listreq.c  RAprefs
 *
 * List requester: ReAction GUI (WindowObject + ListBrowserObject + OK/Cancel).
 * Integrates via SubWindowRAObject and main loop RA_HandleInput path.
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "RAprefsConf.h"

#ifndef LBRE_DOUBLECLICK
#define LBRE_DOUBLECLICK 4
#endif

#define GAD_LIST   1
#define GAD_OK     2
#define GAD_CANCEL 3

static struct List *CurrentList;
static struct List listReqBrowserList;
static Object *RAListReqWindowObj;
static Object *RAListReqBrowserObj;
static char *ListReqTitle;

/* Free list browser nodes built from CurrentList */
static void FreeListReqBrowserList(void)
{
 struct Node *n;

 while (n=RemHead(&listReqBrowserList))
  FreeListBrowserNode((struct ListBrowserNode *)n);
}

/* Build ListBrowser node list from CurrentList (same pattern as main window) */
static void BuildListReqBrowserList(void)
{
 struct Node *node;

 FreeListReqBrowserList();
 node=GetHead(CurrentList);
 while (node) {
  struct ListBrowserNode *lbnode;

  lbnode=AllocListBrowserNode(1,LBNCA_CopyText,TRUE,LBNCA_Text,
    node->ln_Name ? node->ln_Name : "",TAG_END);
  if (lbnode)
   AddTail(&listReqBrowserList,(struct Node *)lbnode);
  node=GetSucc(node);
 }
}

/* Called from main loop when ReAction list requester is closed */
void CloseRAListRequester(void)
{
 if (RAListReqWindowObj) {
  DisposeObject(RAListReqWindowObj);
  RAListReqWindowObj=NULL;
 }
 RAListReqBrowserObj=NULL;
 FreeListReqBrowserList();
}

/* Return the struct Node* at index in CurrentList, or NULL */
static struct Node *GetNodeAtIndex(LONG index)
{
 struct Node *node;

 if (index<0) return NULL;
 node=GetHead(CurrentList);
 while (index>0 && node) {
  node=GetSucc(node);
  index--;
 }
 return node;
}

/* ReAction list requester event handler; returns TRUE when requester should close */
BOOL HandleRAListReqEvent(Object *windowObj, ULONG result, UWORD code)
{
 LONG sel;
 struct Node *node;

 (void)windowObj;

 switch (result) {
  case WMHI_CLOSEWINDOW:
   SubWindowRAReturnData=LREQRET_CANCEL;
   return TRUE;

  case WMHI_GADGETUP:
   switch (code) {
    case GAD_OK:
     sel=-1;
     if (RAListReqBrowserObj)
      GetAttr(LISTBROWSER_Selected,RAListReqBrowserObj,(ULONG *)&sel);
     node=GetNodeAtIndex(sel);
     SubWindowRAReturnData=node ? node : LREQRET_NOSELECT;
     return TRUE;

    case GAD_CANCEL:
     SubWindowRAReturnData=LREQRET_CANCEL;
     return TRUE;

    case GAD_LIST: {
     ULONG relEvent;

     /* Double-click on list item: close with selection (match original prefs) */
     relEvent=0;
     if (RAListReqBrowserObj)
      GetAttr(LISTBROWSER_RelEvent,RAListReqBrowserObj,(ULONG *)&relEvent);
     if (relEvent==LBRE_DOUBLECLICK) {
      sel=-1;
      if (RAListReqBrowserObj)
       GetAttr(LISTBROWSER_Selected,RAListReqBrowserObj,(ULONG *)&sel);
      node=GetNodeAtIndex(sel);
      SubWindowRAReturnData=node ? node : LREQRET_NOSELECT;
      return TRUE;
     }
     break;
    }
   }
   break;
 }

 return FALSE;
}

/* Init list requester (no layout calc needed for ReAction) */
void InitListRequester(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;
 NewList(&listReqBrowserList);
}

/* Open list requester (ReAction) */
BOOL OpenListRequester(ULONG type, struct Window *oldwindow)
{
 Object *layout;
 struct Window *w;

 switch (type) {
  case LISTREQ_EXEC:  CurrentList=&ObjectLists[TMOBJTYPE_EXEC];
                      ListReqTitle=AppStrings[MSG_LISTREQ_TITLE_EXEC];
                      break;
  case LISTREQ_IMAGE: CurrentList=&ObjectLists[TMOBJTYPE_IMAGE];
                      ListReqTitle=AppStrings[MSG_LISTREQ_TITLE_IMAGE];
                      break;
  case LISTREQ_SOUND: CurrentList=&ObjectLists[TMOBJTYPE_SOUND];
                      ListReqTitle=AppStrings[MSG_LISTREQ_TITLE_SOUND];
                      break;
  case LISTREQ_DOCK:  CurrentList=&ObjectLists[TMOBJTYPE_DOCK];
                      ListReqTitle=AppStrings[MSG_LISTREQ_TITLE_DOCK];
                      break;
  case LISTREQ_PUBSC: CurrentList=&PubScreenList;
                      ListReqTitle=AppStrings[MSG_LISTREQ_TITLE_PUBSCREEN];
                      break;
  default:
   return FALSE;
 }

 BuildListReqBrowserList();

 layout=VGroupObject,
  LAYOUT_SpaceOuter,TRUE,
  LAYOUT_SpaceInner,TRUE,
  LAYOUT_BevelStyle,BVS_THIN,
  StartMember,
   RAListReqBrowserObj=ListBrowserObject,
    GA_ID,GAD_LIST,
    GA_RelVerify,TRUE,
    LISTBROWSER_Labels,&listReqBrowserList,
    LISTBROWSER_ShowSelected,TRUE,
   EndMember,
  StartHGroup,EvenSized,
   StartMember,ButtonObject,GA_ID,GAD_OK,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_OK_GAD],ButtonEnd,
   StartMember,ButtonObject,GA_ID,GAD_CANCEL,GA_RelVerify,TRUE,GA_Text,AppStrings[MSG_WINDOW_CANCEL_GAD],ButtonEnd,
  EndGroup,
 EndGroup;

 if (!layout) {
  FreeListReqBrowserList();
  return FALSE;
 }

 RAListReqWindowObj=WindowObject,
  WA_PubScreen,PublicScreen,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_RMBTRAP|WFLG_ACTIVATE,
  WA_Title,ListReqTitle,
  WINDOW_RefWindow,oldwindow,
  WINDOW_Position,WPOS_CENTERWINDOW,
  WINDOW_ParentGroup,layout,
 EndWindow;

 if (!RAListReqWindowObj) {
  DisposeObject(layout);
  FreeListReqBrowserList();
  return FALSE;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAListReqWindowObj))) {
  DisposeObject(RAListReqWindowObj);
  RAListReqWindowObj=NULL;
  RAListReqBrowserObj=NULL;
  FreeListReqBrowserList();
  return FALSE;
 }

 CurrentWindow=w;
 SubWindowPort=w->UserPort;
 SubWindowRAObject=RAListReqWindowObj;
 SubWindowRAHandler=HandleRAListReqEvent;
 SubWindowRACloseFunc=CloseRAListRequester;

 return TRUE;
}

/* Close list requester (called when already closed via RA path; no-op if RA, else for compatibility) */
void CloseListRequester(void)
{
 if (RAListReqWindowObj) {
  CloseRAListRequester();
  SubWindowRAObject=NULL;
  SubWindowPort=NULL;
  SubWindowRAHandler=NULL;
  SubWindowRACloseFunc=NULL;
 }
}

/* Stub for compatibility with code expecting GadTools list requester IDCMP handler */
void *HandleListRequesterIDCMP(struct IntuiMessage *msg)
{
 (void)msg;
 return NULL;
}
