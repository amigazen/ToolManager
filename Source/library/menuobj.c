/*
 * menuobj.c  V2.1
 *
 * TMObject, Type: Menu
 *
 * (c) 1990-1993 Stefan Becker
 */

#include "ToolManagerLib.h"

/* V44/V45 AddAppMenuItemA tags; define if not in workbench.h (e.g. OS3 NDK) */
#ifndef WBAPPMENUA_CommandKeyString
#define WBAPPMENUA_CommandKeyString (TAG_USER + 0x87000001)
#endif
#ifndef WBAPPMENUA_GetKey
#define WBAPPMENUA_GetKey           (TAG_USER + 0x87000002)
#endif
#ifndef WBAPPMENUA_GetTitleKey
#define WBAPPMENUA_GetTitleKey      (TAG_USER + 0x87000003)
#endif
#ifndef WBAPPMENUA_UseKey
#define WBAPPMENUA_UseKey           (TAG_USER + 0x87000004)
#endif

#define MAX_MENU_TITLE_CACHE 27

/* Cache of menu strip title string -> key for WBAPPMENUA_UseKey (V45) */
struct MenuTitleCache {
  char  *mtc_Title;
  ULONG  mtc_Key;
};
static struct MenuTitleCache MenuTitleCache[MAX_MENU_TITLE_CACHE];
static LONG MenuTitleCacheCount=0;

/* extended TMObject structure for TMOBJTYPE_MENU objects */
struct TMObjectMenu {
                     struct TMObject  mo_Object;
                     struct TMLink   *mo_ExecLink;
                     struct TMLink   *mo_SoundLink;
                     struct TMLink    mo_Link;
                     void            *mo_AppMenu;
                     BOOL             mo_IsSeparator;  /* no Exec/Sound; bar only */
                    };

/* Return cached key for menu title, or 0 if not found. */
static ULONG GetMenuTitleKey(const char *title)
{
  LONG i;
  if (!title || !*title) return 0;
  for (i=0; i<MenuTitleCacheCount; i++)
    if (MenuTitleCache[i].mtc_Title && !strcmp(MenuTitleCache[i].mtc_Title,title))
      return MenuTitleCache[i].mtc_Key;
  return 0;
}

/* Create a new menu strip title and cache its key (V45). Returns key or 0. */
static ULONG AddMenuTitleAndCache(const char *title)
{
  struct TagItem titags[4];
  ULONG key;
  char *dup;
  if (!WBHaveTitleKey || MenuTitleCacheCount>=MAX_MENU_TITLE_CACHE) return 0;
  key=0;
  titags[0].ti_Tag =WBAPPMENUA_GetTitleKey;
  titags[0].ti_Data=(ULONG)&key;
  titags[1].ti_Tag =TAG_DONE;
  titags[1].ti_Data=0;
  if (!AddAppMenuItemA(0,0,(STRPTR)title,AppMsgPort,titags)) return 0;
  if (key==0) return 0;
  dup=strdup(title);
  if (!dup) return key;
  MenuTitleCache[MenuTitleCacheCount].mtc_Title=dup;
  MenuTitleCache[MenuTitleCacheCount].mtc_Key=key;
  MenuTitleCacheCount++;
  return key;
}

/* Resolve use_key for this item: ParentKey if set, else title key if MenuTitle set. */
static ULONG ResolveMenuUseKey(STRPTR menuTitle, ULONG parentKey)
{
  ULONG k;
  if (parentKey!=0) return parentKey;
  if (!menuTitle || !*menuTitle) return 0;
  k=GetMenuTitleKey(menuTitle);
  if (k!=0) return k;
  return AddMenuTitleAndCache(menuTitle);
}

/* Create a Menu object */
struct TMObject *CreateTMObjectMenu(struct TMHandle *handle, char *name,
                                    struct TagItem *tags)
{
  struct TagItem *ti;
  struct TagItem *tstate;
  STRPTR menuTitle=NULL;
  ULONG parentKey=0;
  BOOL isSubmenuParent=FALSE;
  BOOL isSeparator=FALSE;
  STRPTR commandKey=NULL;
  ULONG *submenuKeyPtr=NULL;
  ULONG useKey;
  struct TagItem wbTags[10];
  ULONG wbKey;
  LONG n;

  /* Open Workbench */
  if (!GetWorkbench()) return(NULL);

  tstate=tags;
  while (ti=NextTagItem(&tstate)) {
    switch (ti->ti_Tag) {
     case TMOP_MenuTitle:       menuTitle=(STRPTR)ti->ti_Data; break;
     case TMOP_ParentKey:       parentKey=ti->ti_Data; break;
     case TMOP_IsSubmenuParent: isSubmenuParent=(ti->ti_Data!=0); break;
     case TMOP_IsSeparator:     isSeparator=(ti->ti_Data!=0); break;
     case TMOP_CommandKey:      commandKey=(STRPTR)ti->ti_Data; break;
     case TMOP_SubmenuKeyPtr:   submenuKeyPtr=(ULONG *)ti->ti_Data; break;
     default: break;
    }
  }

  if (submenuKeyPtr) *submenuKeyPtr=0;

  {
   struct TMObjectMenu *tmobj;

   if (!(tmobj=(struct TMObjectMenu *)AllocateTMObject(sizeof(struct TMObjectMenu))))
    goto fail_wb;

   tmobj->mo_IsSeparator=isSeparator;

   tstate=tags;
   while (ti=NextTagItem(&tstate)) {
    DEBUG_PRINTF("Got Tag (0x%08lx)\n",ti->ti_Tag);
    switch (ti->ti_Tag) {
     case TMOP_Exec:
      if (ti->ti_Data && !isSeparator) {
       if (tmobj->mo_ExecLink) RemLinkTMObject(tmobj->mo_ExecLink);
       tmobj->mo_ExecLink=AddLinkTMObject(handle,(char *)ti->ti_Data,
                                          TMOBJTYPE_EXEC,(struct TMObject *)tmobj);
      }
      break;
     case TMOP_Sound:
      if (ti->ti_Data && !isSeparator) {
       if (tmobj->mo_SoundLink) RemLinkTMObject(tmobj->mo_SoundLink);
       tmobj->mo_SoundLink=AddLinkTMObject(handle,(char *)ti->ti_Data,
                                           TMOBJTYPE_SOUND,(struct TMObject *)tmobj);
      }
      break;
     default: break;
    }
   }

   useKey=0;
   wbKey=0;
   n=0;

   if (WBHaveAppMenuTags) {
    useKey=ResolveMenuUseKey(menuTitle,parentKey);
    if (useKey!=0) {
     wbTags[n].ti_Tag =WBAPPMENUA_UseKey;
     wbTags[n].ti_Data=useKey;
     n++;
    }
    if (isSubmenuParent) {
     wbTags[n].ti_Tag =WBAPPMENUA_GetKey;
     wbTags[n].ti_Data=(ULONG)&wbKey;
     n++;
    }
    if (commandKey && *commandKey) {
     wbTags[n].ti_Tag =WBAPPMENUA_CommandKeyString;
     wbTags[n].ti_Data=(ULONG)commandKey;
     n++;
    }
   }
   wbTags[n].ti_Tag =TAG_DONE;
   wbTags[n].ti_Data=0;
   n++;

   {
    STRPTR label;
    void *ami;
    if (isSeparator) label="---";
    else label=name;
    ami=AddAppMenuItemA((ULONG)&tmobj->mo_Link,NULL,label,AppMsgPort,
                        WBHaveAppMenuTags && n>1 ? wbTags : NULL);
    if (!ami) {
     if (tmobj->mo_ExecLink) RemLinkTMObject(tmobj->mo_ExecLink);
     if (tmobj->mo_SoundLink) RemLinkTMObject(tmobj->mo_SoundLink);
     FreeMem(tmobj,sizeof(struct TMObjectMenu));
     goto fail_wb;
    }
    tmobj->mo_AppMenu=ami;
   }

   if (submenuKeyPtr && wbKey!=0) *submenuKeyPtr=wbKey;

   tmobj->mo_Link.tml_Linked=(struct TMObject *)tmobj;
   FreeWorkbench();
   return((struct TMObject *)tmobj);
  }

fail_wb:
  FreeWorkbench();
  return(NULL);
}

/* Delete a Menu object */
BOOL DeleteTMObjectMenu(struct TMObjectMenu *tmobj)
{
 DEBUG_PRINTF("Delete/Menu (0x%08lx)\n",tmobj);

 /* Free resources */
 SafeRemoveAppMenuItem(tmobj->mo_AppMenu,&tmobj->mo_Link);
 FreeWorkbench();

 /* Remove links */
 if (tmobj->mo_ExecLink) RemLinkTMObject(tmobj->mo_ExecLink);
 if (tmobj->mo_SoundLink) RemLinkTMObject(tmobj->mo_SoundLink);

 /* Remove object from list */
 Remove((struct Node *) tmobj);

 /* Free object */
 FreeMem(tmobj,sizeof(struct TMObjectMenu));

 /* All OK. */
 return(TRUE);
}

/* Change a Menu object */
BOOL ChangeTMObjectMenu(struct TMHandle *handle,
                        struct TMObjectMenu *tmobj,
                        struct TagItem *tags)
{
 struct TagItem *ti,*tstate;

 /* Scan tag list */
 tstate=tags;
 while (ti=NextTagItem(&tstate)) {

  DEBUG_PRINTF("Got Tag (0x%08lx)\n",ti->ti_Tag);

  switch (ti->ti_Tag) {
   case TMOP_Exec:  if (tmobj->mo_ExecLink) { /* Already got a link? */
                     /* Yep. Remove it! */
                     RemLinkTMObject(tmobj->mo_ExecLink);
                     tmobj->mo_ExecLink=NULL;
                    }

                    if (ti->ti_Data) {
                     /* Create new link to exec object */
                     tmobj->mo_ExecLink=AddLinkTMObject(handle,
                                                        (char *) ti->ti_Data,
                                                        TMOBJTYPE_EXEC,
                                                        (struct TMObject *)
                                                         tmobj);
                    }
                    break;
   case TMOP_Sound: if (tmobj->mo_SoundLink) { /* Already got a link? */
                     /* Yep. Remove it! */
                     RemLinkTMObject(tmobj->mo_SoundLink);
                     tmobj->mo_SoundLink=NULL;
                    }

                    if (ti->ti_Data) {
                     /* Create new link to exec object */
                     tmobj->mo_SoundLink=AddLinkTMObject(handle,
                                                         (char *) ti->ti_Data,
                                                         TMOBJTYPE_SOUND,
                                                         (struct TMObject *)
                                                          tmobj);
                    }
                    break;
  }
 }

 /* All OK. */
 return(TRUE);
}

/* Update link structures */
void DeleteLinkTMObjectMenu(struct TMLink *tml)
{
 struct TMObjectMenu *tmobj=(struct TMObjectMenu *) tml->tml_LinkedTo;

 /* Clear link */
 if (tml==tmobj->mo_ExecLink)
  tmobj->mo_ExecLink=NULL;
 else if (tml==tmobj->mo_SoundLink)
  tmobj->mo_SoundLink=NULL;
}

/* Activate a Menu object */
void ActivateTMObjectMenu(struct TMLink *tml, struct AppMessage *msg)
{
 struct TMObjectMenu *tmobj=(struct TMObjectMenu *) tml->tml_Linked;

 if (tmobj->mo_IsSeparator) return;  /* separator bar has no action */

 /* Activate Sound object */
 if (tmobj->mo_SoundLink) CallActivateTMObject(tmobj->mo_SoundLink,NULL);

 /* Activate Exec object */
 if (tmobj->mo_ExecLink) CallActivateTMObject(tmobj->mo_ExecLink,msg);
}
