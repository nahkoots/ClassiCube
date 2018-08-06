#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "Utils.h"
#include "2DStructs.h"
#include "PackedCol.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
struct DrawTextArgs;
struct FontDesc;
struct Bitmap;
struct AsyncRequest;

enum SOCKET_SELECT { SOCKET_SELECT_READ, SOCKET_SELECT_WRITE, SOCKET_SELECT_ERROR };
#if CC_BUILD_WIN
typedef void* SocketPtr;
#else
typedef Int32 SocketPtr;
#endif

extern UChar* Platform_NewLine; /* Newline for text */
extern UChar Platform_DirectorySeparator;
extern ReturnCode ReturnCode_FileShareViolation;
extern ReturnCode ReturnCode_FileNotFound;
extern ReturnCode ReturnCode_NotSupported;
extern ReturnCode ReturnCode_SocketInProgess;
extern ReturnCode ReturnCode_SocketWouldBlock;
extern ReturnCode ReturnCode_InvalidArg;

void Platform_UnicodeExpand(void* dstPtr, STRING_PURE String* src);
void Platform_Init(void);
void Platform_Free(void);
void Platform_Exit(ReturnCode code);
STRING_PURE String Platform_GetCommandLineArgs(void);

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize, const UChar* place);
void* Platform_MemAllocCleared(UInt32 numElems, UInt32 elemsSize, const UChar* place);
void* Platform_MemRealloc(void* mem, UInt32 numElems, UInt32 elemsSize, const UChar* place);
void Platform_MemFree(void** mem);
void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes);
void Platform_MemCpy(void* dst, void* src, UInt32 numBytes);

void Platform_Log(STRING_PURE String* message);
void Platform_LogConst(const UChar* message);
#define Platform_Log1(format, a1) Platform_Log4(format, a1, NULL, NULL, NULL)
#define Platform_Log2(format, a1, a2) Platform_Log4(format, a1, a2, NULL, NULL)
#define Platform_Log3(format, a1, a2, a3) Platform_Log4(format, a1, a2, a3, NULL)
void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4);
void Platform_CurrentUTCTime(DateTime* time);
void Platform_CurrentLocalTime(DateTime* time);

bool Platform_DirectoryExists(STRING_PURE String* path);
ReturnCode Platform_DirectoryCreate(STRING_PURE String* path);
bool Platform_FileExists(STRING_PURE String* path);
typedef void Platform_EnumFilesCallback(STRING_PURE String* filename, void* obj);
ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback);
ReturnCode Platform_FileGetModifiedTime(STRING_PURE String* path, DateTime* time);

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path);
ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path);
ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path);
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote);
ReturnCode Platform_FileClose(void* file);
ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType);
ReturnCode Platform_FilePosition(void* file, UInt32* position);
ReturnCode Platform_FileLength(void* file, UInt32* length);

void Platform_ThreadSleep(UInt32 milliseconds);
typedef void Platform_ThreadFunc(void);
void* Platform_ThreadStart(Platform_ThreadFunc* func);
void Platform_ThreadJoin(void* handle);
/* Frees handle to thread - NOT THE THREAD ITSELF */
void Platform_ThreadFreeHandle(void* handle);

void* Platform_MutexCreate(void);
void Platform_MutexFree(void* handle);
void Platform_MutexLock(void* handle);
void Platform_MutexUnlock(void* handle);

void* Platform_EventCreate(void);
void Platform_EventFree(void* handle);
void Platform_EventSignal(void* handle);
void Platform_EventWait(void* handle);

struct Stopwatch { Int64 Data[2]; };
void Stopwatch_Start(struct Stopwatch* timer);
Int32 Stopwatch_ElapsedMicroseconds(struct Stopwatch* timer);
ReturnCode Platform_StartShell(STRING_PURE String* args);

void Platform_FontMake(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style);
void Platform_FontFree(struct FontDesc* desc);
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args);
void Platform_SetBitmap(struct Bitmap* bmp);
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col);
void Platform_ReleaseBitmap(void);

void Platform_SocketCreate(SocketPtr* socket);
ReturnCode Platform_SocketAvailable(SocketPtr socket, UInt32* available);
ReturnCode Platform_SocketSetBlocking(SocketPtr socket, bool blocking);
ReturnCode Platform_SocketGetError(SocketPtr socket, ReturnCode* result);

ReturnCode Platform_SocketConnect(SocketPtr socket, STRING_PURE String* ip, Int32 port);
ReturnCode Platform_SocketRead(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketWrite(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketClose(SocketPtr socket);
ReturnCode Platform_SocketSelect(SocketPtr socket, Int32 selectMode, bool* success);

void Platform_HttpInit(void);
ReturnCode Platform_HttpMakeRequest(struct AsyncRequest* request, void** handle);
ReturnCode Platform_HttpGetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size);
ReturnCode Platform_HttpGetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress);
ReturnCode Platform_HttpFreeRequest(void* handle);
ReturnCode Platform_HttpFree(void);

#define AUDIO_MAX_CHUNKS 5
struct AudioFormat { UInt8 Channels, BitsPerSample; Int32 SampleRate; };
#define AudioFormat_Eq(a, b) ((a)->Channels == (b)->Channels && (a)->BitsPerSample == (b)->BitsPerSample && (a)->SampleRate == (b)->SampleRate)
typedef Int32 AudioHandle;

void Platform_AudioInit(AudioHandle* handle, Int32 buffers);
void Platform_AudioFree(AudioHandle handle);
struct AudioFormat* Platform_AudioGetFormat(AudioHandle handle);
void Platform_AudioSetFormat(AudioHandle handle, struct AudioFormat* format);
void Platform_AudioPlayData(AudioHandle handle, Int32 idx, void* data, UInt32 dataSize);
bool Platform_AudioIsCompleted(AudioHandle handle, Int32 idx);
bool Platform_AudioIsFinished(AudioHandle handle);
#endif
