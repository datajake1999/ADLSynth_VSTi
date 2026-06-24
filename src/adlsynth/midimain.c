/****************************************************************************
*
* midimain.c
*
* Copyright (c) 1991-1992 Microsoft Corporation. All Rights Reserved.
* Copyright (c) 2026 Datajake.
*
***************************************************************************/

#include <string.h>
#include "adlib.h"

/***************************************************************************

internal function prototypes

***************************************************************************/

static void synthNoteOff(ADLSYNTH *synth, MIDIMSG msg);
static void synthNoteOn(ADLSYNTH *synth, MIDIMSG msg);
static void synthPitchBend(ADLSYNTH *synth, MIDIMSG msg);
static void synthControlChange(ADLSYNTH *synth, MIDIMSG msg);
static void synthProgramChange(ADLSYNTH *synth, MIDIMSG msg);

static uint8_t FindVoice(ADLSYNTH *synth, uint8_t note, uint8_t channel);
static uint8_t GetNewVoice(ADLSYNTH *synth, uint8_t note, uint8_t channel);
static void FreeVoice(ADLSYNTH *synth, uint8_t voice);

/***************************************************************************

local data

***************************************************************************/

static uint8_t loudervol[128] = {
	0, 0, 65, 65, 66, 66, 67, 67, /* 0 - 7 */
	68, 68, 69, 69, 70, 70, 71, 71, /* 8 - 15 */
	72, 72, 73, 73, 74, 74, 75, 75, /* 16 - 23 */
	76, 76, 77, 77, 78, 78, 79, 79, /* 24 - 31 */
	80, 80, 81, 81, 82, 82, 83, 83, /* 32 - 39 */
	84, 84, 85, 85, 86, 86, 87, 87, /* 40 - 47 */
	88, 88, 89, 89, 90, 90, 91, 91, /* 48 - 55 */
	92, 92, 93, 93, 94, 94, 95, 95, /* 56 - 63 */
	96, 96, 97, 97, 98, 98, 99, 99, /* 64 - 71 */
	100, 100, 101, 101, 102, 102, 103, 103, /* 72 - 79 */
	104, 104, 105, 105, 106, 106, 107, 107, /* 80 - 87 */
	108, 108, 109, 109, 110, 110, 111, 111, /* 88 - 95 */
	112, 112, 113, 113, 114, 114, 115, 115, /* 96 - 103 */
	116, 116, 117, 117, 118, 118, 119, 119, /* 104 - 111 */
	120, 120, 121, 121, 122, 122, 123, 123, /* 112 - 119 */
	124, 124, 125, 125, 126, 126, 127, 127}; /* 120 - 127 */


static char patchKeyOffset[] = {
	0, -12, 12, 0, 0, 12, -12, 0, 0, -24, /* 0 - 9 */
	0, 0, 0, 0, 0, 0, 0, 0, -12, 0, /* 10 - 19 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 20 - 29 */
	0, 0, 12, 12, 12, 0, 0, 12, 12, 0, /* 30 - 39 */
	-12, -12, 0, 12, -12, -12, 0, 12, 0, 0, /* 40 - 49 */
	-12, 0, 0, 0, 12, 12, 0, 0, 12, 0, /* 50 - 59 */
	0, 0, 12, 0, 0, 0, 12, 12, 0, 12, /* 60 - 69 */
	0, 0, -12, 0, -12, -12, 0, 0, -12, -12, /* 70 - 79 */
	0, 0, 0, 0, 0, -12, -19, 0, 0, -12, /* 80 - 89 */
	0, 0, 0, 0, 0, 0, -31, -12, 0, 12, /* 90 - 99 */
	12, 12, 12, 0, 12, 0, 12, 0, 0, 0, /* 100 - 109 */
	0, 12, 0, 0, 0, 0, 12, 12, 12, 0, /* 110 - 119 */
	0, 0, 0, 0, -24, -36, 0, 0}; /* 120 - 127 */


#define msg_ch msg.ch /* all messages */

#define msg_note msg.b1 /* noteoff(0x80),noteon(0x90),keypressure(0xA0) */
#define msg_controller msg.b1 /* controlchange(0xB0) */
#define msg_patch msg.b1 /* programchange(0xC0) */
#define msg_cpress msg.b1 /* channelpressure(0xD0) */
#define msg_lsb msg.b1 /* pitchbend(0xE0) */

#define msg_velocity msg.b2 /* noteoff(0x80), noteon(0x90) */
#define msg_kpress msg.b2 /* keypressure(0xA0) */
#define msg_value msg.b2 /* controlchange(0xB0) */
#define msg_unused msg.b2 /* programchange(0xC0), channelpressure(0xD0) */
#define msg_msb msg.b2 /* pitchbend(0xE0) */

/***************************************************************************

public functions

***************************************************************************/

int synthInit(ADLSYNTH *synth, uint32_t rate)
{
	memset(synth, 0, sizeof(ADLSYNTH));
	OPL3_Reset(&synth->chip, rate);
	if (!LoadPatchData(synth))
	{
		return 0;
	}
	synthReset(synth);
	return 1;
}

void synthMidiData(ADLSYNTH *synth, uint32_t dwData)
{
	uint8_t bType = dwData & 0xf0;
	uint8_t bChannel = dwData & 0x0f;
	uint8_t bParm1 = (dwData >> 8) & 0x7f;
	uint8_t bParm2 = (dwData >> 16) & 0x7f;
	MIDIMSG msg;

	switch (bType)
	{
	case STATUS_NOTEOFF:
		msg_ch = bChannel;
		msg_note = bParm1;
		msg_velocity = bParm2;
		synthNoteOff(synth, msg);
		break;
	case STATUS_NOTEON:
		msg_ch = bChannel;
		msg_note = bParm1;
		msg_velocity = bParm2;
		synthNoteOn(synth, msg);
		break;
	case STATUS_CONTROLCHANGE:
		msg_ch = bChannel;
		msg_controller = bParm1;
		msg_value = bParm2;
		synthControlChange(synth, msg);
		break;
	case STATUS_PROGRAMCHANGE:
		msg_ch = bChannel;
		msg_patch = bParm1;
		msg_unused = bParm2;
		synthProgramChange(synth, msg);
		break;
	case STATUS_PITCHBEND:
		msg_ch = bChannel;
		msg_lsb = bParm1;
		msg_msb = bParm2;
		synthPitchBend(synth, msg);
		break;
	}
}

void synthSysexData(ADLSYNTH *synth, uint8_t *buffer, uint32_t length)
{
	const uint8_t resetArray[6] = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
	if (buffer[0] != 0xF0 || buffer[length-1] != 0xF7)
	{
		return;
	}
	if (length == 6 && memcmp(&resetArray[0], buffer, 6) == 0)
	{
		synthReset(synth);
	}
}

void synthGeneratePCM(ADLSYNTH *synth, int16_t *buffer, uint32_t length)
{
	OPL3_GenerateStream(&synth->chip, buffer, length);
}

void synthAllNotesOff(ADLSYNTH *synth)
{
	uint8_t voice;
	MIDIMSG msg;

	for (voice = 0; voice < (uint8_t)NUMVOICES; voice++) {
		if (synth->voices[voice].alloc) {
			msg_ch = synth->voices[voice].channel;
			msg_note = synth->voices[voice].note;
			synthNoteOff(synth, msg);
		}
	}
}

void synthReset(ADLSYNTH *synth)
{
	uint8_t i;

	synthAllNotesOff(synth);
	ResetChip(synth);
	synth->oldt1 = -1;
	for (i = 0 ; i < NUMCHANNELS; i++)
	{
		if (i == DRUMKITCHANNEL)
		{
			synth->channels[i].patch = 129;
		}
		else
		{
			synth->channels[i].patch = 0;
		}
		synth->channels[i].wPitchBend = 0x2000;
	}
}

int synthActiveVoiceCount(ADLSYNTH *synth)
{
	int count = 0;
	int i;

	for (i = 0; i < NUMVOICES; i++)
	{
		if (synth->voices[i].alloc)
		{
			count++;
		}
	}
	return count;
}

int synthCurrentProgram(ADLSYNTH *synth, int channel)
{
	channel = channel & 0x0f;
	if (channel == DRUMKITCHANNEL)
	{
		return 0;
	}
	return synth->channels[channel].patch;
}

/***************************************************************************

private functions

***************************************************************************/

static void synthOctaveReg(ADLSYNTH *synth, MIDIMSG *pMsg)
{
	int iModNote;
	MIDIMSG msg = *pMsg;

	if ((msg_ch == DRUMKITCHANNEL) || (synth->channels[msg_ch].patch > 127))
	return; /* only affect normal melodic patches */

	iModNote = msg_note + patchKeyOffset[synth->channels[msg_ch].patch];
	if ((iModNote < 0) || (iModNote > 127))
	iModNote = msg_note;

	msg_note = (uint8_t) iModNote;
	*pMsg = msg; /* modify what was pointed to */
}

static void synthNoteOn(ADLSYNTH *synth, MIDIMSG msg)
{
	uint8_t voice;

	if (msg_velocity == 0) { /* 0 velocity means note off */
		synthNoteOff(synth, msg);
		return;
	}

	synthOctaveReg(synth, &msg); /* adjust key # to overcome patch octave diffs */

	/* hack to overcome how quiet this thing is in relation to wave output */
	msg_velocity = loudervol[msg_velocity];

	if (msg_ch == DRUMKITCHANNEL) { /* drum kit hardwired on channel 15 */
		if ((msg_note < FIRSTDRUMNOTE) || (msg_note > LASTDRUMNOTE))
		return;

		synth->channels[DRUMKITCHANNEL].patch = synth->drumpatch[msg_note - FIRSTDRUMNOTE].patch;
		msg_note = synth->drumpatch[msg_note - FIRSTDRUMNOTE].note;

		if ((voice = FindVoice(synth, msg_note, msg_ch)) != 0xFF)
		NoteOff(synth, voice);
		voice = GetNewVoice(synth, msg_note, msg_ch);
	}

	else {
		voice = FindVoice(synth, msg_note, msg_ch); /* voice already assigned? */
		if (voice == 0xff)
		voice = GetNewVoice(synth, msg_note, msg_ch); /* if not, get one */
		else
		NoteOff(synth, voice);
	}

	if (synth->voices[voice].volume != msg_velocity) { /* check if it's already set */
		SetVoiceVolume(synth, voice, msg_velocity);
		synth->voices[voice].volume = msg_velocity;
	}
	/* apply any pb for this channel */
	SetVoicePitch(synth, voice, synth->channels[msg_ch].wPitchBend);
	/* adjust for octave reg. */
	NoteOn(synth, voice, msg_note);
}

static void synthNoteOff(ADLSYNTH *synth, MIDIMSG msg)
{
	uint8_t voice;

	/* adjust key # to overcome patch octave differences */
	synthOctaveReg(synth, &msg); 

	if (msg_ch == DRUMKITCHANNEL) { /* drum kit hardwired on channel 15 */
		uint8_t bPatch;

		if ((msg_note < FIRSTDRUMNOTE) || (msg_note > LASTDRUMNOTE))
		return;

		bPatch = synth->drumpatch[msg_note - FIRSTDRUMNOTE].patch;
		msg_note = synth->drumpatch[msg_note - FIRSTDRUMNOTE].note;

		if ((voice = FindVoice(synth, msg_note, msg_ch)) != 0xFF) {
			if (LOWORD(synth->voices[voice].dwTimeStamp) == bPatch) {
				NoteOff(synth, voice);
				FreeVoice(synth, voice);
			}
		}
		return;
	}

	else {
		voice = FindVoice(synth, msg_note, msg_ch); /* get the assigned voice */
	}

	if (voice == 0xFF)
	return;

	/* turn the note off */

	if (synth->voices[voice].note) { /* check if note is playing */
		NoteOff(synth, voice);
		/* return note to pool of notes. */
		FreeVoice(synth, voice);
	}
}

static void synthPitchBend(ADLSYNTH *synth, MIDIMSG msg)
{
	uint8_t i;
	uint16_t wPB = (((uint16_t)msg_msb) << 7) | msg_lsb;

	/* msb is shifted by 7 because we've redefined the MIDI pitch bend range of 0 - 0x7f7f to 0 - 3fff by concatenating the two 7-bit values in msb and lsb together */

	for (i = 0; i < (uint8_t)NUMVOICES; i++) {
		if ((synth->voices[i].alloc) && (synth->voices[i].channel == msg_ch))
		SetVoicePitch(synth, i, wPB);
	}
	synth->channels[msg_ch].wPitchBend = wPB; /* remember for subsequent notes on this channel */
}

static void synthControlChange(ADLSYNTH *synth, MIDIMSG msg)
{
	if (msg_controller >= 123)
	synthAllNotesOff(synth);
}

static void synthProgramChange(ADLSYNTH *synth, MIDIMSG msg)
{
	uint8_t voice;

	/* drum kit hardwired on channel 15, so ignore patch changes there */
	if (msg_ch == DRUMKITCHANNEL)
	return;

	/* turn off any notes on this channel which are currently on */
	for (voice = 0; voice < (uint8_t)NUMVOICES; voice++) {
		if ((synth->voices[voice].alloc) && (synth->voices[voice].channel == msg_ch)
				&& (synth->voices[voice].note)) { /* check if note is playing */
			NoteOff(synth, voice); /* turn note off */
			FreeVoice(synth, voice); /* return note to pool of notes. */
		}
	}

	/* change the patch for this channel */
	synth->channels[msg_ch].patch = msg_patch;

}

static uint8_t FindVoice(ADLSYNTH *synth, uint8_t note, uint8_t channel)
{
	uint8_t i;

	if (channel == DRUMKITCHANNEL) {
		i = synth->patches[synth->channels[DRUMKITCHANNEL].patch].percVoice;

		if (synth->voices[i].alloc)
		return i;
	}

	else {
		for (i = 0; i < (uint8_t)NUMVOICES; i++) {
			if ((synth->voices[i].alloc) && (synth->voices[i].note == note)
					&& (synth->voices[i].channel == channel)) {
				synth->voices[i].dwTimeStamp = synth->dwAge++;
				return i;
			}
		}
	}

	return 0xFF; /* not found */
}

static uint8_t GetNewVoice(ADLSYNTH *synth, uint8_t note, uint8_t channel) 
{
	uint8_t i;
	uint8_t voice;
	uint8_t patch;
	uint8_t bVoiceToUse;
	uint32_t dwOldestTime = synth->dwAge; /* init to current "time" */

	/* get the patch in use for this channel */
	patch = synth->channels[channel].patch;

	if (synth->patches[patch].mode) { /* it's a percussive patch */
		voice = synth->patches[patch].percVoice; /* use fixed percussion voice */
		synth->voices[voice].alloc = 1;
		synth->voices[voice].note = note;
		synth->voices[voice].channel = channel;
		synth->voices[voice].dwTimeStamp = MAKELONG(patch, 0);
		SetVoiceTimbre(synth, voice, &synth->patches[patch].op0); /* set the timbre */
		return voice;
	}

	/* find a free melodic voice to use */
	for (i = 0; i < (uint8_t)NUMMELODIC; i++) { /* it's a melodic patch */
		if (!synth->voices[i].alloc) {
			bVoiceToUse = i; /* grab first unused one */
			break;
		}
		else if (synth->voices[i].dwTimeStamp < dwOldestTime) {
			dwOldestTime = synth->voices[i].dwTimeStamp;
			bVoiceToUse = i; /* remember oldest one to steal */
		}
	}

	/* at this point, we have either found an unused voice, */
	/* or have found the oldest one among a totally used voice bank */

	if (synth->voices[bVoiceToUse].alloc) /* if we stole it, turn it off */
	NoteOff(synth, bVoiceToUse);

	synth->voices[bVoiceToUse].alloc = 1;
	synth->voices[bVoiceToUse].note = note;
	synth->voices[bVoiceToUse].channel = channel;
	synth->voices[bVoiceToUse].dwTimeStamp = synth->dwAge++;

	SetVoiceTimbre(synth, bVoiceToUse, &synth->patches[patch].op0); /* set the timbre */

	return bVoiceToUse;
}

static void FreeVoice(ADLSYNTH *synth, uint8_t voice) 
{
	synth->voices[voice].alloc = 0;
}
