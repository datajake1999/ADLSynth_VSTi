# Introduction
This is a quickly thrown together VSTi wrapper around the ADLSynth project. ADLSynth is a modern port of the AdLib MIDI driver that is included in the Windows 3.1 DDK. The highly accurate Nuked OPL3 emulator is used for audio output.

# Automatable Parameters

* Volume: Synth master volume.
* VolumeDisplay: Sets the unit for displaying the aforementioned Volume parameter, either dB or %.
* Transpose: Applies an offset to incoming MIDI notes.
* PushMidi: Queue's MIDI events instead of processing them immediately. Queued events have sample accurate timing, while immediate events can have jittery playback with large audio buffers.

# Extra Notes

* This is only a VST2 compatible plug-in. A VST3 version is not planned for various reasons.

# License
The VST interface code as well as the Nuked OPL3 emulator are licensed under the terms of the GNU General Public License. The MIDI to OPL translation code is licensed under the terms of the Windows 3.1 DDK license.
