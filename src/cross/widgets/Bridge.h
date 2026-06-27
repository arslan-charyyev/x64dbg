#pragma once

#include <QObject>
#include <QString>
#include <QDebug>

#include "Types.h"
#include "RegisterContext.h"

#define MAX_SETTING_SIZE 4096

bool BridgeSettingGet(const char* section, const char* key, char* value);
bool BridgeSettingSet(const char* section, const char* key, const char* value);
bool BridgeSettingGetUint(const char* section, const char* key, duint* value);
bool BridgeSettingSetUint(const char* section, const char* key, duint value);
const wchar_t* BridgeUserDirectory();
void* BridgeAlloc(size_t size);
void BridgeFree(void* ptr);

#include "bridgelist.h"

#define MAX_LABEL_SIZE 256
#define MAX_COMMENT_SIZE 256
#define MAX_STRING_SIZE 2048
#define RIGHTS_STRING_SIZE (sizeof("ERWCG"))

// PAGE_SIZE is also defined by some Linux system headers; keep ours guarded.
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif // PAGE_SIZE

#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD 0x100

#define MEM_COMMIT 0x1000

#define MEM_PRIVATE 0x20000
#define MEM_MAPPED 0x40000
#define MEM_IMAGE 0x1000000

enum MODULEPARTY
{
    mod_user = 0,
    mod_system = 1,
};

typedef enum
{
    GUI_PLUGIN_MENU,
    GUI_DISASM_MENU,
    GUI_DUMP_MENU,
    GUI_STACK_MENU,
    GUI_GRAPH_MENU,
    GUI_MEMMAP_MENU,
    GUI_SYMMOD_MENU,
} GUIMENUTYPE;

typedef struct
{
    duint BaseAddress;
    duint AllocationBase;
    DWORD AllocationProtect;
    duint RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
} MEMORY_BASIC_INFORMATION;

typedef struct
{
    MEMORY_BASIC_INFORMATION mbi;
    char info[MAX_MODULE_SIZE];
} MEMPAGE;

typedef struct
{
    int count;
    MEMPAGE* page;
} MEMMAP;

typedef struct
{
    duint start;
    duint end;
} SELECTIONDATA;

enum SEGTYPE
{
    SEG_DEFAULT,
    SEG_ES,
    SEG_DS,
    SEG_FS,
    SEG_GS,
    SEG_CS,
    SEG_SS,
};

typedef enum
{
    FUNC_NONE,
    FUNC_BEGIN,
    FUNC_MIDDLE,
    FUNC_END,
    FUNC_SINGLE
} FUNCTYPE;

typedef enum
{
    LOOP_NONE,
    LOOP_BEGIN,
    LOOP_MIDDLE,
    LOOP_ENTRY,
    LOOP_END,
    LOOP_SINGLE
} LOOPTYPE;

typedef enum
{
    ARG_NONE,
    ARG_BEGIN,
    ARG_MIDDLE,
    ARG_END,
    ARG_SINGLE
} ARGTYPE;

typedef enum
{
    InstructionBody = 0,
    InstructionHeading = 1,
    InstructionTailing = 2,
    InstructionOverlapped = 3, // The byte was executed with differing instruction base addresses
    TraceRecordByteTypeUnknown = 4 // The following is not implemented yet.
} TRACERECORDBYTETYPE;

typedef struct
{
    uint32_t rva;
    uint8_t type;
    uint16_t size;
} DBGRELOCATIONINFO;

typedef struct
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    unsigned char oldbyte;
    unsigned char newbyte;
} DBGPATCHINFO;

struct DBGFUNCTIONS
{
    duint(*GetTraceRecordHitCount)(duint addr);
    TRACERECORDBYTETYPE(*GetTraceRecordByteType)(duint address);
    duint(*ModBaseFromAddr)(duint addr);
    bool (*ModNameFromAddr)(duint base, char* name, bool extension);
    bool (*StringFormatInline)(const char* format, size_t size, char* dest);
    bool (*MemIsCodePage)(duint addr, bool refresh);
    void (*GetMnemonicBrief)(const char* mnem, size_t resultSize, char* result);
    bool (*ModRelocationAtAddr)(duint addr, DBGRELOCATIONINFO* relocation);
    bool (*PatchGetEx)(duint addr, DBGPATCHINFO* info);
    bool (*ValFromString)(const char* expr, duint* value);
    int (*ModGetParty)(duint addr);
    bool (*PatchInRange)(duint start, duint end);
    bool (*MemPatch)(duint start, const unsigned char* data, duint size);
    void (*MemUpdateMap)();
    bool (*GetPageRights)(duint addr, char* rights);
    bool (*SetPageRights)(duint addr, const char* rights);
    bool (*GetUserComment)(duint addr, char* comment);
    duint(*FileOffsetToVa)(const char* modname, duint offset);
    int (*SymAutoComplete)(const char* Search, char** Buffer, int MaxSymbols);
    void (*EnumExceptions)(ListOf(CONSTANTINFO) constants);
    duint(*MemBpSize)(duint addr);
    BP_REF* (*BpRefList)(duint* count);
    bool (*BpRefVa)(BP_REF* ref, BPXTYPE type, duint va);
    bool (*BpRefRva)(BP_REF* ref, BPXTYPE type, const char* module, duint rva);
    void (*BpRefDll)(BP_REF* ref, const char* module);
    void (*BpRefException)(BP_REF* ref, unsigned int code);
    bool (*BpGetFieldNumber)(const BP_REF* ref, BP_FIELD field, duint* value);
    bool (*BpSetFieldNumber)(const BP_REF* ref, BP_FIELD field, duint value);
    bool (*BpGetFieldText)(const BP_REF* ref, BP_FIELD field, CBSTRING callback, void* userdata);
    bool (*BpSetFieldText)(const BP_REF* ref, BP_FIELD field, const char* value);
};

struct MemoryProvider
{
    virtual ~MemoryProvider() = default;
    virtual bool read(duint addr, void* dest, duint size) = 0;
    virtual bool getRange(duint addr, duint & base, duint & size) = 0;
    virtual bool isCodePtr(duint addr) = 0;
    virtual bool isValidPtr(duint addr) = 0;
    virtual bool write(duint addr, const void* src, duint size) { return false; }
    virtual bool writeRegister(const char* name, duint value) { return false; }
    virtual bool modBaseFromAddr(duint addr, duint & base) { return false; }
    virtual bool modNameFromAddr(duint addr, char* buf, duint bufSize, bool extension) { return false; }
    // Fill up to maxCount pages (pass out=null to query the total count) and return the number written.
    virtual size_t getMemoryMap(MEMPAGE* out, size_t maxCount) { return 0; }
};

void DbgSetMemoryProvider(MemoryProvider* provider);

using BreakpointQueryFunc = BPXTYPE(*)(duint addr);
void DbgSetBreakpointQuery(BreakpointQueryFunc func);

// A breakpoint surfaced by the enumeration hook below. Free of ElfBug types so these
// headers still compile on Windows; the Linux adapter fills it from the engine.
struct BridgeBreakpoint
{
    duint addr;
    BPXTYPE type;
};

// Lists the engine's breakpoints (pass out=null to query the count), mirroring the
// memory map's two-call shape. BpRefList turns the result into BP_REF handles.
using BreakpointListFunc = size_t (*)(BridgeBreakpoint* out, size_t maxCount);
void DbgSetBreakpointList(BreakpointListFunc func);

// Mutates an engine breakpoint by address. The command dispatcher calls this for
// bp/bc; the Linux adapter routes it to ElfBug set/delete. Returns true on success.
enum class BpMutation { Set, Delete };
using BreakpointMutateFunc = bool (*)(BpMutation op, duint addr);
void DbgSetBreakpointMutate(BreakpointMutateFunc func);

bool DbgIsDebugging();
DBGFUNCTIONS* DbgFunctions();

inline bool BP_REF::GetField(BP_FIELD field, duint & value)
{
    return DbgFunctions()->BpGetFieldNumber(this, field, &value);
}

inline bool BP_REF::GetField(BP_FIELD field, bool & value)
{
    duint n = 0;
    if(!DbgFunctions()->BpGetFieldNumber(this, field, &n))
        return false;
    value = !!n;
    return true;
}

inline bool BP_REF::SetField(BP_FIELD field, duint value)
{
    return DbgFunctions()->BpSetFieldNumber(this, field, value);
}

inline bool BP_REF::GetField(BP_FIELD field, std::string & value)
{
    return DbgFunctions()->BpGetFieldText(this, field, [](const char* str, void* userdata)
    {
        *(std::string*)userdata = str;
    }, &value);
}

inline bool BP_REF::SetField(BP_FIELD field, const std::string & value)
{
    return DbgFunctions()->BpSetFieldText(this, field, value.c_str());
}

bool DbgGetLabelAt(duint addr, SEGTYPE seg, char* label);
bool DbgGetModuleAt(duint addr, char* module);
bool DbgGetCommentAt(duint addr, char* comment);
bool DbgGetBookmarkAt(duint addr);
BPXTYPE DbgGetBpxTypeAt(duint addr);
bool DbgMemIsValidReadPtr(duint addr);
bool DbgGetStringAt(duint addr, char* str);
duint DbgEval(const char* expr, bool* success = nullptr);
duint DbgValFromString(const char* expr);
bool DbgCmdExec(const char* cmd);
bool DbgCmdExecDirect(const char* cmd);
duint DbgMemFindBaseAddr(duint addr, duint* size);
bool DbgMemRead(duint addr, void* dest, size_t size);
FUNCTYPE DbgGetFunctionTypeAt(duint addr);
XREFTYPE DbgGetXrefTypeAt(duint addr);
ARGTYPE DbgGetArgTypeAt(duint addr);
LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth);
duint DbgGetBranchDestination(duint addr);
bool DbgIsJumpGoingToExecute(duint addr);
bool DbgXrefGet(duint addr, XREF_INFO* info);
void DbgReleaseEncodeTypeBuffer(void* buffer);
void* DbgGetEncodeTypeBuffer(duint addr, duint* size);
bool DbgSetEncodeType(duint addr, duint size, ENCODETYPE type);
void DbgDelEncodeTypeRange(duint start, duint end);
void DbgDelEncodeTypeSegment(duint start);
bool DbgMemMap(MEMMAP* memmap);
void DbgMenuPrepare(GUIMENUTYPE hMenu);
bool DbgSetCommentAt(duint addr, const char* text);
void DbgSettingsUpdated();
duint DbgModBaseFromName(const char* name);
bool DbgIsValidExpression(const char* expression);

struct TYPEDESCRIPTOR;

typedef bool (*TYPETOSTRING)(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount); //don't change destCount for final failure

#define TYPEDESCRIPTOR_MAGIC 0x1337

struct TYPEDESCRIPTOR
{
    bool expanded; //is the type node expanded?
    bool reverse; //big endian?
    uint16_t magic; // compatiblity (set to TYPEDESCRIPTOR_MAGIC for the new version)
    const char* name; //type name (int b)
    duint addr; //virtual address
    duint offset; //offset to addr for the actual location in bytes
    int id; //type id
    int sizeBits; //sizeof(type) in bits
    TYPETOSTRING callback; //convert to string
    void* userdata; //user data
    duint bitOffset; // bit offset from first bitfield
    const char* typeName; // undecorated typename
};

using GuiCallback = void(*)(void*);

void GuiExecuteOnGuiThreadEx(GuiCallback callback, void* data);
void GuiAddLogMessage(const char* msg);
void GuiAddStatusBarMessage(const char* msg);
void GuiUpdateAllViews();
void GuiUpdatePatches();
void GuiUpdateMemoryView();
void GuiUpdateBreakpointsView();
void GuiUpdateDisassemblyView();
bool GuiIsUpdateDisabled();
void GuiUpdateEnable(bool updateNow);
void GuiUpdateDisable();

class GuiDisableUpdateScope
{
    bool updateAfter;
    bool wasEnabled;

public:
    GuiDisableUpdateScope(const GuiDisableUpdateScope &) = delete;

    explicit GuiDisableUpdateScope(bool updateAfter = true)
        : updateAfter(updateAfter)
    {
        wasEnabled = !GuiIsUpdateDisabled();
        if(wasEnabled)
            GuiUpdateDisable();
    }

    ~GuiDisableUpdateScope()
    {
        if(wasEnabled)
            GuiUpdateEnable(updateAfter);
    }
};

// QString helpers
bool DbgCmdExec(const QString & cmd);
bool DbgCmdExecDirect(const QString & cmd);

class QWidget;
class QMenu;
class Architecture;

// Mirrors the Windows Bridge's synchronous GUI-request result types. The cross
// shim keeps the full enum for parity; only a few are wired so far.
class BridgeResult
{
public:
    enum Type
    {
        ScriptAdd,
        ScriptMessage,
        RefInitialize,
        MenuAddToList,
        MenuAdd,
        MenuAddEntry,
        MenuAddSeparator,
        MenuClear,
        MenuRemove,
        SelectionGet,
        SelectionSet,
        GetlineWindow,
        MenuSetIcon,
        MenuSetEntryIcon,
        MenuSetEntryChecked,
        MenuSetVisible,
        MenuSetEntryVisible,
        MenuSetName,
        MenuSetEntryName,
        GetGlobalNotes,
        GetDebuggeeNotes,
        RegisterScriptLang,
        LoadGraph,
        GraphAt,
        GetActiveView,
        TypeAddNode,
        TypeClear,
        MenuSetEntryHotkey,
        GraphCurrent,
        Last,
    };
};

class Bridge : public QObject
{
    Q_OBJECT
    Bridge() = default;

signals:
    void close();
    void repaintTableView();
    void updateDump();
    void updateDisassembly();
    void updateBreakpoints();
    void dbgStateChanged(DBGSTATE state);
    void updateMemory();
    void disassembleAt(duint va, duint cip);
    void dumpAt(duint va);
    void selectInMemoryMap(duint addr);
    void selectionMemmapGet(SELECTIONDATA* selection);
    void selectionMemmapSet(const SELECTIONDATA* selection);
    void focusMemmap();
    void getDumpAttention();

    void typeAddNode(void* parent, const TYPEDESCRIPTOR* descriptor, void** result);
    void typeClear();
    void typeUpdateWidget();

public:
    static Bridge* getBridge();
    static Architecture* getArchitecture();
    static void CopyToClipboard(const QString & str);

    void addMsgToLog(const QByteArray & bytes);
    void emitMenuAddToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu = -1);
    void setResult(BridgeResult::Type type, dsint result = 0);

    duint mLastCip = 0;
    bool mIsRunning = true;
};
