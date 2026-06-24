@echo off
g++ -c -DVST_FORCE_DEPRECATED=0 -D_WIN32_WINNT=0x0400 -I..\VST2_SDK -O3 ..\src\adlsynth\*.c ..\src\opl3\*.c ..\src\vst\*.cpp ..\VST2_SDK\public.sdk\source\vst2.x\*.cpp
g++ *.o -s -static -shared -lkernel32 -luser32 -o ADLSynth.dll
del *.o
