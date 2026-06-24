/****************************************************************************
*
* adlib.c
*
* Copyright Ad Lib Inc, 1988, 1989
* Copyright (c) 1991-1992 Microsoft Corporation. All Rights Reserved.
* Copyright (c) 2026 Datajake.
*
***************************************************************************/

#include "adlib.h"

/***************************************************************************

internal function prototypes

***************************************************************************/

static void ChangePitch(ADLSYNTH *synth, uint8_t voice, uint16_t pitchBend);
static void SndSetAllPrm(ADLSYNTH *synth, uint8_t slot);
static void SndSAVEK(ADLSYNTH *synth, uint8_t slot);
static void SndSFeedFm(ADLSYNTH *synth, uint8_t slot);
static void SndSAttDecay(ADLSYNTH *synth, uint8_t slot);
static void SndSSusRelease(ADLSYNTH *synth, uint8_t slot);
static void SndWaveSelect(ADLSYNTH *synth, uint8_t slot);

/***************************************************************************

public data

***************************************************************************/

/* this table gives the offset of each slot within the chip. */
uint8_t offsetSlot[] = {
	0, 1, 2, 3, 4, 5,
	8, 9, 10, 11, 12, 13,
	16, 17, 18, 19, 20, 21
};

/* this table indicates if the slot is a modulator (0) or a carrier (1). */
uint8_t operSlot[] = {
	0, 0, 0, /* 1 2 3 */
	1, 1, 1, /* 4 5 6 */
	0, 0, 0, /* 7 8 9 */
	1, 1, 1, /* 10 11 12 */
	0, 0, 0, /* 13 14 15 */
	1, 1, 1, /* 16 17 18 */
};

/***************************************************************************

local data

***************************************************************************/

static uint8_t percMasks[] = {
	0x10, 0x08, 0x04, 0x02, 0x01 };

/* voice number associated with each slot (melodic mode only) */
static uint8_t voiceSlot[] = {
	0, 1, 2,
	0, 1, 2,
	3, 4, 5,
	3, 4, 5,
	6, 7, 8,
	6, 7, 8,
};

/* slot numbers for percussive voices
* (0 indicates that there is only one slot)
*/
static uint8_t slotPerc[][2] = {
	{12, 15}, /* Bass Drum */
	{16, 0}, /* SD */
	{14, 0}, /* TOM */
	{17, 0}, /* TOP-CYM */
	{13, 0} /* HH */
};

/* slot numbers as a function of the voice and the operator (melodic only) */
static uint8_t slotVoice[][2] = {
	{0, 3}, /* voice 0 */
	{1, 4}, /* 1 */
	{2, 5}, /* 2 */
	{6, 9}, /* 3 */
	{7, 10}, /* 4 */
	{8, 11}, /* 5 */
	{12, 15}, /* 6 */
	{13, 16}, /* 7 */
	{14, 17} /* 8 */
};

/****************************************************************************

macros

***************************************************************************/

#define GetLocPrm(synth, slot, prm) ((uint16_t)synth->paramSlot[slot][prm])

/***************************************************************************

public functions

***************************************************************************/

void SetSlotParam(ADLSYNTH *synth, uint8_t slot, uint8_t *param, uint8_t waveSel)
{
	int i;
	uint8_t *ptr;

	for (i = 0, ptr = &synth->paramSlot[slot][0]; i < NUMLOCPARAM - 1; i++)
	*ptr++ = *param++;
	*ptr = waveSel &= 0x3;
	SndSetAllPrm(synth, slot);
}

void SetFreq(ADLSYNTH *synth, uint8_t voice, uint8_t pitch, uint8_t keyOn)
{
	uint16_t FNum;
	uint8_t t1;

	/* remember the keyon and pitch of the voice */
	synth->voiceKeyOn[voice] = keyOn;
	synth->notePitch[voice] = pitch;

	pitch += synth->halfToneOffset[voice];
	if (pitch > 95)
	pitch = 95;

	/* get the FNum for the voice */
	FNum = * (synth->fNumFreqPtr[voice] + synth->noteMOD12[pitch]);

	/* output the FNum, KeyOn and Block values */
	SndOutput(synth, (uint8_t)(0xA0 | voice), (uint8_t)FNum); /* FNum bits 0 - 7 (D0 - D7) */
	t1 = (uint8_t)(keyOn ? 32 : 0); /* Key On (D5) */
	t1 += (synth->noteDIV12[pitch] << 2); /* Block (D2 - D4) */
	t1 += (0x3 & (FNum >> 8)); /* FNum bits 8 - 9 (D0 - D1) */
	SndOutput(synth, (uint8_t)(0xB0 | voice), t1);
}

void SndOutput(ADLSYNTH *synth, uint8_t addr, uint8_t dataVal)
{
	OPL3_WriteRegBuffered(&synth->chip, addr, dataVal);
}

void SndSAmVibRhythm(ADLSYNTH *synth)
{
	uint8_t t1;

	t1 = (uint8_t)(synth->amDepth ? 0x80 : 0);
	t1 |= synth->vibDepth ? 0x40 : 0;
	t1 |= synth->fPercussion ? 0x20 : 0;
	t1 |= synth->percBits;
	SndOutput(synth, 0xBD, t1);
}

void SndSNoteSel(ADLSYNTH *synth)
{
	SndOutput(synth, 0x08, (uint8_t)(synth->noteSel ? 64 : 0));
}

void SndSKslLevel(ADLSYNTH *synth, uint8_t slot)
{
	uint16_t t1;

	t1 = 63 - (GetLocPrm(synth, slot, prmLevel) & 0x3f); /* amplitude */
	t1 = synth->slotRelVolume[slot] * t1;
	t1 += t1 + MAXVOLUME; /* round off to 0.5 */
	t1 = 63 - t1 / (2 * MAXVOLUME);

	t1 |= GetLocPrm(synth, slot, prmKsl) << 6;
	SndOutput(synth, (uint8_t)(0x40 | offsetSlot[slot]), (uint8_t)t1);
}

void SetVoiceTimbre(ADLSYNTH *synth, uint8_t voice, OPERATOR *pOper0)
{
	uint8_t *prm0;
	uint8_t *prm1;
	uint8_t wave0;
	uint8_t wave1;
	uint8_t *wavePtr;

	prm0 = (uint8_t *)pOper0;
	prm1 = prm0 + NUMLOCPARAM - 1;
	wavePtr = prm0 + 2 * (NUMLOCPARAM - 1);
	wave0 = *wavePtr++;
	wave1 = *wavePtr;

	if (!synth->fPercussion || voice < BD) { /* melodic only */
		SetSlotParam(synth, slotVoice[voice][0], prm0, wave0);
		SetSlotParam(synth, slotVoice[voice][1], prm1, wave1);
	}
	else if (voice == BD) { /* bass drum */
		SetSlotParam(synth, slotPerc[0][0], prm0, wave0);
		SetSlotParam(synth, slotPerc[0][1], prm1, wave1);
	}
	else { /* percussion, 1 slot */
		SetSlotParam(synth, slotPerc[voice - BD][0], prm0, wave0);
	}
}

void SetVoicePitch(ADLSYNTH *synth, uint8_t voice, uint16_t pitchBend)
{
	if (!synth->fPercussion || voice <= BD) { /* melodic and bass drum voices */
		if (pitchBend > MAX_PITCH) 
		pitchBend = MAX_PITCH;
		ChangePitch(synth, voice, pitchBend);
		SetFreq(synth, voice, synth->notePitch[voice], synth->voiceKeyOn[voice]);
	}
}

void SetVoiceVolume(ADLSYNTH *synth, uint8_t voice, uint8_t volume)
{
	uint8_t slot;
	uint8_t *slots;

	if (volume > MAXVOLUME)
	volume = MAXVOLUME;

	if (!synth->fPercussion || voice <= BD) { /* melodic voice */
		slots = slotVoice[voice];
		synth->slotRelVolume[slots[1]] = volume;
		SndSKslLevel(synth, slots[1]);
		if (!GetLocPrm(synth, slots[0], prmFm)) {
			/* additive synthesis: set volume of first slot too */
			synth->slotRelVolume[slots[0]] = volume;
			SndSKslLevel(synth, slots[0]);
		}
	}
	else { /* percussive voice */
		slot = slotPerc[voice - BD][0];
		synth->slotRelVolume[slot] = volume;
		SndSKslLevel(synth, slot);
	}
}

void NoteOn(ADLSYNTH *synth, uint8_t voice, uint8_t pitch)
{
	/* adjust pitch for chip */
	if (pitch < (MID_C - CHIP_MID_C))
	pitch = 0;
	else
	pitch -= (MID_C - CHIP_MID_C);

	if (voice < BD || !synth->fPercussion) /* this is a melodic voice */
	SetFreq(synth, voice, pitch, 1);
	else { /* this is a percussive voice */
		if (voice == BD)
		SetFreq(synth, BD, pitch, 0);
		else if (voice == TOM) {
			/* for the last 4 percussions, only the TOM may change in frequency, which also modifies the SD */
			SetFreq(synth, TOM, pitch, 0);
			SetFreq(synth, SD, (uint8_t)(pitch + TOM_TO_SD), 0);
		}

		synth->percBits |= percMasks[voice - BD];
		SndSAmVibRhythm(synth);
	}
}

void NoteOff(ADLSYNTH *synth, uint8_t voice)
{
	if (!synth->fPercussion || voice < BD)
	SetFreq(synth, voice, synth->notePitch[voice], 0); /* shut off */
	else {
		synth->percBits &= ~percMasks[voice - BD];
		SndSAmVibRhythm(synth);
	}
}

/***************************************************************************

private functions

***************************************************************************/

static void ChangePitch(ADLSYNTH *synth, uint8_t voice, uint16_t pitchBend)
{
	int16_t t1;
	int16_t t2;
	int16_t delta;
	uint32_t dw;

	dw = (uint32_t)((int32_t)((int16_t)pitchBend - MID_PITCH) * synth->pitchRangeStep);
	t1 = (int16_t)((LOBYTE(HIWORD(dw)) << 8) | HIBYTE(LOWORD(dw))) >> 5;

	if (synth->oldt1 == t1) {
		synth->fNumFreqPtr[voice] = synth->oldPtr;
		synth->halfToneOffset[voice] = synth->oldHt;
	}

	else {
		if (t1 < 0) {
			t2 = NR_STEP_PITCH - 1 - t1;
			synth->oldHt = synth->halfToneOffset[voice] = -(t2 / NR_STEP_PITCH);
			delta = (t2 - NR_STEP_PITCH + 1) % NR_STEP_PITCH;
			if (delta)
			delta = NR_STEP_PITCH - delta;
		}
		else {
			synth->oldHt = synth->halfToneOffset[voice] = t1 / NR_STEP_PITCH;
			delta = t1 % NR_STEP_PITCH;
		}

		synth->oldPtr = synth->fNumFreqPtr[voice] = synth->fNumNotes[delta];
		synth->oldt1 = t1;
	}
}

static void SndSetAllPrm(ADLSYNTH *synth, uint8_t slot)
{
	SndSAmVibRhythm(synth);
	SndSNoteSel(synth);
	SndSKslLevel(synth, slot);
	SndSFeedFm(synth, slot);
	SndSAttDecay(synth, slot);
	SndSSusRelease(synth, slot);
	SndSAVEK(synth, slot);
	SndWaveSelect(synth, slot);
}

static void SndSAVEK(ADLSYNTH *synth, uint8_t slot)
{
	uint8_t t1;

	t1 = (uint8_t)(GetLocPrm(synth, slot, prmAm) ? 0x80 : 0);
	t1 += GetLocPrm(synth, slot, prmVib) ? 0x40 : 0;
	t1 += GetLocPrm(synth, slot, prmStaining) ? 0x20 : 0;
	t1 += GetLocPrm(synth, slot, prmKsr) ? 0x10 : 0;
	t1 += GetLocPrm(synth, slot, prmMulti) & 0xf;
	SndOutput(synth, (uint8_t)(0x20 | offsetSlot[slot]), t1);
}

static void SndSFeedFm(ADLSYNTH *synth, uint8_t slot)
{
	uint8_t t1;

	if (operSlot[slot])
	return;

	t1 = (uint8_t)(GetLocPrm(synth, slot, prmFeedBack) << 1);
	t1 |= GetLocPrm(synth, slot, prmFm) ? 0 : 1;
	SndOutput(synth, (uint8_t)(0xC0 | voiceSlot[slot]), t1);
}

static void SndSAttDecay(ADLSYNTH *synth, uint8_t slot)
{
	uint8_t t1;

	t1 = (uint8_t)(GetLocPrm(synth, slot, prmAttack) << 4);
	t1 |= GetLocPrm(synth, slot, prmDecay) & 0xf;
	SndOutput(synth, (uint8_t)(0x60 | offsetSlot[slot]), t1);
}

static void SndSSusRelease(ADLSYNTH *synth, uint8_t slot)
{
	uint8_t t1;

	t1 = (uint8_t)(GetLocPrm(synth, slot, prmSustain) << 4);
	t1 |= GetLocPrm(synth, slot, prmRelease) & 0xf;
	SndOutput(synth, (uint8_t)(0x80 | offsetSlot[slot]), t1);
}

static void SndWaveSelect(ADLSYNTH *synth, uint8_t slot)
{
	uint8_t wave;

	if (synth->modeWaveSel)
	wave = (uint8_t)(GetLocPrm(synth, slot, prmWaveSel) & 0x03);
	else
	wave = 0;
	SndOutput(synth, (uint8_t)(0xE0 | offsetSlot[slot]), wave);
}
