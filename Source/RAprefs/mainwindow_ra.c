/*
 * mainwindow_ra.c  RAprefs
 *
 * ReAction main window: layout and event handling.
 * Replaces GadTools main window with ChooserObject, ListBrowserObject, buttons.
 */

#include "RAprefsConf.h"

/* AppStrings indices (match locale.c array order) */
#define IX_MAINWIN_TYPE_GAD 19
#define IX_MAINWIN_TYPE_EXEC_CYCLE_LABEL 20
#define IX_MAINWIN_TYPE_IMAGE_CYCLE_LABEL 21
#define IX_MAINWIN_TYPE_SOUND_CYCLE_LABEL 22
#define IX_MAINWIN_TYPE_MENU_CYCLE_LABEL 23
#define IX_MAINWIN_TYPE_ICON_CYCLE_LABEL 24
#define IX_MAINWIN_TYPE_DOCK_CYCLE_LABEL 25
#define IX_MAINWIN_TYPE_ACCESS_CYCLE_LABEL 26
#define IX_MAINWIN_LIST_GAD 27
#define IX_MAINWIN_SORT_GAD 28
#define IX_MAINWIN_NEW_GAD 29
#define IX_MAINWIN_EDIT_GAD 30
#define IX_MAINWIN_COPY_GAD 31
#define IX_MAINWIN_SAVE_GAD 32
#define IX_MAINWIN_USE_GAD 33
#define IX_MAINWIN_TEST_GAD 34
#define IX_MAINWIN_PROJECT_MENU_LABEL 35
#define IX_MAINWIN_OPEN_MENU_LABEL 36
#define IX_MAINWIN_OPEN_MENU_SHORTCUT 37
#define IX_MAINWIN_SAVEAS_MENU_LABEL 39
#define IX_MAINWIN_SAVEAS_MENU_SHORTCUT 40
#define IX_MAINWIN_LASTSAVED_MENU_LABEL 45
#define IX_MAINWIN_LASTSAVED_MENU_SHORTCUT 46
#define IX_MAINWIN_RESTORE_MENU_LABEL 47
#define IX_MAINWIN_RESTORE_MENU_SHORTCUT 48
#define IX_MAINWIN_CREATEICONS_MENU_LABEL 50
#define IX_MAINWIN_CREATEICONS_MENU_SHORTCUT 51
#define IX_MAINWIN_WRITE_ERROR 52
#define IX_MAINWIN_TITLE 18
#define IX_WINDOW_TOP_GAD 0
#define IX_WINDOW_UP_GAD 1
#define IX_WINDOW_DOWN_GAD 2
#define IX_WINDOW_BOTTOM_GAD 3
#define IX_WINDOW_REMOVE_GAD 4
#define IX_WINDOW_CANCEL_GAD 17
#define IX_MAINWIN_APPEND_MENU_LABEL 38
#define IX_MAINWIN_ABOUT_MENU_LABEL 41
#define IX_MAINWIN_QUIT_MENU_LABEL 42
#define IX_MAINWIN_QUIT_MENU_SHORTCUT 43
#define IX_MAINWIN_EDIT_MENU_LABEL 44
#define IX_MAINWIN_SETTINGS_MENU_LABEL 49
#define IX_FILEREQ_TITLE_FILE 104
#define IX_FILEREQ_OK_GAD 106
#define IX_FILEREQ_SAVE_GAD 107
#define IX_FILEREQ_CANCEL_GAD 108

/* Gadget IDs (match layout order; 1-based for GA_ID) */
#define G_ObjType  1
#define G_ObjList  2
#define G_Top      3
#define G_Up       4
#define G_Down     5
#define G_Bottom   6
#define G_Sort     7
#define G_New      8
#define G_Edit     9
#define G_Copy    10
#define G_Remove  11
#define G_Help    12
#define G_Save    13
#define G_Use     14
#define G_Test    15
#define G_Cancel  16
#define G_MAX     16

/* Menu user data (match original mainwindow.c) */
#define MENU_PROJECT  0
#define MENU_OPEN     1
#define MENU_APPEND   2
#define MENU_SAVEAS   3
#define MENU_ABOUT    5
#define MENU_QUIT     7
#define MENU_EDIT     8
#define MENU_LSAVED   9
#define MENU_RESTORE 10
#define MENU_SETTING 11
#define MENU_CRICONS 12

#define RA_WINDOW_IDCMP (IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_GADGETUP|IDCMP_MENUPICK|IDCMP_VANILLAKEY)

/* Edit windows function tables (same as mainwindow.c) */
OpenWindowFuncPtr OpenEditWindowFunctions[TMOBJTYPES]={
 OpenExecEditWindow,
 OpenImageEditWindow,
 OpenSoundEditWindow,
 OpenMenuEditWindow,
 OpenIconEditWindow,
 OpenDockEditWindow,
 OpenAccessEditWindow
};

CopyNodeFuncPtr CopyNodeFunctions[TMOBJTYPES]={
 CopyExecNode,
 CopyImageNode,
 CopySoundNode,
 CopyMenuNode,
 CopyIconNode,
 CopyDockNode,
 CopyAccessNode
};

static Object *GL[G_MAX+1];
static Object *RAWindowObj;
static struct Window *w;
static void *aw;
static struct Menu *mn;
static struct List browserList;
static struct List *typelist;
static struct Requester DummyReq;
static struct EasyStruct es={sizeof(struct EasyStruct),0,NULL,NULL,NULL};
static char *deftooltypes[]={"USE",NULL};

/* Current list state (shared with rest of prefs) */
static struct List *CurrentList=&ObjectLists[TMOBJTYPE_EXEC];
static struct Node *CurrentNode=NULL;
static struct Node *OldNode;
static ULONG CurrentListNumber=0;
static LONG CurrentTop=0;
static LONG CurrentOrd=-1;
static OpenWindowFuncPtr OpenEditWindow=OpenExecEditWindow;
static CopyNodeFuncPtr   CopyNode=CopyExecNode;
static FreeNodeFuncPtr   FreeNode=FreeExecNode;

static char *cyclelabels[TMOBJTYPES+1];
static struct NewMenu mdata[]={
 {NM_TITLE,NULL,NULL,0,~0,NULL},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_OPEN},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_APPEND},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_SAVEAS},
 {NM_ITEM,NM_BARLABEL,NULL,0,~0,NULL},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_ABOUT},
 {NM_ITEM,NM_BARLABEL,NULL,0,~0,NULL},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_QUIT},
 {NM_TITLE,NULL,NULL,0,~0,NULL},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_LSAVED},
 {NM_ITEM,NULL,NULL,0,~0,(APTR) MENU_RESTORE},
 {NM_TITLE,NULL,NULL,0,~0,NULL},
 {NM_ITEM,NULL,NULL,0,~1,(APTR) MENU_CRICONS},
 {NM_END}
};

/* Build ListBrowser node list from CurrentList (object names) */
static void FreeBrowserList(void)
{
 struct Node *n;

 while (n=RemHead(&browserList))
  FreeListBrowserNode(n);
}

static void BuildBrowserList(void)
{
 struct Node *node;

 FreeBrowserList();
 node=GetHead(CurrentList);
 while (node) {
  struct ListBrowserNode *lbnode;
  struct Node *np;

  lbnode=AllocListBrowserNode(1,LBNCA_CopyText,TRUE,LBNCA_Text,
    (STRPTR)(node->ln_Name ? node->ln_Name : ""),TAG_END);
  if (lbnode) {
   np=(struct Node *)lbnode;
   AddTail(&browserList,np);
  }
  node=GetSucc(node);
 }
}

static void DetachObjectList(void)
{
 SetGadgetAttrs((struct Gadget *)GL[G_ObjList],w,NULL,LISTBROWSER_Labels,(ULONG)~0,TAG_END);
 FreeBrowserList();
}

static void AttachObjectList(void)
{
 BuildBrowserList();
 SetGadgetAttrs((struct Gadget *)GL[G_ObjList],w,NULL,LISTBROWSER_Labels,(ULONG)&browserList,
   LISTBROWSER_ShowSelected,TRUE,TAG_END);
}

static void DisableObjectGadgets(BOOL disable)
{
 SetGadgetAttrs((struct Gadget *)GL[G_Top],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Up],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Down],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Bottom],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Edit],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Copy],w,NULL,GA_Disabled,disable,TAG_END);
 SetGadgetAttrs((struct Gadget *)GL[G_Remove],w,NULL,GA_Disabled,disable,TAG_END);
}

static void ConfigWriteError(char *s)
{
 es.es_TextFormat=AppStrings[IX_MAINWIN_WRITE_ERROR];
 es.es_GadgetFormat=AppStrings[IX_FILEREQ_CANCEL_GAD];
 EasyRequest(w,&es,NULL,s);
}

static void LoadConfigFile(BOOL delobjs)
{
 char *file;

 FileReqParms.frp_Window=w;
 FileReqParms.frp_Title=AppStrings[IX_FILEREQ_TITLE_FILE];
 FileReqParms.frp_OKText=AppStrings[IX_FILEREQ_OK_GAD];
 FileReqParms.frp_Flags1=FRF_DOPATTERNS;
 FileReqParms.frp_Flags2=FRF_REJECTICONS;
 FileReqParms.frp_OldFile=(char *)PrefsFileName;

 if (file=OpenFileRequester(&DummyReq)) {
  DetachObjectList();
  DisableObjectGadgets(TRUE);
  if (delobjs) FreeAllObjects();
  DisableWindow(w,&DummyReq);
  if (!ReadConfigFile(file)) DisplayBeep(NULL);
  EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
  CurrentList=&ObjectLists[CurrentListNumber];
  CurrentNode=NULL;
  CurrentTop=0;
  CurrentOrd=-1;
  free(file);
  AttachObjectList();
 }
}

static void TypeGadgetFunc(ULONG num)
{
 DetachObjectList();
 CurrentList=&ObjectLists[num];
 CurrentListNumber=num;
 CurrentNode=NULL;
 CurrentTop=0;
 CurrentOrd=-1;
 OpenEditWindow=OpenEditWindowFunctions[num];
 CopyNode=CopyNodeFunctions[num];
 FreeNode=FreeNodeFunctions[num];
 DisableObjectGadgets(TRUE);
 AttachObjectList();
}

static void SortGadgetFunc(void)
{
 BOOL notfinished=TRUE;
 struct Node *first;

 DetachObjectList();
 while (notfinished) {
  notfinished=FALSE;
  if (first=GetHead(CurrentList)) {
   struct Node *second;

   while (second=GetSucc(first)) {
    if (stricmp(first->ln_Name,second->ln_Name)>0) {
     Remove(first);
     Insert(CurrentList,first,second);
     notfinished=TRUE;
    } else {
     first=second;
    }
   }
  }
 }
 CurrentNode=NULL;
 CurrentOrd=-1;
 CurrentTop=0;
 DisableObjectGadgets(TRUE);
 AttachObjectList();
}

static void EditGadgetFunc(struct Node *editnode)
{
 if (!UpdateWindow) {
  OldNode=editnode;
  if ((*OpenEditWindow)(editnode,w)) {
   DisableWindow(w,&DummyReq);
   UpdateWindow=UpdateMainWindow;
  } else
   DisplayBeep(NULL);
 }
}

static void CopyGadgetFunc(void)
{
 if (CurrentNode) {
  struct Node *newnode;

  DetachObjectList();
  if (newnode=(*CopyNode)(CurrentNode)) {
   Insert(CurrentList,newnode,CurrentNode);
   CurrentNode=newnode;
   CurrentOrd++;
   CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
  }
  AttachObjectList();
 }
}

static void *SaveGadgetFunc(void)
{
 void *rc=NULL;

 DisableWindow(w,&DummyReq);
 if (WriteConfigFile((char *)SavePrefsFileName))
  if (CopyFile((char *)SavePrefsFileName,(char *)PrefsFileName))
   rc=(void *)1;
  else
   ConfigWriteError((char *)PrefsFileName);
 else
  ConfigWriteError((char *)SavePrefsFileName);
 EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
 return(rc);
}

static void *UseGadgetFunc(void)
{
 void *rc=NULL;

 DisableWindow(w,&DummyReq);
 if (WriteConfigFile((char *)PrefsFileName))
  rc=(void *)1;
 else
  ConfigWriteError((char *)PrefsFileName);
 EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
 return(rc);
}

static void TestGadgetFunc(void)
{
 DisableWindow(w,&DummyReq);
 if (!WriteConfigFile((char *)PrefsFileName))
  ConfigWriteError((char *)PrefsFileName);
 EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
}

void InitRAMainWindow(UWORD left, UWORD fheight)
{
 (void)left;
 (void)fheight;

 cyclelabels[TMOBJTYPE_EXEC]  =AppStrings[IX_MAINWIN_TYPE_EXEC_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_IMAGE] =AppStrings[IX_MAINWIN_TYPE_IMAGE_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_SOUND] =AppStrings[IX_MAINWIN_TYPE_SOUND_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_MENU]  =AppStrings[IX_MAINWIN_TYPE_MENU_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_ICON]  =AppStrings[IX_MAINWIN_TYPE_ICON_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_DOCK]  =AppStrings[IX_MAINWIN_TYPE_DOCK_CYCLE_LABEL];
 cyclelabels[TMOBJTYPE_ACCESS]=AppStrings[IX_MAINWIN_TYPE_ACCESS_CYCLE_LABEL];
 cyclelabels[TMOBJTYPES]      =NULL;

 mdata[MENU_PROJECT].nm_Label  =AppStrings[IX_MAINWIN_PROJECT_MENU_LABEL];
 mdata[MENU_OPEN].nm_Label     =AppStrings[IX_MAINWIN_OPEN_MENU_LABEL];
 mdata[MENU_OPEN].nm_CommKey   =AppStrings[IX_MAINWIN_OPEN_MENU_SHORTCUT];
 mdata[MENU_APPEND].nm_Label   =AppStrings[IX_MAINWIN_APPEND_MENU_LABEL];
 mdata[MENU_SAVEAS].nm_Label   =AppStrings[IX_MAINWIN_SAVEAS_MENU_LABEL];
 mdata[MENU_SAVEAS].nm_CommKey =AppStrings[IX_MAINWIN_SAVEAS_MENU_SHORTCUT];
 mdata[MENU_ABOUT].nm_Label    =AppStrings[IX_MAINWIN_ABOUT_MENU_LABEL];
 mdata[MENU_QUIT].nm_Label     =AppStrings[IX_MAINWIN_QUIT_MENU_LABEL];
 mdata[MENU_QUIT].nm_CommKey   =AppStrings[IX_MAINWIN_QUIT_MENU_SHORTCUT];
 mdata[MENU_EDIT].nm_Label     =AppStrings[IX_MAINWIN_EDIT_MENU_LABEL];
 mdata[MENU_LSAVED].nm_Label   =AppStrings[IX_MAINWIN_LASTSAVED_MENU_LABEL];
 mdata[MENU_LSAVED].nm_CommKey =AppStrings[IX_MAINWIN_LASTSAVED_MENU_SHORTCUT];
 mdata[MENU_RESTORE].nm_Label  =AppStrings[IX_MAINWIN_RESTORE_MENU_LABEL];
 mdata[MENU_RESTORE].nm_CommKey=AppStrings[IX_MAINWIN_RESTORE_MENU_SHORTCUT];
 mdata[MENU_SETTING].nm_Label  =AppStrings[IX_MAINWIN_SETTINGS_MENU_LABEL];
 mdata[MENU_CRICONS].nm_Label  =AppStrings[IX_MAINWIN_CREATEICONS_MENU_LABEL];
 mdata[MENU_CRICONS].nm_CommKey=AppStrings[IX_MAINWIN_CREATEICONS_MENU_SHORTCUT];
 mdata[MENU_CRICONS].nm_Flags  =CHECKIT|MENUTOGGLE|(CreateIcons ? CHECKED : 0);

 InitRequester(&DummyReq);
}

ULONG OpenRAMainWindow(UWORD wx, UWORD wy)
{
 Object *MainLayout;
 ULONG sigmask;

 NewList(&browserList);
 typelist=ChooserLabelsA(cyclelabels);
 if (!typelist)
  return 0;

 mn=CreateMenus(mdata,GTMN_FullMenu,TRUE,TAG_DONE);
 if (!mn) {
  FreeChooserLabels(typelist);
  return 0;
 }
 if (!LayoutMenus(mn,ScreenVI,GTMN_NewLookMenus,TRUE,TAG_DONE)) {
  FreeMenus(mn);
  FreeChooserLabels(typelist);
  return 0;
 }

 BuildBrowserList();

 RAWindowObj=WindowObject,
  WA_IDCMP,RA_WINDOW_IDCMP,
  WA_Top,(LONG)wy,
  WA_Left,(LONG)wx,
  WA_SizeGadget,TRUE,
  WA_DepthGadget,TRUE,
  WA_DragBar,TRUE,
  WA_CloseGadget,TRUE,
  WA_Activate,TRUE,
  WA_Title,AppStrings[IX_MAINWIN_TITLE],
  WA_PubScreen,PublicScreen,
  WINDOW_GadgetHelp,TRUE,
  WINDOW_MenuStrip,mn,
  WINDOW_ParentGroup,MainLayout=VGroupObject,
   LAYOUT_SpaceOuter,TRUE,
   LAYOUT_BevelStyle,BVS_THIN,
   StartMember,GL[G_ObjType]=ChooserObject,
    GA_ID,G_ObjType,
    GA_RelVerify,TRUE,
    CHOOSER_Labels,typelist,
   EndMember,
   MemberLabel(AppStrings[IX_MAINWIN_TYPE_GAD]),
   StartHGroup,BAligned,
    StartMember,GL[G_ObjList]=ListBrowserObject,
     GA_ID,G_ObjList,
     GA_RelVerify,TRUE,
     LISTBROWSER_Labels,&browserList,
     LISTBROWSER_ShowSelected,TRUE,
    EndMember,
    StartVGroup,
     StartMember,GL[G_Top]=ButtonObject,GA_ID,G_Top,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_TOP_GAD],GA_Disabled,TRUE,ButtonEnd,
     StartMember,GL[G_Up]=ButtonObject,GA_ID,G_Up,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_UP_GAD],GA_Disabled,TRUE,ButtonEnd,
     StartMember,GL[G_Down]=ButtonObject,GA_ID,G_Down,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_DOWN_GAD],GA_Disabled,TRUE,ButtonEnd,
     StartMember,GL[G_Bottom]=ButtonObject,GA_ID,G_Bottom,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_BOTTOM_GAD],GA_Disabled,TRUE,ButtonEnd,
     StartMember,GL[G_Sort]=ButtonObject,GA_ID,G_Sort,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_SORT_GAD],ButtonEnd,
    EndGroup,
    CHILD_WeightedWidth,0,
    CHILD_WeightedHeight,0,
   EndGroup,
   StartHGroup,EvenSized,
    StartMember,GL[G_New]=ButtonObject,GA_ID,G_New,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_NEW_GAD],ButtonEnd,
    StartMember,GL[G_Edit]=ButtonObject,GA_ID,G_Edit,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_EDIT_GAD],GA_Disabled,TRUE,ButtonEnd,
    StartMember,GL[G_Copy]=ButtonObject,GA_ID,G_Copy,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_COPY_GAD],GA_Disabled,TRUE,ButtonEnd,
    StartMember,GL[G_Remove]=ButtonObject,GA_ID,G_Remove,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_REMOVE_GAD],GA_Disabled,TRUE,ButtonEnd,
   EndGroup,
   CHILD_WeightedHeight,0,
   StartHGroup,EvenSized,
    StartMember,GL[G_Save]=ButtonObject,GA_ID,G_Save,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_SAVE_GAD],ButtonEnd,
    StartMember,GL[G_Use]=ButtonObject,GA_ID,G_Use,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_USE_GAD],ButtonEnd,
    StartMember,GL[G_Test]=ButtonObject,GA_ID,G_Test,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_MAINWIN_TEST_GAD],ButtonEnd,
    StartMember,GL[G_Cancel]=ButtonObject,GA_ID,G_Cancel,GA_RelVerify,TRUE,GA_Text,AppStrings[IX_WINDOW_CANCEL_GAD],ButtonEnd,
   EndGroup,
   CHILD_WeightedHeight,0,
   StartMember,GL[G_Help]=ButtonObject,GA_ID,G_Help,GA_ReadOnly,TRUE,GA_Text,"",ButtonEnd,
   CHILD_WeightedHeight,0,
  EndGroup,
 EndWindow;

 if (!RAWindowObj) {
  FreeMenus(mn);
  FreeChooserLabels(typelist);
  FreeBrowserList();
  return 0;
 }

 if (!(w=(struct Window *)RA_OpenWindow(RAWindowObj))) {
  DisposeObject(RAWindowObj);
  FreeMenus(mn);
  FreeChooserLabels(typelist);
  FreeBrowserList();
  return 0;
 }

 /* Set GA_ID/GA_RelVerify on underlying gadgets after window open so IDCMP reports correct ID */
 {
  LONG i;

  i=1;
  do {
   SetGadgetAttrs((struct Gadget *)GL[i],w,NULL,GA_ID,i,GA_RelVerify,TRUE,TAG_END);
   i++;
  } while (i<=G_MAX);
}

 UnlockPubScreen(NULL,PublicScreen);

 GetAttr(WINDOW_SigMask,RAWindowObj,&sigmask);
 IDCMPPort=w->UserPort;
 CurrentWindow=w;
 HandleAppMsg=HandleRAMainWindowAppMsg;

 if (WorkbenchBase && WBScreen)
  aw=AddAppWindowA(0,0,w,AppMsgPort,NULL);
 else
  aw=NULL;

 SetGadgetAttrs((struct Gadget *)GL[G_ObjType],w,NULL,CHOOSER_Active,CurrentListNumber,TAG_END);

 return sigmask;
}

void CloseRAMainWindow(void)
{
 if (aw) {
  HandleAppMsg=NULL;
  RemoveAppWindow(aw);
 }
 ClearMenuStrip(w);
 DisposeObject(RAWindowObj);
 RAWindowObj=NULL;
 w=NULL;
 FreeMenus(mn);
 FreeChooserLabels(typelist);
 FreeBrowserList();
}

/* Alias so execwindow.c/imagewindow.c can restore HandleAppMsg on close */
void HandleMainWindowAppMsg(struct AppMessage *msg)
{
 HandleRAMainWindowAppMsg(msg);
}

void HandleRAMainWindowAppMsg(struct AppMessage *msg)
{
 struct WBArg *wa;
 BPTR olddir;
 struct DiskObject *dobj;

 if (!(wa=msg->am_ArgList))
  return;
 olddir=CurrentDir(wa->wa_Lock);
 if (dobj=GetDiskObjectNew(wa->wa_Name)) {
  switch (dobj->do_Type) {
   case WBTOOL:
   case WBPROJECT:
    if (!UpdateWindow && OpenSelectWindow(wa,w)) {
     DisableWindow(w,&DummyReq);
     DetachObjectList();
     DisableObjectGadgets(TRUE);
     UpdateWindow=UpdateAppMainWindow;
    } else
     DisplayBeep(NULL);
    break;
   default:
    DisplayBeep(NULL);
    break;
  }
  FreeDiskObject(dobj);
 } else
  DisplayBeep(NULL);
 CurrentDir(olddir);
}

/* Update main window after select window closed */
void UpdateAppMainWindow(void *data)
{
 if (data!=(void *)-1) {
  CurrentNode=NULL;
  CurrentTop=0;
  CurrentOrd=-1;
 }
 DisableObjectGadgets(CurrentNode==NULL);
 AttachObjectList();
 EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
 UpdateWindow=NULL;
 CurrentWindow=w;
}

/* Update main window after edit window closed */
void UpdateMainWindow(void *data)
{
 struct Node *NewNode;

 DetachObjectList();
 if (data!=(void *)-1) {
  NewNode=(struct Node *)data;
  if (!NewNode->ln_Name)
   NewNode->ln_Name=strdup("");
  if (OldNode) {
   Insert(CurrentList,NewNode,OldNode);
   Remove(OldNode);
   (*FreeNode)(OldNode);
   CurrentNode=NewNode;
  } else {
   if (CurrentNode) {
    Insert(CurrentList,NewNode,CurrentNode);
    CurrentOrd++;
    CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
   } else {
    struct Node *tmpnode;
    ULONG i;

    AddTail(CurrentList,NewNode);
    tmpnode=GetHead(CurrentList);
    i=0;
    while (tmpnode) {
     i++;
     tmpnode=GetSucc(tmpnode);
    }
    CurrentOrd=(LONG)(i-1);
    CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
   }
   CurrentNode=NewNode;
  }
 }
 DisableObjectGadgets(CurrentNode==NULL);
 AttachObjectList();
 EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
 UpdateWindow=NULL;
 CurrentWindow=w;
}

static BOOL DoMenuPick(USHORT menunum)
{
 struct MenuItem *menuitem;
 ULONG userdata;

 while (menunum!=MENUNULL) {
  menuitem=ItemAddress(mn,menunum);
  userdata=(ULONG)GTMENUITEM_USERDATA(menuitem);
  switch (userdata) {
   case MENU_OPEN:
    LoadConfigFile(TRUE);
    break;
   case MENU_APPEND:
    LoadConfigFile(FALSE);
    break;
   case MENU_SAVEAS: {
     char *file;

     FileReqParms.frp_Window=w;
     FileReqParms.frp_Title=AppStrings[IX_FILEREQ_TITLE_FILE];
     FileReqParms.frp_OKText=AppStrings[IX_FILEREQ_SAVE_GAD];
     FileReqParms.frp_Flags1=FRF_DOSAVEMODE;
     FileReqParms.frp_Flags2=FRF_REJECTICONS;
     FileReqParms.frp_OldFile=(char *)SavePrefsFileName;
     if (file=OpenFileRequester(&DummyReq)) {
      DisableWindow(w,&DummyReq);
      if (WriteConfigFile(file)) {
       if (CreateIcons) {
        struct DiskObject *dobj;

        if (dobj=GetDiskObjectNew(file)) {
         char *deftool=dobj->do_DefaultTool;
         char **tooltypes=dobj->do_ToolTypes;
         UBYTE type=dobj->do_Type;

         dobj->do_DefaultTool=ProgramName;
         dobj->do_ToolTypes=deftooltypes;
         dobj->do_Type=WBPROJECT;
         PutDiskObject(file,dobj);
         dobj->do_DefaultTool=deftool;
         dobj->do_ToolTypes=tooltypes;
         dobj->do_Type=type;
         FreeDiskObject(dobj);
        }
       }
      } else
       ConfigWriteError(file);
      EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
      free(file);
     }
    }
    break;
   case MENU_ABOUT: {
     es.es_TextFormat="ToolManager " TMVERSION "." TMREVISION " (" __COMMODORE_DATE__ ")\nFreely distributable\n(c) " TMCRYEAR "  Stefan Becker";
     es.es_GadgetFormat=AppStrings[IX_FILEREQ_CANCEL_GAD];
     DisableWindow(w,&DummyReq);
     EasyRequest(w,&es,NULL,NULL);
     EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
    }
    break;
   case MENU_QUIT:
    return TRUE;
   case MENU_LSAVED:
    DetachObjectList();
    DisableObjectGadgets(TRUE);
    FreeAllObjects();
    DisableWindow(w,&DummyReq);
    if (!ReadConfigFile((char *)SavePrefsFileName))
     DisplayBeep(NULL);
    EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
    CurrentList=&ObjectLists[CurrentListNumber];
    CurrentNode=NULL;
    CurrentTop=0;
    CurrentOrd=-1;
    AttachObjectList();
    break;
   case MENU_RESTORE:
    DetachObjectList();
    DisableObjectGadgets(TRUE);
    FreeAllObjects();
    DisableWindow(w,&DummyReq);
    if (!ReadConfigFile((char *)PrefsFileName))
     DisplayBeep(NULL);
    EnableWindow(w,&DummyReq,RA_WINDOW_IDCMP);
    CurrentList=&ObjectLists[CurrentListNumber];
    CurrentNode=NULL;
    CurrentTop=0;
    CurrentOrd=-1;
    AttachObjectList();
    break;
   case MENU_CRICONS:
    CreateIcons=(menuitem->Flags&CHECKED)!=0;
    break;
   default:
    break;
  }
  menunum=menuitem->NextSelect;
 }
 return FALSE;
}

BOOL HandleRAMainWindowEvent(Object *windowObj, ULONG result, UWORD code)
{
 ULONG gadid;
 void *closeval;
 ULONG i;
 struct Node *pred;
 struct Node *succ;
 struct Node *tmpnode;

 (void)windowObj;

 switch (result&WMHI_CLASSMASK) {
  case WMHI_CLOSEWINDOW:
   return (BOOL)(!UpdateWindow);

  case WMHI_GADGETUP:
   gadid=result&WMHI_GADGETMASK;
   switch (gadid) {
    case G_ObjType:
     TypeGadgetFunc(code);
     break;
    case G_ObjList: {
     LONG ord;

     ord=(LONG)((code==(UWORD)~0) ? -1 : code);
     CurrentOrd=ord;
     CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
     if (CurrentOrd>=0) {
      CurrentNode=GetHead(CurrentList);
      for (i=0; i<(ULONG)CurrentOrd && CurrentNode; i++)
       CurrentNode=GetSucc(CurrentNode);
     } else
      CurrentNode=NULL;
     DisableObjectGadgets(CurrentNode==NULL);
     break;
    }
    case G_Top:
     if (CurrentNode) {
      DetachObjectList();
      Remove(CurrentNode);
      AddHead(CurrentList,CurrentNode);
      CurrentTop=0;
      CurrentOrd=0;
      AttachObjectList();
     }
     break;
    case G_Up:
     if (CurrentNode && (pred=GetPred(CurrentNode))) {
      DetachObjectList();
      pred=GetPred(pred);
      Remove(CurrentNode);
      Insert(CurrentList,CurrentNode,pred);
      CurrentOrd--;
      CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
      AttachObjectList();
     }
     break;
    case G_Down:
     if (CurrentNode && (succ=GetSucc(CurrentNode))) {
      DetachObjectList();
      Remove(CurrentNode);
      Insert(CurrentList,CurrentNode,succ);
      CurrentOrd++;
      CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
      AttachObjectList();
     }
     break;
    case G_Bottom:
     if (CurrentNode) {
      ULONG cnt;

      DetachObjectList();
      Remove(CurrentNode);
      AddTail(CurrentList,CurrentNode);
      tmpnode=GetHead(CurrentList);
      cnt=0;
      while (tmpnode) {
       cnt++;
       tmpnode=GetSucc(tmpnode);
      }
      CurrentOrd=(LONG)(cnt-1);
      CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
      AttachObjectList();
     }
     break;
    case G_Sort:
     SortGadgetFunc();
     break;
    case G_New:
     EditGadgetFunc(NULL);
     break;
    case G_Edit:
     EditGadgetFunc(CurrentNode);
     break;
    case G_Copy:
     CopyGadgetFunc();
     break;
    case G_Remove:
     if (CurrentNode) {
      DetachObjectList();
      DisableObjectGadgets(TRUE);
      Remove(CurrentNode);
      (*FreeNode)(CurrentNode);
      CurrentNode=NULL;
      CurrentTop=(CurrentOrd>ListViewRows-4) ? CurrentOrd-ListViewRows+4 : 0;
      CurrentOrd=-1;
      AttachObjectList();
     }
     break;
    case G_Save:
     closeval=SaveGadgetFunc();
     return (BOOL)(closeval!=NULL);
    case G_Use:
     closeval=UseGadgetFunc();
     return (BOOL)(closeval!=NULL);
    case G_Test:
     TestGadgetFunc();
     break;
    case G_Cancel:
     return (BOOL)(!UpdateWindow);
    default:
     break;
   }
   break;

  case WMHI_MENUPICK:
   return DoMenuPick((USHORT)code);

  case WMHI_VANILLAKEY:
   /* Optional: map key shortcuts; same as original KeyArray */
   break;

  default:
   break;
 }

 return FALSE;
}

Object *GetRAMainWindowObject(void)
{
 return RAWindowObj;
}
