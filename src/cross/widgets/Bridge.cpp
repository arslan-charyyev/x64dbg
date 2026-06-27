
#include <algorithm>
#include <atomic>
#include <vector>
#include <QGuiApplication>
#include <QClipboard>
#include <QThread>
#include <QCoreApplication>

#include "Bridge.h"
#include "Types.h"
#include <Disassembler/Architecture.h>
#include <zydis_wrapper.h>

struct InvalidMemoryProvider : MemoryProvider
{
    bool read(duint addr, void* dest, duint size) override
    {
        return false;
    }

    bool getRange(duint addr, duint & base, duint & size) override
    {
        return false;
    }

    bool isCodePtr(duint addr) override
    {
        return false;
    }

    bool isValidPtr(duint addr) override
    {
        return false;
    }
} gInvalidMemoryProvider;

static std::atomic<MemoryProvider*> gMemory{&gInvalidMemoryProvider};

void DbgSetMemoryProvider(MemoryProvider* provider)
{
    gMemory.store(provider ? provider : &gInvalidMemoryProvider);
}

// BRIDGE

bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    return false;
}

bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    return false;
}

bool BridgeSettingGetUint(const char* section, const char* key, duint* value)
{
    return false;
}

bool BridgeSettingSetUint(const char* section, const char* key, duint value)
{
    return false;
}

const wchar_t* BridgeUserDirectory()
{
    return L".";
}

void* BridgeAlloc(size_t size)
{
    return malloc(size);
}

void BridgeFree(void* ptr)
{
    free(ptr);
}

// DBG

bool DbgIsDebugging()
{
    return gMemory.load() != &gInvalidMemoryProvider;
}

// The enumeration hook only feeds BpRefList. Per-breakpoint state (existence,
// enabled, active) is resolved live through the query hook (DbgGetBpxTypeAt)
// instead, so a ref built directly via BpRefVa stays correct without a prior
// BpRefList, and querying many addresses (e.g. a sidebar paint) stays cheap.
static std::atomic<BreakpointListFunc> gBreakpointList{nullptr};
// Backs the pointer BpRefList hands out. Widget data access is single-threaded (the
// Qt GUI thread, never the debug loop), so one buffer reused per call suffices; it
// stays valid until the next BpRefList call, which is how the populate loop reads it.
static std::vector<BP_REF> gBpRefs;

void DbgSetBreakpointList(BreakpointListFunc func)
{
    gBreakpointList.store(func);
}

static bool bpRefExists(const BP_REF* ref)
{
    // DLL/exception refs carry no probeable address and don't exist in ElfBug
    // (BpRefVa rejects the same three types); address-based refs are confirmed
    // live through the query hook.
    if(!ref || ref->type == bp_none || ref->type == bp_dll || ref->type == bp_exception)
        return false;
    return DbgGetBpxTypeAt(ref->module + ref->offset) == ref->type;
}

DBGFUNCTIONS* DbgFunctions()
{
    static DBGFUNCTIONS f = []
    {
        DBGFUNCTIONS f{};
        f.GetTraceRecordHitCount = [](duint addr) -> duint
        {
            return 0;
        };
        f.GetTraceRecordByteType = [](duint addr)
        {
            return TRACERECORDBYTETYPE::TraceRecordByteTypeUnknown;
        };
        f.ModBaseFromAddr = [](duint addr) -> duint
        {
            duint base = 0;
            if(!gMemory.load()->modBaseFromAddr(addr, base))
                return 0;
            return base;
        };
        f.ModNameFromAddr = [](duint addr, char* name, bool extension)
        {
            return gMemory.load()->modNameFromAddr(addr, name, MAX_MODULE_SIZE, extension);
        };
        f.StringFormatInline = [](const char* format, size_t size, char* dest)
        {
            return false;
        };
        f.MemIsCodePage = [](duint addr, bool refresh)
        {
            return gMemory.load()->isCodePtr(addr);
        };
        f.GetMnemonicBrief = [](const char* mnem, size_t resultSize, char* result)
        {
            *result = '\0';
        };
        f.ModRelocationAtAddr = [](duint addr, DBGRELOCATIONINFO * relocation)
        {
            return false;
        };
        f.PatchGetEx = [](duint addr, DBGPATCHINFO * info)
        {
            return false;
        };
        f.ValFromString = [](const char* expr, duint * value)
        {
            bool success = false;
            *value = DbgEval(expr, &success);
            return success;
        };
        f.ModGetParty = [](duint addr)
        {
            return 0;
        };
        f.PatchInRange = [](duint start, duint end)
        {
            return false;
        };
        f.MemPatch = [](duint start, const unsigned char* data, duint size)
        {
            return gMemory.load()->write(start, data, size);
        };
        f.MemUpdateMap = [] {};
        f.GetPageRights = [](duint addr, char* rights)
        {
            return false;
        };
        f.SetPageRights = [](duint addr, const char* rights)
        {
            return false;
        };
        f.GetUserComment = [](duint addr, char* comment)
        {
            return false;
        };
        f.FileOffsetToVa = [](const char* modname, duint offset) -> duint
        {
            return 0;
        };
        f.SymAutoComplete = [](const char* Search, char** Buffer, int MaxSymbols)
        {
            return 0;
        };
        // Exception/memory breakpoints don't exist in ElfBug, so these stay stubbed.
        f.EnumExceptions = [](ListInfo * constants) {};
        f.MemBpSize = [](duint addr) -> duint
        {
            return 0;
        };
        f.BpRefList = [](duint* count) -> BP_REF*
        {
            auto list = gBreakpointList.load();
            if(!list)
            {
                *count = 0;
                return nullptr;
            }
            std::vector<BridgeBreakpoint> bps(list(nullptr, 0));
            size_t n = std::min(list(bps.data(), bps.size()), bps.size());
            gBpRefs.clear();
            gBpRefs.reserve(n);
            for(size_t i = 0; i < n; i++)
                gBpRefs.push_back({bps[i].type, 0, bps[i].addr});
            *count = (duint)gBpRefs.size();
            return gBpRefs.data();
        };
        f.BpRefVa = [](BP_REF * ref, BPXTYPE type, duint va)
        {
            if(type == bp_none || type == bp_dll || type == bp_exception)
            {
                *ref = { bp_none };
                return false;
            }
            // No module database: store the absolute address as the offset (module 0).
            *ref = { type, 0, va };
            return true;
        };
        f.BpRefRva = [](BP_REF * ref, BPXTYPE type, const char* module, duint rva)
        {
            // Module-relative refs need a module base the shim cannot resolve.
            *ref = { bp_none };
            return false;
        };
        f.BpRefDll = [](BP_REF * ref, const char* module)
        {
            *ref = { bp_dll };
        };
        f.BpRefException = [](BP_REF * ref, unsigned int code)
        {
            *ref = { bp_exception, 0, code };
        };
        f.BpGetFieldNumber = [](const BP_REF * ref, BP_FIELD field, duint * value)
        {
            if(!bpRefExists(ref))
                return false;
            switch(field)
            {
            case bpf_type:
                *value = ref->type;
                return true;
            case bpf_offset:
                *value = ref->offset;
                return true;
            case bpf_address:
                *value = ref->module + ref->offset;
                return true;
            case bpf_enabled:
            case bpf_active:
                *value = 1;
                return true;
            case bpf_singleshoot:
            case bpf_silent:
            case bpf_typeex:
            case bpf_hwsize:
            case bpf_hwslot:
            case bpf_oldbytes:
            case bpf_fastresume:
            case bpf_hitcount:
                *value = 0;
                return true;
            default:
                return false;
            }
        };
        f.BpSetFieldNumber = [](const BP_REF * ref, BP_FIELD field, duint value)
        {
            return false;
        };
        f.BpGetFieldText = [](const BP_REF * ref, BP_FIELD field, CBSTRING callback, void* userdata)
        {
            if(!bpRefExists(ref))
                return false;
            switch(field)
            {
            case bpf_module:
            {
                char mod[MAX_MODULE_SIZE] = "";
                gMemory.load()->modNameFromAddr(ref->module + ref->offset, mod, sizeof(mod), false);
                callback(mod, userdata);
                return true;
            }
            case bpf_name:
            case bpf_breakcondition:
            case bpf_logtext:
            case bpf_logcondition:
            case bpf_commandtext:
            case bpf_commandcondition:
            case bpf_logfile:
                callback("", userdata);
                return true;
            default:
                return false;
            }
        };
        f.BpSetFieldText = [](const BP_REF * ref, BP_FIELD field, const char* value)
        {
            return false;
        };
        return f;
    }();
    return &f;
}

// TODO: resolve via per-module .dynsym lookup.
bool DbgGetLabelAt(duint addr, SEGTYPE seg, char* label)
{
    return false;
}

bool DbgGetModuleAt(duint addr, char* module)
{
    return false;
}

bool DbgGetCommentAt(duint addr, char* comment)
{
    return false;
}

bool DbgGetBookmarkAt(duint addr)
{
    return false;
}

static std::atomic<BreakpointQueryFunc> gBreakpointQuery{nullptr};

void DbgSetBreakpointQuery(BreakpointQueryFunc func)
{
    gBreakpointQuery.store(func);
}

BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    auto query = gBreakpointQuery.load();
    if(query)
        return query(addr);
    return bp_none;
}

bool DbgMemIsValidReadPtr(duint addr)
{
    return gMemory.load()->isValidPtr(addr);
}

// TODO: detect printable ASCII / UTF-16LE string at addr.
bool DbgGetStringAt(duint addr, char* str)
{
    return false;
}

duint DbgEval(const char* expr, bool* success)
{
    // TODO: replace this hex-only stub with a real cross expression evaluator (breaks rsp+8, symbols, mod.main(), etc.).
    if(success)
        *success = false;
    if(!expr)
        return 0;

    QString str = QString::fromUtf8(expr).trimmed();
    if(str.isEmpty())
        return 0;

    bool negative = false;
    if(str.startsWith('-'))
    {
        negative = true;
        str = str.mid(1).trimmed();
    }
    if(str.startsWith("0x", Qt::CaseInsensitive))
        str = str.mid(2);

    bool ok = false;
    const duint value = str.toULongLong(&ok, 16);
    if(!ok)
        return 0;

    if(success)
        *success = true;
    return negative ? static_cast<duint>(0) - value : value;
}

duint DbgValFromString(const char* expr)
{
    return DbgEval(expr, nullptr);
}

// Minimal dispatch for the view-follow commands the widgets emit ("disasm <expr>"
// / "dump <expr>"); mirrors how Windows' GuiDisasmAt/GuiDumpAt drive the views via
// Bridge signals. Unrecognized commands stay no-ops until the shim grows real ones.
static bool execCommand(const char* cmd)
{
    if(!cmd)
        return false;

    const QString text = QString::fromUtf8(cmd).trimmed();
    const int sep = text.indexOf(' ');
    if(sep > 0)
    {
        const QString command = text.left(sep);
        bool ok = false;
        const duint addr = DbgEval(text.mid(sep + 1).trimmed().toUtf8().constData(), &ok);
        if(ok)
        {
            if(command == "disasm" || command == "disassemble")
            {
                emit Bridge::getBridge()->disassembleAt(addr, addr);
                return true;
            }
            if(command == "dump")
            {
                emit Bridge::getBridge()->dumpAt(addr);
                return true;
            }
        }
    }

    printf("DbgCmdExec(\"%s\")\n", cmd);
    return false;
}

bool DbgCmdExec(const char* cmd)
{
    return execCommand(cmd);
}

bool DbgCmdExecDirect(const char* cmd)
{
    return execCommand(cmd);
}

bool DbgCmdExec(const QString & cmd)
{
    return DbgCmdExec(cmd.toUtf8().constData());
}

bool DbgCmdExecDirect(const QString & cmd)
{
    return DbgCmdExecDirect(cmd.toUtf8().constData());
}

duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    duint rangeBase = 0;
    duint rangeSize = 0;
    if(!gMemory.load()->getRange(addr, rangeBase, rangeSize))
        return 0;

    if(size != nullptr)
        *size = rangeSize;

    return rangeBase;
}

bool DbgMemRead(duint addr, void* dest, size_t size)
{
    return gMemory.load()->read(addr, dest, size);
}

FUNCTYPE DbgGetFunctionTypeAt(duint addr)
{
    return FUNC_NONE;
}

XREFTYPE DbgGetXrefTypeAt(duint addr)
{
    return XREF_NONE;
}

ARGTYPE DbgGetArgTypeAt(duint addr)
{
    return ARG_NONE;
}

LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth)
{
    return LOOP_NONE;
}

duint DbgGetBranchDestination(duint addr)
{
    uint8_t data[MAX_DISASM_BUFFER];
    if(!DbgMemRead(addr, data, sizeof(data)))
        return 0;
    Zydis zydis(sizeof(duint) == 8); // TODO: architecture plumbed per-caller
    if(!zydis.Disassemble(addr, data))
        return 0;
    return zydis.BranchDestination();
}

bool DbgIsJumpGoingToExecute(duint addr)
{
    return false; // TODO
}

bool DbgXrefGet(duint addr, XREF_INFO* info)
{
    return false;
}

void DbgReleaseEncodeTypeBuffer(void* buffer)
{
    BridgeFree(buffer);
}

void* DbgGetEncodeTypeBuffer(duint addr, duint* size)
{
    return nullptr;
}

bool DbgSetEncodeType(duint addr, duint size, ENCODETYPE type)
{
    return false;
}

void DbgDelEncodeTypeRange(duint start, duint end)
{
}

void DbgDelEncodeTypeSegment(duint start)
{
}

bool DbgMemMap(MEMMAP* memmap)
{
    if(!memmap)
        return false;

    memmap->count = 0;
    memmap->page = nullptr;

    auto* provider = gMemory.load();
    const size_t count = provider->getMemoryMap(nullptr, 0);
    if(count == 0)
        return false;

    // The widget frees memmap->page via BridgeFree (see MemoryMapView::refreshMapSlot).
    memmap->page = static_cast<MEMPAGE*>(BridgeAlloc(count * sizeof(MEMPAGE)));
    if(!memmap->page)
        return false;

    memmap->count = static_cast<int>(provider->getMemoryMap(memmap->page, count));
    return memmap->count > 0;
}

void DbgMenuPrepare(GUIMENUTYPE hMenu)
{
}

bool DbgSetCommentAt(duint addr, const char* text)
{
    return false;
}

void DbgSettingsUpdated()
{
}

duint DbgModBaseFromName(const char* name)
{
    return 0;
}

bool DbgIsValidExpression(const char* expression)
{
    return false;
}

// GUI

void GuiExecuteOnGuiThreadEx(GuiCallback callback, void* data)
{
    if(QThread::currentThread() == QCoreApplication::instance()->thread())
    {
        callback(data);
        return;
    }
    QMetaObject::invokeMethod(QCoreApplication::instance(), [callback, data]()
    {
        callback(data);
    }, Qt::QueuedConnection);
}

void GuiAddLogMessage(const char* msg)
{
    Bridge::getBridge()->addMsgToLog(msg);
}

void GuiUpdateAllViews()
{
    Bridge::getBridge()->repaintTableView();
}

void GuiUpdatePatches()
{
}

void GuiUpdateMemoryView()
{
}

void GuiAddStatusBarMessage(const char* msg)
{
}

void GuiUpdateBreakpointsView()
{
    emit Bridge::getBridge()->updateBreakpoints();
}

bool GuiIsUpdateDisabled()
{
    return false;
}

void GuiUpdateEnable(bool updateNow)
{
}

void GuiUpdateDisable()
{
}

Bridge* Bridge::getBridge()
{
    static Bridge i;
    return &i;
}

Architecture* Bridge::getArchitecture()
{
    struct DefaultArchitecture : Architecture
    {
        bool disasm64() const override
        {
            return sizeof(void*) == 8;
        }
        bool addr64() const override
        {
            return sizeof(void*) == 8;
        }
    };
    static DefaultArchitecture arch;
    return &arch;
}

void Bridge::CopyToClipboard(const QString & str)
{
    QGuiApplication::clipboard()->setText(str);
}

void Bridge::addMsgToLog(const QByteArray & bytes)
{
    printf("addMsgToLog: %s\n", bytes.data());
}

void Bridge::emitMenuAddToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu)
{
    // TODO: plugin menu integration is not implemented in the cross shim yet.
}

void Bridge::setResult(BridgeResult::Type type, dsint result)
{
    // TODO: wire synchronous GUI-request results once the debugger drives the Bridge.
}
