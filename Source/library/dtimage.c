/*
 * dtimage.c  V1.0
 *
 * Optional datatypes.library support for loading images (e.g. PNG, JPEG, GIF)
 * for use as dock icons. When datatypes.library is available, any picture
 * format supported by the system is decoded in memory; we copy the planar
 * bitmap from the picture object into our TMImageData layout (same as native
 * ILBM).
 *
 * (c) 1990-1996 Stefan Becker (ToolManager)
 * Datatypes integration (c) 2026 amigazen project
 */

#include "ToolManagerLib.h"

/* Datatypes library functions; resolved at link time. DataTypesBase must be set. */
extern Object *NewDTObjectA(APTR name, struct TagItem *attrs);
extern void DisposeDTObject(Object *obj);
extern ULONG GetDTAttrsA(Object *obj, struct TagItem *attrs);
extern ULONG SetDTAttrsA(Object *obj, struct Window *win, struct Requester *req, struct TagItem *attrs);
extern ULONG DoDTMethodA(Object *obj, struct Window *win, struct Requester *req, Msg msg);

/* Minimal datatypes constants (match datatypes.library / pictureclass.h). */
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((((ULONG)(a))<<24)|(((ULONG)(b))<<16)|(((ULONG)(c))<<8)|((ULONG)(d)))
#endif

#define GID_PICTURE        MAKE_ID('P','I','C','T')
#define DTA_GroupID        (0x80000000UL + 0UL)
#define DTM_PROCLAYOUT     0x404UL
#define PDTA_BitMap        (0x80000000UL + 4UL)
#define PDTA_Remap         (0x80000000UL + 9UL)

/* DTM_PROCLAYOUT message (intuition/gadgetclass.h struct gpLayout). */
struct gpLayoutMsg {
	ULONG MethodID;
	APTR  gpl_GInfo;
	ULONG gpl_Initial;
};

/* Global so datatypes.library stubs can use it when opened here. */
struct Library *DataTypesBase = NULL;

static BOOL OpenDataTypesLib(void)
{
	if (DataTypesBase)
		return TRUE;
	DataTypesBase = OpenLibrary("datatypes.library", 39);
	return (DataTypesBase != NULL);
}

/*
 * Load an image via datatypes as a picture, copy its planar bitmap into
 * a newly allocated TMImageData (same layout as ReadIFFData). Returns
 * TMImageData on success (caller frees with FreeIFFData), NULL otherwise.
 */
struct TMImageData *ReadImageViaDataTypes(char *name)
{
	struct TMImageData *tmid = NULL;
	struct TMImageNode *tmin = NULL;
	Object *obj = NULL;
	struct BitMap *bm = NULL;
	struct TagItem tags[3];
	struct TagItem getTags[4];
	struct TagItem setTags[3];
	ULONG bpr;
	ULONG rows;
	ULONG depth;
	ULONG planeoff;
	ULONG size;
	ULONG i;
	struct gpLayoutMsg gpl;

	if (!name || !name[0])
		return NULL;
	if (!OpenDataTypesLib())
		return NULL;

	tags[0].ti_Tag = DTA_GroupID;
	tags[0].ti_Data = GID_PICTURE;
	tags[1].ti_Tag = TAG_DONE;
	tags[1].ti_Data = 0;
	obj = NewDTObjectA((APTR)name, tags);
	if (!obj)
		return NULL;

	/* Some picture datatypes need DTM_PROCLAYOUT before exposing PDTA_BitMap. */
	getTags[0].ti_Tag = PDTA_BitMap;
	getTags[0].ti_Data = (ULONG)(APTR)&bm;
	getTags[1].ti_Tag = TAG_DONE;
	getTags[1].ti_Data = 0;
	if (GetDTAttrsA(obj, getTags) == 0 || !bm) {
		setTags[0].ti_Tag = PDTA_Remap;
		setTags[0].ti_Data = (ULONG)FALSE;
		setTags[1].ti_Tag = TAG_DONE;
		setTags[1].ti_Data = 0;
		SetDTAttrsA(obj, NULL, NULL, setTags);
		gpl.MethodID = DTM_PROCLAYOUT;
		gpl.gpl_GInfo = NULL;
		gpl.gpl_Initial = 1;
		DoDTMethodA(obj, NULL, NULL, (Msg)&gpl);
		getTags[0].ti_Tag = PDTA_BitMap;
		getTags[0].ti_Data = (ULONG)(APTR)&bm;
		GetDTAttrsA(obj, getTags);
	}

	if (!bm) {
		DisposeDTObject(obj);
		return NULL;
	}

	bpr = (ULONG)(bm->BytesPerRow);
	rows = (ULONG)(bm->Rows);
	depth = (ULONG)(bm->Depth);
	if (depth == 0 || depth > 8) {
		DisposeDTObject(obj);
		return NULL;
	}
	planeoff = bpr * rows;
	size = planeoff * depth;

	tmid = AllocMem(sizeof(struct TMImageData), MEMF_PUBLIC | MEMF_CLEAR);
	if (!tmid) {
		DisposeDTObject(obj);
		return NULL;
	}
	tmin = AllocMem(sizeof(struct TMImageNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (!tmin) {
		FreeMem(tmid, sizeof(struct TMImageData));
		DisposeDTObject(obj);
		return NULL;
	}
	tmin->tmin_Data = AllocMem(size, MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR);
	if (!tmin->tmin_Data) {
		FreeMem(tmin, sizeof(struct TMImageNode));
		FreeMem(tmid, sizeof(struct TMImageData));
		DisposeDTObject(obj);
		return NULL;
	}

	/* Copy planar data from datatype BitMap into our buffer (plane 0, then 1, ...). */
	for (i = 0; i < depth; i++) {
		if (bm->Planes[i])
			CopyMem((APTR)bm->Planes[i], (APTR)((char *)tmin->tmin_Data + i * planeoff), (long)planeoff);
	}

	/* Width in pixels from BytesPerRow (same formula as ILBM: BPR = (((w+15)>>4)<<1) => w = bpr<<3 ). */
	tmid->tmid_Normal.Width = (UWORD)(bpr << 3);
	tmid->tmid_Normal.Height = (UWORD)rows;
	tmid->tmid_Normal.Depth = (UBYTE)depth;
	tmid->tmid_Normal.PlanePick = (UWORD)((1L << depth) - 1);
	tmid->tmid_Normal.ImageData = tmin->tmin_Data;
	tmid->tmid_Selected = tmid->tmid_Normal;
	tmid->tmid_Selected.ImageData = tmin->tmin_Data;
	tmid->tmid_Data = tmin;
	tmin->tmin_Next = NULL;

	DisposeDTObject(obj);
	return tmid;
}
