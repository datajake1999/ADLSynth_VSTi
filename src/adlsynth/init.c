/****************************************************************************
*
* init.c
*
* Copyright (c) 1991-1992 Microsoft Corporation. All Rights Reserved.
* Copyright (c) 2026 Datajake.
*
***************************************************************************/

#include "adlib.h"
#include "adlbank.h"
#include "adldrum.h"

/***************************************************************************

internal function prototypes

***************************************************************************/

static void SoundInit(ADLSYNTH *synth);
static void SetMode(ADLSYNTH *synth, uint8_t mode);
static void SetGParam(ADLSYNTH *synth, uint8_t amD, uint8_t vibD, uint8_t nSel);
static void Set3812(ADLSYNTH *synth, uint8_t state);
static void InitSlotVolume(ADLSYNTH *synth);
static void InitFNums(ADLSYNTH *synth);
static void SoundChut(ADLSYNTH *synth, uint8_t voice);
static void SetPitchRange(ADLSYNTH *synth, uint16_t pR);
static void SetFNum(uint16_t *fNumVec, int num, int den);
static int32_t CalcPremFNum(int numDeltaDemiTon, int denDeltaDemiTon);
static int LoadPatches(ADLSYNTH *synth);
static int LoadDrumPatches(ADLSYNTH *synth);

/***************************************************************************

private functions

***************************************************************************/

static void SoundInit(ADLSYNTH *synth)
{
	uint8_t i;

	SetGParam(synth, 0, 0, 0); /* init global parameters */
	InitSlotVolume(synth); /* sets volume of each slot to MAXVOLUME */
	InitFNums(synth); /* initializes frequency shift table to no shift */
	for (i = 0 ; i <= 8; i++)
	SoundChut(synth, i); /* set frequencies of voices 0 - 8 to 0 */
	SetMode(synth, 1); /* percussion mode (melodic mode == 0) */
	SetPitchRange(synth, 2); /* GMM pitch range is 2 semitones */
	Set3812(synth, 1); /* sets wave-select parameter */
}

static void SetMode(ADLSYNTH *synth, uint8_t mode)
{
	if (mode) {
		SetFreq(synth, TOM, TOM_PITCH, 0); /* set frequency of TOM voice */
		SetFreq(synth, SD, SD_PITCH, 0); /* set frequency of SD voice */
	}

	synth->fPercussion = mode;
	synth->percBits = 0; /* initialize control bits of percussive voices */
}

static void SetGParam(ADLSYNTH *synth, uint8_t amD, uint8_t vibD, uint8_t nSel)
{
	synth->amDepth = amD;
	synth->vibDepth = vibD;
	synth->noteSel = nSel;

	SndSAmVibRhythm(synth);
	SndSNoteSel(synth);
}

static void Set3812(ADLSYNTH *synth, uint8_t state)
{
	uint8_t i;

	/* set waveform for each of the 18 slots to sine wave */
	for (i = 0; i < 18; i++)
	SndOutput(synth, (uint8_t)(0xE0 | offsetSlot[i]), 0);

	/* enable/disable the wave-select parameters */
	synth->modeWaveSel = (uint8_t)(state ? 0x20 : 0);
	SndOutput(synth, 1, synth->modeWaveSel);
}

static void InitSlotVolume(ADLSYNTH *synth)
{
	int i;

	for (i = 0; i < 18; i++)
	synth->slotRelVolume[i] = MAXVOLUME;
}

static void InitFNums(ADLSYNTH *synth)
{
	uint16_t i, j, k;
	uint16_t num; /* numerator */
	uint16_t numStep; /* step value for numerator */
	uint16_t row; /* row in the frequency table */

	/* calculate each row in the fNumNotes table */
	numStep = 100 / NR_STEP_PITCH;
	for (num = row = 0; row < NR_STEP_PITCH; row++, num += numStep)
	SetFNum(synth->fNumNotes[row], num, 100);

	/* fNumFreqPtr has an element for each voice, pointing to the */
	/* appropriate row in the fNumNotes table. They're all initialized */
	/* to the first row, which represents no pitch shift. */
	for (i = 0; i < 11; i++) {
		synth->fNumFreqPtr[i] = synth->fNumNotes[0];
		synth->halfToneOffset[i] = 0;
	}

	/* just for optimization */
	for (i = 0, k = 0; i < 8; i++)
	for (j = 0; j < 12; j++, k++) {
		synth->noteDIV12[k] = (uint8_t)i;
		synth->noteMOD12[k] = (uint8_t)j;
	}
}

static void SoundChut(ADLSYNTH *synth, uint8_t voice)
{
	SndOutput(synth, (uint8_t)(0xA0 | voice), 0);
	SndOutput(synth, (uint8_t)(0xB0 | voice), 0);
}

static void SetPitchRange(ADLSYNTH *synth, uint16_t pR)
{
	if (pR > 12)
	pR = 12;
	if (pR < 1)
	pR = 1;
	synth->pitchRangeStep = pR * NR_STEP_PITCH;
}

static void SetFNum(uint16_t *fNumVec, int num, int den)
{
	int i;
	int32_t val;

	*fNumVec++ = (uint16_t)((4 + (val = CalcPremFNum(num, den))) >> 3);
	for (i = 1; i < 12; i++) {
		val *= 106;
		*fNumVec++ = (uint16_t)((4 + (val /= 100)) >> 3);
	}
}

static int32_t CalcPremFNum(int numDeltaDemiTon, int denDeltaDemiTon)
{
	int32_t f8;
	int32_t fNum8;
	int32_t d100;

	d100 = denDeltaDemiTon * 100;
	f8 = (d100 + 6 * numDeltaDemiTon) * (26044L * 2L);
	f8 /= d100 * 25;

	fNum8 = f8 * 16384;
	fNum8 *= 9L;
	fNum8 /= 179L * 625L;

	return fNum8;
}

static int LoadPatches(ADLSYNTH *synth)
{
	int iPatches;
	uint32_t dwOffset;
	TIMBRE *lpBankTimbre;
	TIMBRE *lpPatchTimbre;
	BANKHDR *lpBankHdr;

	/* read the bank data, loading patches as we find them */

	lpBankHdr = (BANKHDR *)ADLBank;
	dwOffset = lpBankHdr->offsetTimbre; /* point to first one */

	for (iPatches = 0; iPatches < MAXPATCH; iPatches++) {

		lpBankTimbre = (TIMBRE *)(ADLBank + dwOffset);
		lpPatchTimbre = &synth->patches[iPatches];
		*lpPatchTimbre = *lpBankTimbre;

		dwOffset += sizeof(TIMBRE);
		if (dwOffset + sizeof(TIMBRE) > sizeof(ADLBank)) {
			break;
		}
	}

	return iPatches;
}

static int LoadDrumPatches(ADLSYNTH *synth)
{
	int iPatches;
	int key;
	uint32_t dwOffset;
	DRUMFILEPATCH *lpPatch;

	/* read the drum data, loading patches as we find them */

	dwOffset = 0;
	for (iPatches = 0; iPatches < NUMDRUMNOTES; iPatches++) {

		lpPatch = (DRUMFILEPATCH *)(ADLDrum + dwOffset);
		key = lpPatch->key;
		if ((key >= FIRSTDRUMNOTE) && (key <= LASTDRUMNOTE)) {
			synth->drumpatch[key - FIRSTDRUMNOTE].patch = lpPatch->patch;
			synth->drumpatch[key - FIRSTDRUMNOTE].note = lpPatch->note;
		}

		dwOffset += sizeof(DRUMFILEPATCH);
		if (dwOffset + sizeof(DRUMFILEPATCH) > sizeof(ADLDrum)) {
			break;
		}
	}

	return iPatches;
}

/***************************************************************************

public functions

***************************************************************************/

int LoadPatchData(ADLSYNTH *synth)
{
	if (!LoadPatches(synth)) /* load the melodic patches */
	return 0;

	if (!LoadDrumPatches(synth)) /* load the drum kit information */
	return 0;

	return 1;
}

void ResetChip(ADLSYNTH *synth)
{
	SoundInit(synth); /* reset card to be good */
}
