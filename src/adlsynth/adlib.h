/****************************************************************************
*
* adlib.h
*
* Copyright (c) 1991-1992 Microsoft Corporation. All Rights Reserved.
* Copyright (c) 2026 Datajake.
*
***************************************************************************/

#ifndef ADLIB_H
#define ADLIB_H

#include <stdint.h>
#include "../opl3/opl3.h"

#ifndef HIBYTE
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))
#endif
#ifndef LOBYTE
#define LOBYTE(w) ((uint8_t)(w))
#endif
#ifndef HIWORD
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((uint16_t)(l))
#endif
#ifndef MAKELONG
#define MAKELONG(a, b) ((long)(((uint16_t)(a)) | ((uint32_t)((uint16_t)(b))) << 16))
#endif

#define NUMVOICES (synth->fPercussion?11:9) /* number of voices we have */
#define NUMMELODIC (synth->fPercussion?6:9) /* number of melodic voices */
#define NUMPERCUSSIVE (synth->fPercussion?5:0) /* number of percussive voices */

#define NUMCHANNELS 16 /* number of MIDI channels */
#define DRUMKITCHANNEL 15 /* our drum kit channel */

#define MAXPATCH 180 /* max patch number (>127 for drums) */
#define MAXVOLUME 0x7f
#define NUMLOCPARAM 14 /* number of loc params per voice */

#define FIRSTDRUMNOTE 35
#define LASTDRUMNOTE 81
#define NUMDRUMNOTES (LASTDRUMNOTE - FIRSTDRUMNOTE + 1)

#define MAX_PITCH 0x3fff /* maximum pitch bend value */
#define MID_PITCH 0x2000 /* mid pitch bend value (no shift) */
#define NR_STEP_PITCH 25 /* steps per half-tone for pitch bend */
#define MID_C 60 /* MIDI standard mid C */
#define CHIP_MID_C 48 /* sound chip mid C */

#define RT_BANK 256
#define RT_DRUMKIT 257
#define DEFAULTBANK 1
#define DEFAULTDRUMKIT 1

/****************************************************************************/

#define STATUS_NOTEOFF 0x80
#define STATUS_NOTEON 0x90
#define STATUS_POLYPHONICKEY 0xa0
#define STATUS_CONTROLCHANGE 0xb0
#define STATUS_PROGRAMCHANGE 0xc0
#define STATUS_CHANNELPRESSURE 0xd0
#define STATUS_PITCHBEND 0xe0

/****************************************************************************

definitions of sound chip parameters

***************************************************************************/

/* parameters of each voice */
#define prmKsl 0 /* key scale level (KSL) - level controller */
#define prmMulti 1 /* frequency multiplier (MULTI) - oscillator */
#define prmFeedBack 2 /* modulation feedback (FB) - oscillator */
#define prmAttack 3 /* attack rate (AR) - envelope generator */
#define prmSustain 4 /* sustain level (SL) - envelope generator */
#define prmStaining 5 /* sustaining sound (SS) - envelope generator */
#define prmDecay 6 /* decay rate (DR) - envelope generator */
#define prmRelease 7 /* release rate (RR) - envelope generator */
#define prmLevel 8 /* output level (OL) - level controller */
#define prmAm 9 /* amplitude vibrato (AM) - level controller */
#define prmVib 10 /* frequency vibrator (VIB) - oscillator */
#define prmKsr 11 /* envelope scaling (KSR) - envelope generator */
#define prmFm 12 /* fm=0, additive=1 (FM) (operator 0 only) */
#define prmWaveSel 13 /* wave select */

/* global parameters */
#define prmAmDepth 14
#define prmVibDepth 15
#define prmNoteSel 16
#define prmPercussion 17

/* percussive voice numbers (0-5 are melodic voices, 12 op): */
#define BD 6 /* bass drum (2 op) */
#define SD 7 /* snare drum (1 op) */
#define TOM 8 /* tom-tom (1 op) */
#define CYMB 9 /* cymbal (1 op) */
#define HIHAT 10 /* hi-hat (1 op) */

/* In percussive mode, the last 4 voices (SD TOM HH CYMB) are created
* using melodic voices 7 & 8. A noise generator uses channels 7 & 8
* frequency information for creating rhythm instruments. Best results
* are obtained by setting TOM two octaves below mid C and SD 7 half-tones
* above TOM. In this implementation, only the TOM pitch may vary, with the
* SD always set 7 half-tones above.
*/
#define TOM_PITCH 24 /* best frequency, in range of 0 to 95 */
#define TOM_TO_SD 7 /* 7 half-tones between voice 7 & 8 */
#define SD_PITCH (TOM_PITCH + TOM_TO_SD)

/****************************************************************************

bank file support

***************************************************************************/

#define BANK_SIG_LEN 6
#define BANK_FILLER_SIZE 8

/* instrument bank file header */
typedef struct {
	char majorVersion;
	char minorVersion;
	char sig[BANK_SIG_LEN]; /* signature: "ADLIB-" */
	uint16_t nrDefined; /* number of list entries used */
	uint16_t nrEntry; /* number of total entries in list */
	int32_t offsetIndex; /* offset of start of list of names */
	int32_t offsetTimbre; /* offset of start of data */
	char filler[BANK_FILLER_SIZE]; /* must be zero */
} BANKHDR;

/****************************************************************************

typedefs

***************************************************************************/

typedef struct {
	uint8_t ksl; /* KSL */
	uint8_t freqMult; /* MULTI */
	uint8_t feedBack; /* FB */
	uint8_t attack; /* AR */
	uint8_t sustLevel; /* SL */
	uint8_t sustain; /* SS */
	uint8_t decay; /* DR */
	uint8_t release; /* RR */
	uint8_t output; /* OL */
	uint8_t am; /* AM */
	uint8_t vib; /* VIB */
	uint8_t ksr; /* KSR */
	uint8_t fm; /* FM */
} OPERATOR;

typedef struct {
	uint8_t mode; /* 0 = melodic, 1 = percussive */
	uint8_t percVoice; /* if mode == 1, voice number to be used */
	OPERATOR op0;
	OPERATOR op1;
	uint8_t wave0; /* waveform for operator 0 */
	uint8_t wave1; /* waveform for operator 1 */
} TIMBRE;

typedef struct {
	uint8_t patch; /* the patch to use */
	uint8_t note; /* the note to play */
} DRUMPATCH;

typedef struct {
	uint8_t key; /* the key to map */
	uint8_t patch; /* the patch to use */
	uint8_t note; /* the note to play */
} DRUMFILEPATCH;

typedef struct {
	uint8_t ch; /* channel number */
	uint8_t b1; /* first data byte */
	uint8_t b2; /* second data byte */
} MIDIMSG;

typedef struct {
	uint8_t alloc; /* is voice allocated? */
	uint8_t note; /* note that is currently being played */
	uint8_t channel; /* channel that it is being played on */
	uint8_t volume; /* current volume setting of voice */
	uint32_t dwTimeStamp; /* time voice was allocated */
} VOICE;

typedef struct {
	uint8_t patch; /* the patch on this channel */
	uint16_t wPitchBend;
} CHANNEL;

typedef struct {
	uint8_t slotRelVolume[18]; /* relative volume of slots */
	uint8_t percBits; /* control bits of percussive voices */
	uint8_t amDepth; /* chip global parameters ... */
	uint8_t vibDepth; /* ... */
	uint8_t noteSel; /* ... */
	uint8_t modeWaveSel; /* != 0 if used with 'wave-select' parms */
	int fPercussion; /* percussion mode parameter */
	int pitchRangeStep; /* == pitchRange * NR_STEP_PITCH */
	uint16_t fNumNotes[NR_STEP_PITCH] [12];
	uint16_t *fNumFreqPtr[11]; /* lines of fNumNotes table (one per voice) */
	int halfToneOffset[11]; /* one per voice */
	uint8_t noteDIV12[96]; /* table of (0..95) DIV 12 */
	uint8_t noteMOD12[96]; /* table of (0..95) MOD 12 */
	uint8_t notePitch[11]; /* pitch value for each voice (implicit 0 init) */
	uint8_t voiceKeyOn[11]; /* keyOn bit for each voice (implicit 0 init) */
	uint8_t paramSlot[18][NUMLOCPARAM]; /* all the parameters of slots... */
	TIMBRE patches[MAXPATCH]; /* melodic patch information */
	DRUMPATCH drumpatch[NUMDRUMNOTES]; /* drumkit patch information */
	int oldt1;
	int oldHt;
	uint16_t *oldPtr;
	VOICE voices[11]; /* 9 voices if melodic mode or 11 if percussive */
	uint32_t dwAge; /* voice relative age */
	CHANNEL channels[NUMCHANNELS];
	opl3_chip chip;
} ADLSYNTH;

/****************************************************************************

function prototypes

***************************************************************************/

/****************************************************************************

midimain.c

***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
int synthInit(ADLSYNTH *synth, uint32_t rate);
void synthMidiData(ADLSYNTH *synth, uint32_t dwData);
void synthSysexData(ADLSYNTH *synth, uint8_t *buffer, uint32_t length);
void synthGeneratePCM(ADLSYNTH *synth, int16_t *buffer, uint32_t length);
void synthAllNotesOff(ADLSYNTH *synth);
void synthReset(ADLSYNTH *synth);
int synthActiveVoiceCount(ADLSYNTH *synth);
int synthCurrentProgram(ADLSYNTH *synth, int channel);
#ifdef __cplusplus
}
#endif

/****************************************************************************

adlib.c

***************************************************************************/

extern uint8_t offsetSlot[]; /* offset of each slot */
extern uint8_t operSlot[]; /* operator (0 or 1) in each slot */
void SetSlotParam(ADLSYNTH *synth, uint8_t slot, uint8_t *param, uint8_t waveSel);
void SetFreq(ADLSYNTH *synth, uint8_t voice, uint8_t pitch, uint8_t keyOn);
void SndOutput(ADLSYNTH *synth, uint8_t addr, uint8_t dataVal);
void SndSAmVibRhythm(ADLSYNTH *synth);
void SndSNoteSel(ADLSYNTH *synth);
void SndSKslLevel(ADLSYNTH *synth, uint8_t slot);
void SetVoiceTimbre(ADLSYNTH *synth, uint8_t voice, OPERATOR *pOper0);
void SetVoicePitch(ADLSYNTH *synth, uint8_t voice, uint16_t pitchBend);
void SetVoiceVolume(ADLSYNTH *synth, uint8_t voice, uint8_t volume);
void NoteOn(ADLSYNTH *synth, uint8_t voice, uint8_t pitch);
void NoteOff(ADLSYNTH *synth, uint8_t voice);

/****************************************************************************

init.c

***************************************************************************/

int LoadPatchData(ADLSYNTH *synth);
void ResetChip(ADLSYNTH *synth);

#endif
