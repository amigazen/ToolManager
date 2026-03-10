/*
 * movewindow.c  V2.1 / RAprefs
 *
 * Move window: drag window to set X/Y. Supports GadTools integer gadgets
 * or ReAction integer objects via OpenMoveWindowRA().
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerConf.h"
#include <gadgets/integer.h>

struct Window *MoveWindowPtr=NULL;
static UWORD ww,wh;

static struct IntuiText it={1,0,JAM2,INTERWIDTH,INTERHEIGHT/2,NULL,NULL,NULL};
static struct Gadget g={NULL,0,0,0,0,GFLG_GADGHNONE|GFLG_LABELITEXT,
                        GACT_IMMEDIATE,GTYP_SYSGADGET|GTYP_WDRAGGING,NULL,NULL,
                        &it,0,NULL,0,NULL};
static struct Window *GadWindow;
static struct Gadget *XGad,*YGad;
static Object *MoveRAXObj,*MoveRAYObj;
static ULONG OldX,OldY;
ULONG MoveWindowOffX,MoveWindowOffY;

/* Init move window */
void InitMoveWindow(UWORD left, UWORD fheight)
{
 struct DrawInfo *dri;
 char *s=AppStrings[MSG_MOVEWIN_DRAG_GAD];

 /* Get screen draw info */
 if (dri=GetScreenDrawInfo(PublicScreen)) {
  /* Set pens for IntuiText */
  it.FrontPen=dri->dri_Pens[TEXTPEN];
  it.BackPen=dri->dri_Pens[BACKGROUNDPEN];

  FreeScreenDrawInfo(PublicScreen,dri);
 }

 /* Set gadget text & text attr */
 it.ITextFont=&ScreenTextAttr;
 it.IText=s;

 /* Calculate drag gadget & window sizes */
 ww=TextLength(&TmpRastPort,s,strlen(s))+2*INTERWIDTH;
 wh=fheight;
 g.Width=ww;
 g.Height=wh;
}

/* Open move window (GadTools integer gadgets) */
void OpenMoveWindow(struct Window *w, struct Gadget *xgad,
                                      struct Gadget *ygad)
{
 MoveRAXObj=NULL;
 MoveRAYObj=NULL;
 OldX=((struct StringInfo *) xgad->SpecialInfo)->LongInt+MoveWindowOffX;
 OldY=((struct StringInfo *) ygad->SpecialInfo)->LongInt+MoveWindowOffY;

 if (MoveWindowPtr=OpenWindowTags(NULL,WA_Left,       OldX,
                                       WA_Top,        OldY,
                                       WA_Width,      ww,
                                       WA_Height,     wh,
                                       WA_AutoAdjust, TRUE,
                                       WA_PubScreen,  PublicScreen,
                                       WA_Flags,      WFLG_BORDERLESS|
                                                      WFLG_RMBTRAP,
                                       WA_Gadgets,    &g,
                                       TAG_DONE)) {
  DrawBevelBox(MoveWindowPtr->RPort,0,0,ww,wh,GT_VisualInfo,ScreenVI,
                                              TAG_DONE);
  MoveWindowPtr->UserData=(BYTE *) HandleMoveWindowIDCMP;
  ModifyIDCMP(MoveWindowPtr,IDCMP_INTUITICKS|IDCMP_INACTIVEWINDOW);
  GadWindow=w;
  XGad=xgad;
  YGad=ygad;
  SavedSubWindowPort=SubWindowPort;
  SavedSubWindowHandler=SubWindowHandler;
  SubWindowPort=MoveWindowPtr->UserPort;
  SubWindowHandler=HandleMoveWindowIDCMP;
 }
}

/* Open move window (ReAction integer objects) */
void OpenMoveWindowRA(struct Window *w, Object *xIntObj, Object *yIntObj)
{
 LONG v;

 XGad=NULL;
 YGad=NULL;
 MoveRAXObj=xIntObj;
 MoveRAYObj=yIntObj;
 GadWindow=w;
 OldX=0;
 OldY=0;
 if (xIntObj) GetAttr(INTEGER_Number,xIntObj,(ULONG *)&OldX);
 if (yIntObj) GetAttr(INTEGER_Number,yIntObj,(ULONG *)&OldY);
 v=OldX; OldX=(ULONG)v+MoveWindowOffX;
 v=OldY; OldY=(ULONG)v+MoveWindowOffY;

 if (MoveWindowPtr=OpenWindowTags(NULL,WA_Left,       (LONG)OldX,
                                       WA_Top,        (LONG)OldY,
                                       WA_Width,      ww,
                                       WA_Height,     wh,
                                       WA_AutoAdjust, TRUE,
                                       WA_PubScreen,  PublicScreen,
                                       WA_Flags,      WFLG_BORDERLESS|
                                                      WFLG_RMBTRAP,
                                       WA_Gadgets,    &g,
                                       TAG_DONE)) {
  DrawBevelBox(MoveWindowPtr->RPort,0,0,ww,wh,GT_VisualInfo,ScreenVI,
                                              TAG_DONE);
  MoveWindowPtr->UserData=(BYTE *) HandleMoveWindowIDCMP;
  ModifyIDCMP(MoveWindowPtr,IDCMP_INTUITICKS|IDCMP_INACTIVEWINDOW);
  SavedSubWindowPort=SubWindowPort;
  SavedSubWindowHandler=SubWindowHandler;
  SavedSubWindowRAObject=SubWindowRAObject;
  SavedSubWindowRAHandler=SubWindowRAHandler;
  SavedSubWindowRACloseFunc=SubWindowRACloseFunc;
  SubWindowRAObject=NULL;
  SubWindowRAHandler=NULL;
  SubWindowRACloseFunc=NULL;
  SubWindowPort=MoveWindowPtr->UserPort;
  SubWindowHandler=HandleMoveWindowIDCMP;
 }
}

/* Close move window */
void CloseMoveWindow(void)
{
 RemoveGList(MoveWindowPtr,&g,-1);
 CloseWindowSafely(MoveWindowPtr);
 MoveWindowPtr=NULL;
 MoveRAXObj=NULL;
 MoveRAYObj=NULL;
 SubWindowPort=SavedSubWindowPort;
 SubWindowHandler=SavedSubWindowHandler;
 SubWindowRAObject=SavedSubWindowRAObject;
 SubWindowRAHandler=SavedSubWindowRAHandler;
 SubWindowRACloseFunc=SavedSubWindowRACloseFunc;
 SavedSubWindowRAObject=NULL;
 SavedSubWindowRAHandler=NULL;
 SavedSubWindowRACloseFunc=NULL;
}

/* Handle move window IDCMP events */
void *HandleMoveWindowIDCMP(struct IntuiMessage *msg)
{
 ULONG val;

 (void)msg;
 if ((val=MoveWindowPtr->LeftEdge)!=OldX) {
  OldX=val;
  if (XGad)
   GT_SetGadgetAttrs(XGad,GadWindow,NULL,GTIN_Number,val-MoveWindowOffX,TAG_DONE);
  else if (MoveRAXObj)
   SetAttrs(MoveRAXObj,INTEGER_Number,(LONG)(val-MoveWindowOffX),TAG_END);
 }
 if ((val=MoveWindowPtr->TopEdge)!=OldY) {
  OldY=val;
  if (YGad)
   GT_SetGadgetAttrs(YGad,GadWindow,NULL,GTIN_Number,val-MoveWindowOffY,TAG_DONE);
  else if (MoveRAYObj)
   SetAttrs(MoveRAYObj,INTEGER_Number,(LONG)(val-MoveWindowOffY),TAG_END);
 }
 return NULL;
}
