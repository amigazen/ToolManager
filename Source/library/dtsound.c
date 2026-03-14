/*
 * dtsound.c  V1.0
 *
 * Optional datatypes.library support for loading sounds (e.g. WAV, AIFF, MP3)
 * and converting to 8SVX-style data in memory. When a sound is activated,
 * we try to load the file via datatypes (GID_SOUND), copy VoiceHeader and
 * sample into our buffer, then play via audio.device. 
 *
 * (c) 1990-1996 Stefan Becker (ToolManager)
 * Datatypes sound integration (c) 2026 amigazen project
 */

#include "ToolManagerLib.h"

/* Shared with dtimage.c */
extern struct Library *DataTypesBase;

/* Open datatypes if not already open (same global as dtimage). */
static BOOL OpenDataTypesLib(void)
{
	if (DataTypesBase)
		return TRUE;
	DataTypesBase = OpenLibrary("datatypes.library", 39);
	return (DataTypesBase != NULL);
}

/*
 * In-memory 8SVX-style sound: VoiceHeader + sample buffer (caller frees with
 * FreeSoundData). Used only inside the library.
 */
struct TMSoundData {
	struct VoiceHeader vh;
	ULONG sampleLen;
	UBYTE *sample;
};

/*
 * Load a sound file via datatypes (GID_SOUND), copy VoiceHeader and sample
 * into newly allocated TMSoundData. Returns NULL on failure or if datatypes
 * unavailable. Caller must call FreeSoundData on success.
 */
struct TMSoundData *ReadSoundViaDataTypes(char *name)
{
	struct TMSoundData *tsd = NULL;
	Object *obj = NULL;
	struct VoiceHeader *svh = NULL;
	UBYTE *sample = NULL;
	ULONG bytelen = 0;
	ULONG size = 0;
	struct TagItem tags[3];

	if (!name || !name[0])
		return NULL;
	if (!OpenDataTypesLib())
		return NULL;

	tags[0].ti_Tag = DTA_GroupID;
	tags[0].ti_Data = GID_SOUND;
	tags[1].ti_Tag = TAG_DONE;
	tags[1].ti_Data = 0;
	obj = NewDTObjectA((APTR)name, tags);
	if (!obj)
		return NULL;

	if (GetDTAttrs(obj,
			SDTA_VoiceHeader, (ULONG)(APTR)&svh,
			SDTA_Sample, (ULONG)(APTR)&sample,
			SDTA_SampleLength, (ULONG)(APTR)&bytelen,
			TAG_DONE) < 3 || !svh || !sample || bytelen == 0) {
		DisposeDTObject(obj);
		return NULL;
	}

	tsd = AllocMem(sizeof(struct TMSoundData), MEMF_PUBLIC | MEMF_CLEAR);
	if (!tsd) {
		DisposeDTObject(obj);
		return NULL;
	}
	/* audio.device CMD_WRITE requires even length (2-131072). */
	size = bytelen;
	if (size < 2)
		size = 2;
	if ((size & 1) != 0)
		size++;
	tsd->sampleLen = size;
	tsd->sample = AllocMem(size, MEMF_PUBLIC | MEMF_CHIP);
	if (!tsd->sample) {
		FreeMem(tsd, sizeof(struct TMSoundData));
		DisposeDTObject(obj);
		return NULL;
	}
	tsd->vh = *svh;
	CopyMem((APTR)sample, (APTR)tsd->sample, (long)bytelen);
	DisposeDTObject(obj);
	return tsd;
}

void FreeSoundData(struct TMSoundData *tsd)
{
	if (!tsd)
		return;
	if (tsd->sample)
		FreeMem(tsd->sample, tsd->sampleLen);
	FreeMem(tsd, sizeof(struct TMSoundData));
}

/* audio.device CMD_WRITE and flags (devices/audio.h). */
#define AUDIONAME "audio.device"
#define CMD_WRITE  2
#define ADIOF_PERVOL 0x80

/* Minimal IOAudio for CMD_WRITE (extends IORequest). */
struct IOAudio {
	struct IORequest ioa_Request;
	WORD  ioa_AllocKey;
	UBYTE *ioa_Data;
	ULONG ioa_Length;
	UWORD ioa_Period;
	UWORD ioa_Volume;
	UWORD ioa_Cycles;
};

/*
 * Play 8SVX-style sound via audio.device (channel 0, one shot). Sample must
 * be chip RAM (TMSoundData uses MEMF_CHIP). Blocks until playback completes.
 */
void PlaySoundData(struct TMSoundData *tsd)
{
	struct IORequest *io;
	struct IOAudio *ioa;
	struct MsgPort *port;
	ULONG period;
	UWORD vol;

	if (!tsd || !tsd->sample || tsd->sampleLen == 0)
		return;
	port = CreateMsgPort();
	if (!port)
		return;
	io = (struct IORequest *)AllocMem(sizeof(struct IOAudio), MEMF_PUBLIC | MEMF_CLEAR);
	if (!io) {
		DeleteMsgPort(port);
		return;
	}
	io->io_Message.mn_ReplyPort = port;
	io->io_Message.mn_Length = sizeof(struct IOAudio);
	io->io_Command = CMD_WRITE;
	io->io_Flags = ADIOF_PERVOL;
	io->io_Unit = 1;  /* channel 0 */
	if (OpenDevice((STRPTR)AUDIONAME, 0, io, 0) != 0) {
		FreeMem(io, sizeof(struct IOAudio));
		DeleteMsgPort(port);
		return;
	}
	ioa = (struct IOAudio *)io;
	ioa->ioa_Data = tsd->sample;
	ioa->ioa_Length = tsd->sampleLen;
	period = (ULONG)tsd->vh.vh_SamplesPerSec;
	if (period == 0)
		period = 22050;
	/* Period = 3579545 / rate (PAL); clamp to valid range 124..65536. */
	period = 3579545UL / period;
	if (period < 124)
		period = 124;
	if (period > 65536)
		period = 65536;
	ioa->ioa_Period = (UWORD)period;
	vol = (UWORD)(tsd->vh.vh_Volume & 0x7F);
	if (vol > 64)
		vol = 64;
	ioa->ioa_Volume = vol;
	ioa->ioa_Cycles = 1;
	DoIO(io);
	CloseDevice(io);
	FreeMem(io, sizeof(struct IOAudio));
	DeleteMsgPort(port);
}
