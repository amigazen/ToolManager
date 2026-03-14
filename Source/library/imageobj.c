/*
 * imageobj.c  V2.1
 *
 * TMObject, Type: Image
 *
 * (c) 1990-1993 Stefan Becker
 *
 * V44: When icon.library 44+ is available, uses GetIconTagList/DrawIconStateA
 * for palette-mapped icons; TMOP_Screen (Option B) sets remap target.
 */

#include "ToolManagerLib.h"

/* Internal only: optional datatypes image load (dtimage.c); not part of public API. */
struct TMImageData *ReadImageViaDataTypes(char *name);

/* extended TMTimerReq structure */
struct TMTimerReqImage {
                        struct TMTimerReq   tmtri_TimerReq;
                        struct Image        tmtri_Image;
                        struct TMImageNode *tmtri_NextImage;
                       };

/* extended TMObject structure for TMOBJTYPE_Image objects */
struct TMObjectImage {
                      struct TMObject     io_Object;
                      ULONG               io_Flags;
                      char               *io_File;
                      struct TMImageData *io_Data;
                      UWORD               io_XSize;
                      UWORD               io_YSize;
                      struct Image       *io_Normal;
                      struct Image       *io_Selected;
                     };

/* io_Flags */
#define IO_FreeData         (1L<<0)
#define IO_DiskObj          (1L<<1)
#define IO_UseDrawIconState (1L<<2)  /* V44: use DrawIconStateA; icon in io_Data */

/* Data */
static ULONG IconCount=0;
static ULONG IconVersion=0;  /* 44 when V44+ available, else 37 */
struct Library *IconBase=NULL;

/* Try to open icon.library; prefer 44 for palette-mapped support, fall back to 37 */
static BOOL GetIconLibrary(void)
{
 if (IconBase) {
  IconCount++;
  return(TRUE);
 }
 /* Try V44 first for GetIconTagList/DrawIconStateA/LayoutIconA */
 if ((IconBase=OpenLibrary("icon.library",44))) {
  IconVersion=44;
  IconCount++;
  return(TRUE);
 }
 if ((IconBase=OpenLibrary("icon.library",37))) {
  IconVersion=37;
  IconCount++;
  return(TRUE);
 }
 return(FALSE);
}

/* Try to close icon.library */
static void FreeIconLibrary(void)
{
 if (--IconCount==0) {
  CloseLibrary(IconBase);
  IconBase=NULL;
  IconVersion=0;
 }
}

/* Set icon.library global remap screen (V44); call when dock opens/closes (Option B). */
void IconSetGlobalScreen(struct Screen *screen)
{
 if (IconBase && IconVersion>=44) {
  struct TagItem tag[2];
  tag[0].ti_Tag=ICONCTRLA_SetGlobalScreen;
  tag[0].ti_Data=(ULONG)screen;
  tag[1].ti_Tag=TAG_DONE;
  tag[1].ti_Data=0;
  IconControlA(NULL,tag);
 }
}

/* Remap a palette-mapped icon for the given screen and update link size (V44). */
void LayoutIconForImageLink(struct TMLinkImage *tmli, struct Screen *screen)
{
 struct TMObjectImage *tmobj;

 if (!IconBase || IconVersion<44 || !tmli || !tmli->tmli_Link.tml_Linked)
  return;
 if (tmli->tmli_Link.tml_Linked->tmo_Type!=TMOBJTYPE_IMAGE)
  return;
 tmobj=(struct TMObjectImage *)tmli->tmli_Link.tml_Linked;
 if (!(tmobj->io_Flags&IO_UseDrawIconState) || !tmobj->io_Data)
  return;
 if (LayoutIconA((struct DiskObject *)tmobj->io_Data,screen,NULL)) {
  struct Rectangle rect;
  if (GetIconRectangleA(NULL,(struct DiskObject *)tmobj->io_Data,
                        NULL,&rect,NULL)) {
   tmli->tmli_Width=(UWORD)(rect.MaxX-rect.MinX+1);
   tmli->tmli_Height=(UWORD)(rect.MaxY-rect.MinY+1);
  }
 }
}

/* Create an Image object */
struct TMObject *CreateTMObjectImage(struct TMHandle *handle, char *name,
                                     struct TagItem *tags)
{
 struct TMObjectImage *tmobj;
 struct Screen *icon_screen;
 struct TagItem *ti;
 struct TagItem *tstate;

 /* allocate memory for object */
 if (tmobj=(struct TMObjectImage *)
            AllocateTMObject(sizeof(struct TMObjectImage))) {
  icon_screen=NULL;

  /* Scan tag list */
  tstate=tags;
  while (ti=NextTagItem(&tstate)) {

   DEBUG_PRINTF("Got Tag (0x%08lx)\n",ti->ti_Tag);

   switch (ti->ti_Tag) {
    case TMOP_File:   tmobj->io_File=(char *) ti->ti_Data;
                      break;
    case TMOP_Data:   tmobj->io_Data=(struct TMImageData *) ti->ti_Data;
                      break;
    case TMOP_Screen: icon_screen=(struct Screen *) ti->ti_Data;
                      break;
   }
  }

  /* Sanity check */
  if (!tmobj->io_File && !tmobj->io_Data) tmobj->io_File=DefaultNoName;

  /* File name supplied? */
  if (tmobj->io_File) {
   /* Yes. Read image data from it */
   BPTR fl;

   /* Ignore user supplied data */
   tmobj->io_Data=NULL;

   /* Try to open as IFF file first */
   if (fl=Lock(tmobj->io_File,ACCESS_READ)) {
    /* Got it. */
    UnLock(fl);

    /* Read IFF file */
    if (tmobj->io_Data=ReadIFFData(tmobj->io_File)) {
     /* Got an IFF ILBM/ANIM */
     struct Image *img=&tmobj->io_Data->tmid_Normal;

     tmobj->io_XSize=img->Width;
     tmobj->io_YSize=img->Height+1;
     tmobj->io_Normal=img;

     /* Get selected image */
     img=&tmobj->io_Data->tmid_Selected;
     if (img->ImageData)
      tmobj->io_Selected=img;
    }
   }

   /* IFF failed: try datatypes.library (e.g. PNG, JPEG, GIF) when available */
   if (!(tmobj->io_Data) && (tmobj->io_Data=ReadImageViaDataTypes(tmobj->io_File))) {
    struct Image *img=&tmobj->io_Data->tmid_Normal;
    tmobj->io_XSize=img->Width;
    tmobj->io_YSize=img->Height+1;
    tmobj->io_Normal=img;
    img=&tmobj->io_Data->tmid_Selected;
    if (img->ImageData)
     tmobj->io_Selected=img;
   }

   /* Got IFF data? No --> Open icon file */
   if (!(tmobj->io_Data) && GetIconLibrary()) {
    if (IconVersion>=44) {
     /* V44: try GetIconTagList for palette-mapped icon and remap to screen */
     struct TagItem gettags[5];
     struct DiskObject *dobj;
     struct Rectangle rect;

     gettags[0].ti_Tag=ICONGETA_GetPaletteMappedIcon;
     gettags[0].ti_Data=TRUE;
     gettags[1].ti_Tag=ICONGETA_RemapIcon;
     gettags[1].ti_Data=TRUE;
     gettags[2].ti_Tag=ICONGETA_Screen;
     gettags[2].ti_Data=(ULONG)icon_screen;
     gettags[3].ti_Tag=TAG_DONE;
     gettags[3].ti_Data=0;

     dobj=GetIconTagList(tmobj->io_File,gettags);
     if (dobj) {
      tmobj->io_Data=(struct TMImageData *)dobj;
      tmobj->io_Flags|=IO_DiskObj|IO_UseDrawIconState;
      tmobj->io_Normal=NULL;
      tmobj->io_Selected=NULL;
      if (GetIconRectangleA(NULL,dobj,NULL,&rect,NULL)) {
       tmobj->io_XSize=(UWORD)(rect.MaxX-rect.MinX+1);
       tmobj->io_YSize=(UWORD)(rect.MaxY-rect.MinY+1);
      } else {
       tmobj->io_XSize=0;
       tmobj->io_YSize=0;
      }
     }
    }
    /* Fallback: legacy GetDiskObject when V44 path not used or failed */
    if (!(tmobj->io_Data) && (tmobj->io_Data=(struct TMImageData *)GetDiskObject(tmobj->io_File))) {
     struct Image *img=((struct DiskObject *) tmobj->io_Data)
                        ->do_Gadget.GadgetRender;

     tmobj->io_Flags|=IO_DiskObj;
     tmobj->io_XSize=img->Width;
     tmobj->io_YSize=img->Height+1;
     tmobj->io_Normal=img;
     tmobj->io_Selected=((struct DiskObject *) tmobj->io_Data)
                         ->do_Gadget.SelectRender;
    }
    if (!tmobj->io_Data)
     FreeIconLibrary();
   }

   /* All OK? */
   if (tmobj->io_Data) {
    tmobj->io_Flags|=IO_FreeData; /* Set free resources flag */
    return(tmobj);
   }
  }
  else {
   /* No. User supplied image data directly */
   struct Image *img=&tmobj->io_Data->tmid_Normal;

   tmobj->io_XSize=img->Width;
   tmobj->io_YSize=img->Height+1;
   tmobj->io_Normal=img;

   /* Get selected image */
   img=&tmobj->io_Data->tmid_Selected;
   if (img->ImageData)
    tmobj->io_Selected=img;

   /* All OK */
   return(tmobj);
  }
  FreeMem(tmobj,sizeof(struct TMObjectImage));
 }

 /* call failed */
 return(NULL);
}

/* Delete an Image object */
BOOL DeleteTMObjectImage(struct TMObjectImage *tmobj)
{
 DEBUG_PRINTF("Delete/Image (0x%08lx)\n",tmobj);

 /* Remove links */
 DeleteAllLinksTMObject((struct TMObject *) tmobj);

 /* Remove object from list */
 Remove((struct Node *) tmobj);

 /* Free resources? */
 if (tmobj->io_Flags&IO_FreeData)
  /* Yes, Icon file? */
  if (tmobj->io_Flags&IO_DiskObj) {
   /* Yes, release data */
   FreeDiskObject((struct DiskObject *) tmobj->io_Data);
   FreeIconLibrary();
  }
  else
   /* No, IFF data -> release it */
   FreeIFFData(tmobj->io_Data);

 /* Free object */
 FreeMem(tmobj,sizeof(struct TMObjectImage));

 /* All OK. */
 return(TRUE);
}

/* Allocate & Initialize a TMLinkImage structure */
struct TMLink *AllocLinkTMObjectImage(struct TMObjectImage *tmobj)
{
 struct TMLinkImage *tml;

 /* Allocate memory for link structure */
 if (tml=AllocMem(sizeof(struct TMLinkImage),MEMF_CLEAR|MEMF_PUBLIC)) {
  /* Initialize link structure */
  tml->tmli_Link.tml_Size=sizeof(struct TMLinkImage);
  tml->tmli_Width=tmobj->io_XSize;
  tml->tmli_Height=tmobj->io_YSize;
  tml->tmli_Normal=tmobj->io_Normal;
  tml->tmli_Selected=tmobj->io_Selected;
 }

 return(tml);
}

/* Send timer request */
static void SendTimerRequest(struct TMTimerReqImage *tmtri, BOOL last)
{
 struct timerequest *tr=&tmtri->tmtri_TimerReq.tmtr_Request;

 /* Initialize timer request */
 tr->tr_node.io_Command=TR_ADDREQUEST;
 tr->tr_time.tv_secs=last ? 1 : 0;
 tr->tr_time.tv_micro=last ? 0 : 33333;

 /* Send timer request */
 SendIO((struct IORequest *) tr);
}

/* Abort timer request */
static void AbortTimerRequest(struct TMTimerReqImage *tmtri)
{
 /* I/O request still active? */
 if (!CheckIO((struct IORequest *) tmtri))
  /* Yes. Abort it */
  AbortIO((struct IORequest *) tmtri);

 /* Remove request from port */
 WaitIO((struct IORequest *) tmtri);
}

/* Activate an Image object */
void ActivateTMObjectImage(struct TMLinkImage *tmli, void *start)
{
 struct TMObjectImage *tmobj=(struct TMObjectImage *)
                              tmli->tmli_Link.tml_Linked;
 UWORD x=tmli->tmli_LeftEdge;
 UWORD y=tmli->tmli_TopEdge;
 ULONG st=(ULONG)start;

 DEBUG_PRINTF("Activate/Image (%ld)\n",start);

 /* V44: palette-mapped icon path uses DrawIconStateA only (no planar animation) */
 if (tmobj->io_Flags&IO_UseDrawIconState) {
  ULONG ids_state=IDS_NORMAL;
  if (st==IOC_ACTIVE || st==IOC_FULLANIM) ids_state=IDS_SELECTED;
  if (st==NULL) {
   struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;
   if (tmtri) {
    WaitIO((struct IORequest *)tmtri);
    FreeMem(tmtri,sizeof(struct TMTimerReqImage));
    tmli->tmli_Link.tml_Active=NULL;
   }
  }
  DrawIconStateA(tmli->tmli_RastPort,(struct DiskObject *)tmobj->io_Data,
                 NULL,(LONG)x,(LONG)y,ids_state,NULL);
  return;
 }

 /* Start animation? */
 switch (st) {
  case NULL:         {
                      struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;
                      struct TMImageNode *tmin=tmtri->tmtri_NextImage;

                      /* Remove timer request */
                      WaitIO((struct IORequest *) tmtri);

                      /* Draw next picture? */
                      if (tmin) {
                       /* Yes. Draw next picture */
                       struct Image *img=&tmtri->tmtri_Image;

                       /* Set image pointer */
                       img->ImageData=tmin->tmin_Data;

                       /* Draw picture */
                       DrawImage(tmli->tmli_RastPort,img,x,y);

                       /* Set next picture */
                       tmtri->tmtri_NextImage=tmin->tmin_Next;

                       /* Activate timer */
                       SendTimerRequest(tmtri,tmtri->tmtri_NextImage==NULL);
                      } else {
                       /* No. Draw normal picture */
                       DrawImage(tmli->tmli_RastPort,tmli->tmli_Normal,x,y);

                       /* Free timer request */
                       FreeMem(tmtri,sizeof(struct TMTimerReqImage));
                       tmli->tmli_Link.tml_Active=NULL;
                      }
                     }
                     break;
  case IOC_DEACTIVE: {
                      struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;

                      /* Anim active? */
                      if (tmtri) {
                       /* Yes. Stop it! Abort timer request */
                       AbortTimerRequest(tmtri);

                       /* Free timer request */
                       FreeMem(tmtri,sizeof(struct TMTimerReqImage));
                       tmli->tmli_Link.tml_Active=NULL;
                      }

                      /* Draw inactive image */
                      DrawImage(tmli->tmli_RastPort,tmli->tmli_Normal,x,y);
                     }
                     break;
  case IOC_ACTIVE:   {
                      struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;
                      struct RastPort *rp=tmli->tmli_RastPort;

                      /* Anim active? */
                      if (tmtri) {
                       /* Yes. Stop it! Abort timer request */
                       AbortTimerRequest(tmtri);

                       /* Draw inactive state */
                       DrawImage(rp,tmli->tmli_Normal,x,y);

                       /* Free timer request */
                       FreeMem(tmtri,sizeof(struct TMTimerReqImage));
                       tmli->tmli_Link.tml_Active=NULL;
                      }

                      /* Draw selected state. Got a selected image? */
                      if (tmobj->io_Selected)
                       /* Yes, draw it */
                       DrawImage(rp,tmobj->io_Selected,x,y);
                      else {
                       /* No, invert image */
                       SetDrMd(rp,COMPLEMENT);
                       SetAPen(rp,0xff);
                       RectFill(rp,x,y,x+tmobj->io_XSize-1,y+tmobj->io_YSize-2);
                      }
                     }
                     break;
  case IOC_FULLANIM: {
                      struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;
                      struct RastPort *rp=tmli->tmli_RastPort;

                      /* Anim active? */
                      if (tmtri) {
                       /* Yes. Stop it! Abort timer request */
                       AbortTimerRequest(tmtri);

                       /* Draw inactive state */
                       DrawImage(rp,tmli->tmli_Normal,x,y);

                       /* Free timer request */
                       FreeMem(tmtri,sizeof(struct TMTimerReqImage));
                       tmli->tmli_Link.tml_Active=NULL;
                      }

                      /* Start animation after 1st image. Get memory. */
                      if (tmtri=AllocMem(sizeof(struct TMTimerReqImage),
                                         MEMF_PUBLIC)) {
                       /* Got it */
                       struct Image *img=tmobj->io_Selected;

                       /* Init timer request */
                       tmtri->tmtri_TimerReq.tmtr_Request=*deftimereq;
                       tmtri->tmtri_Image=*(tmli->tmli_Normal);
                       tmtri->tmtri_TimerReq.tmtr_Link=(struct TMLink *) tmli;

                       /* No icon file and more than one picture? */
                       if (!(tmobj->io_Flags & IO_DiskObj) && img)
                        /* Yes. Set anim starting image */
                        tmtri->tmtri_NextImage=tmobj->io_Data->tmid_Data->
                                                tmin_Next->tmin_Next;
                       else
                        /* Only one picture animation */
                        tmtri->tmtri_NextImage=NULL;

                       /* Got a second picture? */
                       if (img)
                        /* Yes, draw it */
                        DrawImage(rp,img,x,y);
                       else {
                        /* No, invert image */
                        SetDrMd(rp,COMPLEMENT);
                        SetAPen(rp,0xff);
                        RectFill(rp,x,y,x+tmobj->io_XSize-1,
                                 y+tmobj->io_YSize-2);
                       }

                       /* Send timer request */
                       SendTimerRequest(tmtri,FALSE);

                       /* Set active flag */
                       tmli->tmli_Link.tml_Active=tmtri;
                      }
                     }
                     break;
  case IOC_CONTANIM: {
                      struct TMTimerReqImage *tmtri=tmli->tmli_Link.tml_Active;

                      /* Anim active? */
                      if (tmtri) {
                       /* Yes. Stop it! Abort timer request */
                       AbortTimerRequest(tmtri);

                       /* Free timer request */
                       FreeMem(tmtri,sizeof(struct TMTimerReqImage));
                       tmli->tmli_Link.tml_Active=NULL;
                      }

                      /* Start animation after 2nd image */
                      /* Icon file or only one picture? */
                      if ((tmobj->io_Flags & IO_DiskObj) ||
                          !tmobj->io_Selected)
                       /* Yes. Draw normal state */
                       DrawImage(tmli->tmli_RastPort,tmli->tmli_Normal,x,y);
                      else {
                       struct TMImageNode *tmin=tmobj->io_Data->tmid_Data->
                                                 tmin_Next->tmin_Next;

                       /* Got more than 2 images and can we allocate memory */
                       /* for timer request? */
                       if (tmin && (tmtri=AllocMem(
                                           sizeof(struct TMTimerReqImage),
                                           MEMF_PUBLIC))) {
                        /* Yes, start anim. Init timer request */
                        tmtri->tmtri_TimerReq.tmtr_Request=*deftimereq;
                        tmtri->tmtri_Image=*(tmli->tmli_Normal);
                        tmtri->tmtri_TimerReq.tmtr_Link=(struct TMLink *) tmli;
                        tmtri->tmtri_NextImage=tmin;

                        /* Send timer request */
                        SendTimerRequest(tmtri,FALSE);

                        /* Set active flag */
                        tmli->tmli_Link.tml_Active=tmtri;
                       } else
                        /* No. Draw normal state */
                        DrawImage(tmli->tmli_RastPort,tmli->tmli_Normal,x,y);
                      }
                     }
                     break;
 }
}
