#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <string>
#include <string.h>
#include <type_traits>

// TODO: do something cross platform
using duint = uint64_t;
using dsint = int64_t;
using byte_t = uint8_t;

#ifndef _WIN32
using DWORD = uint32_t;
using WORD = uint16_t;
using NTSTATUS = uint32_t;

// Win32 GUID stand-in for widgets that interpret raw bytes as a GUID.
typedef struct _GUID
{
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    unsigned char Data4[8];
} GUID;

#define _TRUNCATE ((size_t)-1)

// MSVC intrinsic stand-in: BridgeList uses it as a type-check fast-fail.
#define __debugbreak() __builtin_trap()

template<size_t Count, class... Args>
int sprintf_s(char (&Dest)[Count], const char* fmt, Args... args)
{
    return snprintf(Dest, Count, fmt, args...);
}

inline size_t strcpy_s(char* dst, size_t size, const char* src)
{
    if(!dst || !src || !size)
        return 0;
    size_t i = 0;
    while(i < size - 1 && src[i]) dst[i] = src[i], i++;
    dst[i] = 0;
    return i;
}
#endif // _WIN32

// These types are needed in very deep UI components
typedef enum
{
    enc_unknown,  //must be 0
    enc_byte,     //1 byte
    enc_word,     //2 bytes
    enc_dword,    //4 bytes
    enc_fword,    //6 bytes
    enc_qword,    //8 bytes
    enc_tbyte,    //10 bytes
    enc_oword,    //16 bytes
    enc_mmword,   //8 bytes
    enc_xmmword,  //16 bytes
    enc_ymmword,  //32 bytes
    enc_zmmword,  //64 bytes avx512 not supported
    enc_real4,    //4 byte float
    enc_real8,    //8 byte double
    enc_real10,   //10 byte decimal
    enc_ascii,    //ascii sequence
    enc_unicode,  //unicode sequence
    enc_code,     //start of code
    enc_junk,     //junk code
    enc_middle    //middle of data
} ENCODETYPE;

typedef enum
{
    initialized,
    paused,
    running,
    stopped
} DBGSTATE;

typedef enum
{
    XREF_NONE,
    XREF_DATA,
    XREF_JMP,
    XREF_CALL
} XREFTYPE;

typedef struct
{
    duint addr;
    XREFTYPE type;
} XREF_RECORD;

typedef struct
{
    duint refcount;
    XREF_RECORD* references;
} XREF_INFO;

// Breakpoint types, mirroring src/bridge/bridgemain.h + src/dbg/_dbgfunctions.h
// so the ported BreakpointsView/EditBreakpointDialog compile unchanged.

#define MAX_MODULE_SIZE 256
#define MAX_BREAKPOINT_SIZE 256
#define MAX_CONDITIONAL_EXPR_SIZE 256
#define MAX_CONDITIONAL_TEXT_SIZE 256

typedef enum
{
    bp_none = 0,
    bp_normal = 1,
    bp_hardware = 2,
    bp_memory = 4,
    bp_dll = 8,
    bp_exception = 16
} BPXTYPE;

typedef enum
{
    hw_access,
    hw_write,
    hw_execute
} BPHWTYPE;

typedef enum
{
    mem_access,
    mem_read,
    mem_write,
    mem_execute
} BPMEMTYPE;

typedef enum
{
    dll_load = 1,
    dll_unload,
    dll_all
} BPDLLTYPE;

typedef enum
{
    ex_firstchance = 1,
    ex_secondchance,
    ex_all
} BPEXTYPE;

typedef enum
{
    hw_byte,
    hw_word,
    hw_dword,
    hw_qword
} BPHWSIZE;

typedef struct
{
    const char* name;
    duint value;
} CONSTANTINFO;

typedef void(*CBSTRING)(const char* str, void* userdata);

typedef enum
{
    bpf_type,
    bpf_offset,
    bpf_address,
    bpf_enabled,
    bpf_singleshoot,
    bpf_active,
    bpf_silent,
    bpf_typeex,
    bpf_hwsize,
    bpf_hwslot,
    bpf_oldbytes,
    bpf_fastresume,
    bpf_hitcount,
    bpf_module,
    bpf_name,
    bpf_breakcondition,
    bpf_logtext,
    bpf_logcondition,
    bpf_commandtext,
    bpf_commandcondition,
    bpf_logfile,
} BP_FIELD;

// A reference to a breakpoint. The GetField/SetField helpers are defined inline
// in Bridge.h, once DbgFunctions() is declared.
struct BP_REF
{
    BPXTYPE type;
    duint module;
    duint offset;

    bool GetField(BP_FIELD field, duint & value);
    bool GetField(BP_FIELD field, bool & value);
    bool SetField(BP_FIELD field, duint value);
    bool GetField(BP_FIELD field, std::string & value);
    bool SetField(BP_FIELD field, const std::string & value);

    template<class T, typename = typename std::enable_if< std::is_enum<T>::value, T >::type>
    void GetField(BP_FIELD field, T & value)
    {
        duint n = 0;
        GetField(field, n);
        value = (T)n;
    }
};

typedef struct
{
    BPXTYPE type;
    duint addr;
    bool enabled;
    bool singleshoot;
    bool active;
    char name[MAX_BREAKPOINT_SIZE];
    char mod[MAX_MODULE_SIZE];
    unsigned short slot;
    unsigned char typeEx;
    unsigned char hwSize;
    unsigned int hitCount;
    bool fastResume;
    bool silent;
    char breakCondition[MAX_CONDITIONAL_EXPR_SIZE];
    char logText[MAX_CONDITIONAL_TEXT_SIZE];
    char logCondition[MAX_CONDITIONAL_EXPR_SIZE];
    char commandText[MAX_CONDITIONAL_TEXT_SIZE];
    char commandCondition[MAX_CONDITIONAL_EXPR_SIZE];
} BRIDGEBP;

typedef struct
{
    int count;
    BRIDGEBP* bp;
} BPMAP;
