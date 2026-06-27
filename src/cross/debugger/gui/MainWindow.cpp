#include "gui/MainWindow.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QFile>
#include <QStyleFactory>
#include <QThread>
#include <QCoreApplication>
#include <Memory/MemoryPage.h>
#include "core/LinuxArchitecture.h"
#include "gui/CPUStack.h"
#include "Gui/MemoryMapView.h"
#include "Gui/BreakpointsView.h"

static LinuxArchitecture gArch;

static QIcon icon(const char* name)
{
    return QIcon(QString(":/Default/icons/%1.png").arg(name));
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("x64dbg");
    resize(1200, 800);

    mProvider = new DbgAdapter(this);

    setupToolBar();
    setupTabs();

    connect(mProvider, &DbgAdapter::processCreated, this, &MainWindow::onProcessCreated, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::processExited, this, &MainWindow::onProcessExited, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::registersUpdated, this, [this](const REGDUMP & dump)
    {
        mRegisters->setRegisters(&dump);
    }, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::logMessage, this, &MainWindow::onLogMessage, Qt::QueuedConnection);
    connect(mProvider, &DbgAdapter::stopped, this, &MainWindow::onStopped, Qt::QueuedConnection);

    // MemoryMapView's "Follow in Disassembler/Dump" issue these commands via the shim,
    // which re-emits them as Bridge signals to drive the CPU tab's existing views.
    connect(Bridge::getBridge(), &Bridge::disassembleAt, this, [this](duint va, duint)
    {
        mDisassembly->gotoAddress(va);
        mTabWidget->setCurrentIndex(0);
    });
    connect(Bridge::getBridge(), &Bridge::dumpAt, this, [this](duint va)
    {
        mHexDump->printDumpAt(va);
        mTabWidget->setCurrentIndex(0);
    });

    const auto menuFile = menuBar()->addMenu(tr("&File"));
    const auto actionOpen = menuFile->addAction(tr("&Open..."), this, &MainWindow::onOpen);
    actionOpen->setShortcut(QKeySequence::Open);
    menuFile->addSeparator();
    menuFile->addAction(tr("E&xit"), this, &QWidget::close);

    const auto menuDebug = menuBar()->addMenu(tr("&Debug"));
    const auto actionRun = menuDebug->addAction(tr("&Run"), this, &MainWindow::onContinue);
    actionRun->setShortcut(Qt::Key_F9);

    menuDebug->addSeparator();
    const auto actionBp = menuDebug->addAction(tr("Toggle &Breakpoint"), this, &MainWindow::onToggleBreakpoint);
    actionBp->setShortcut(Qt::Key_F2);

    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
    stopDebugThread();
}

void MainWindow::stopDebugThread()
{
    if(!mDebugThread)
        return;

    // Continue() breaks any pause spin-loop in the debug thread so
    // Stop()'s SIGKILL can be reaped by waitpid and the loop exits.
    mProvider->Continue();
    (void)mProvider->Stop();

    if(!mDebugThread->wait(3000))
    {
        mDebugThread->terminate();
        mDebugThread->wait();
    }
    delete mDebugThread;
    mDebugThread = nullptr;

    // The killed session may have already queued cross-thread signals (notably
    // processExited, whose handler nulls the memory provider). Drop them so a
    // stale event can't tear down the session we are about to start.
    QCoreApplication::removePostedEvents(this, QEvent::MetaCall);
}

void MainWindow::setupToolBar()
{
    const auto toolBar = new QToolBar(tr("Main Toolbar"), this);
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addToolBar(toolBar);

    const auto actionOpen = toolBar->addAction(icon("folder-horizontal-open"), QString(), this, &MainWindow::onOpen);
    actionOpen->setToolTip(tr("Open (Ctrl+O)"));

    const auto actionRestart = toolBar->addAction(icon("arrow-restart"), QString());
    actionRestart->setToolTip(tr("Restart"));
    actionRestart->setEnabled(false);

    const auto actionClose = toolBar->addAction(icon("control-stop"), QString());
    actionClose->setToolTip(tr("Close"));
    actionClose->setEnabled(false);

    toolBar->addSeparator();

    const auto actionRun = toolBar->addAction(icon("arrow-run"), QString(), this, &MainWindow::onContinue);
    actionRun->setToolTip(tr("Run (F9)"));

    const auto actionPause = toolBar->addAction(icon("control-pause"), QString(), this, &MainWindow::onPause);
    actionPause->setToolTip(tr("Pause"));

    toolBar->addSeparator();

    const auto actionStepInto = toolBar->addAction(icon("arrow-step-into"), QString(), this, &MainWindow::onStepInto);
    actionStepInto->setToolTip(tr("Step Into (F7)"));
    actionStepInto->setShortcut(Qt::Key_F7);

    const auto actionStepOver = toolBar->addAction(icon("arrow-step-over"), QString(), this, &MainWindow::onStepOver);
    actionStepOver->setToolTip(tr("Step Over (F8)"));
    actionStepOver->setShortcut(Qt::Key_F8);
}

void MainWindow::setupTabs()
{
    mTabWidget = new QTabWidget(this);
    mTabWidget->setDocumentMode(true);
    setCentralWidget(mTabWidget);

    mTabWidget->addTab(createCpuTab(), icon("processor-cpu"), tr("CPU"));

    mLog = new QTextBrowser(this);
    mLog->setFont(ConfigFont("Log"));
    mTabWidget->addTab(mLog, icon("log"), tr("Log"));

    auto makePlaceholder = [this](const QString & text)
    {
        const auto label = new QLabel(text, this);
        label->setAlignment(Qt::AlignCenter);
        label->setFont(ConfigFont("Log"));
        return label;
    };

    mBreakpoints = new BreakpointsView();
    mTabWidget->addTab(mBreakpoints, icon("breakpoint"), tr("Breakpoints"));
    mMemoryMap = new MemoryMapView();
    mTabWidget->addTab(mMemoryMap, icon("memory-map"), tr("Memory Map"));
    mTabWidget->addTab(makePlaceholder(tr("Call stack view - not yet implemented")), icon("callstack"), tr("Call Stack"));
    mTabWidget->addTab(makePlaceholder(tr("Threads view - not yet implemented")), icon("arrow-threads"), tr("Threads"));

    onLogMessage("[x64dbg] Ready. Open an ELF binary to begin debugging.");
}

QWidget* MainWindow::createCpuTab()
{
    const auto memPage = new MemoryPage(0, 0, this);
    mDisassembly = new Disassembly(&gArch, false, this);
    mHexDump = new HexDump(&gArch, this, memPage);
    mStack = new CPUStack(&gArch, mProvider, this);
    mRegisters = new RegistersView(this);

    {
        const int charwidth = mHexDump->getCharWidth();
        HexDump::ColumnDescriptor wColDesc;
        HexDump::DataDescriptor dDesc{};

        wColDesc.isData = true;
        wColDesc.itemCount = 16;
        wColDesc.separator = 4;
        dDesc.itemSize = HexDump::Byte;
        dDesc.byteMode = HexDump::HexByte;
        wColDesc.data = dDesc;
        mHexDump->appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

        wColDesc.isData = true;
        wColDesc.itemCount = 16;
        wColDesc.separator = 0;
        dDesc.itemSize = HexDump::Byte;
        dDesc.byteMode = HexDump::AsciiByte;
        wColDesc.data = dDesc;
        mHexDump->appendDescriptor(8 + charwidth * 16, tr("ASCII"), false, wColDesc);

        wColDesc.isData = false;
        wColDesc.itemCount = 0;
        wColDesc.separator = 0;
        wColDesc.data = dDesc;
        mHexDump->appendDescriptor(0, "", false, wColDesc);
    }

    // Allow all widgets to be freely resized by splitters
    mDisassembly->setMinimumHeight(0);
    mHexDump->setMinimumHeight(0);
    mStack->setMinimumHeight(0);

    const auto topSplitter = new QSplitter(Qt::Horizontal);
    topSplitter->addWidget(mDisassembly);
    topSplitter->addWidget(mRegisters);
    topSplitter->setStretchFactor(0, 70);
    topSplitter->setStretchFactor(1, 30);

    const auto bottomSplitter = new QSplitter(Qt::Horizontal);
    bottomSplitter->addWidget(mHexDump);
    bottomSplitter->addWidget(mStack);
    bottomSplitter->setStretchFactor(0, 60);
    bottomSplitter->setStretchFactor(1, 40);

    const auto mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setStretchFactor(0, 55);
    mainSplitter->setStretchFactor(1, 45);
    mainSplitter->setChildrenCollapsible(false);

    connect(mStack, &CPUStack::followDisasmRequested,
            mDisassembly, &Disassembly::gotoAddress);

    return mainSplitter;
}

void MainWindow::onOpen()
{
    const auto path = QFileDialog::getOpenFileName(this, tr("Open ELF Binary"), QString(), tr("All Files (*)"));
    if(path.isEmpty())
        return;

    onLogMessage(QString("[x64dbg] Launching: %1").arg(path));

    stopDebugThread();

    if(!mProvider->loadEngine())
        return;

    DbgSetMemoryProvider(mProvider);

    //? Init and Start must run on the same thread
    auto pathBytes = path.toUtf8();
    mDebugThread = QThread::create([this, pathBytes]()
    {
        if(!mProvider->launch(pathBytes.constData()))
        {
            DbgSetMemoryProvider(nullptr);
            emit mProvider->logMessage("[x64dbg] Failed to launch process");
            return;
        }
        mProvider->Start();
    });
    mDebugThread->start();
}

void MainWindow::onContinue() const
{
    if(mProvider && mProvider->isActive())
    {
        onLogMessage("[x64dbg] Resuming...");
        mProvider->Continue();
        statusBar()->showMessage(tr("Running"));
    }
}

void MainWindow::onProcessCreated(const duint entryPoint) const
{
    onLogMessage(QString("[x64dbg] Process attached, entry: 0x%1").arg(entryPoint, 0, 16));
    mHexDump->printDumpAt(entryPoint);
    GuiUpdateBreakpointsView();
}

void MainWindow::onProcessExited(const int exitCode) const
{
    onLogMessage(QString("[x64dbg] Process exited with code %1").arg(exitCode));
    DbgSetMemoryProvider(nullptr);
    mDisassembly->reloadData();
    mHexDump->reloadData();
    mMemoryMap->refreshMapSlot();
    constexpr REGDUMP emptyDump{};
    mRegisters->setRegisters(&emptyDump);
    statusBar()->showMessage(QString("Process exited with code %1").arg(exitCode));
}

void MainWindow::onLogMessage(const QString & msg) const
{
    mLog->append(msg);
}

void MainWindow::onPause() const
{
    if(mProvider && mProvider->isActive())
        mProvider->Pause();
}

void MainWindow::onStepInto() const
{
    if(mProvider && mProvider->isActive())
        mProvider->StepInto();
}

void MainWindow::onStepOver() const
{
    onLogMessage("[x64dbg] Step-over not implemented, using step-into");
    onStepInto();
}

void MainWindow::onToggleBreakpoint() const
{
    if(!mProvider || !mProvider->isActive())
        return;

    const duint addr = mDisassembly->rvaToVa(mDisassembly->getInitialSelection());
    if(!mProvider->toggleBreakpoint(addr))
        onLogMessage(QString("[x64dbg] Failed to toggle breakpoint at 0x%1").arg(addr, 0, 16));
    mDisassembly->reloadData();
    GuiUpdateBreakpointsView();
}

void MainWindow::onStopped(const duint rip, const QString & reason) const
{
    mDisassembly->gotoAddress(rip);
    mDisassembly->reloadData();
    mMemoryMap->refreshMapSlot();
    GuiUpdateBreakpointsView();
    statusBar()->showMessage(QString("%1 - 0x%2").arg(reason).arg(rip, 0, 16));
    mTabWidget->setCurrentIndex(0);
}

void MainWindow::loadTheme()
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QIcon::setThemeSearchPaths({":/", "qrc:/"});
    QIcon::setThemeName("Default");
    QIcon::setFallbackThemeName("Default");
    QApplication::setWindowIcon(QIcon(":/Default/icons/bug.png"));

    if(QFile styleFile(":/style.qss"); styleFile.open(QFile::ReadOnly))
        qApp->setStyleSheet(styleFile.readAll());

    QColor bg(0x212121);
    QColor bgSecondary(0x313131);
    QColor bgHover(0x414141);
    QColor text(0xe0e0e0);
    QColor accent(0x89a2f6);
    QColor disabled(0x646464);
    QColor border(0x515151);

    QPalette palette;
    palette.setColor(QPalette::Window, bg);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, bgSecondary);
    palette.setColor(QPalette::AlternateBase, bg);
    palette.setColor(QPalette::ToolTipBase, bgHover);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, bg);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::black);
    palette.setColor(QPalette::Light, bg.lighter(120));
    palette.setColor(QPalette::Dark, bg.darker(130));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    QApplication::setPalette(palette);

    ConfigurationPalette p;
    p.background = bg;
    p.darkGrey = border;
    p.lightGrey = bgSecondary;
    p.black = text;

    auto* config = new Configuration(p);

    {
        QFont tableFont = config->Fonts["AbstractTableView"];
        tableFont.setPointSize(9);
        config->Fonts["AbstractTableView"] = tableFont;
        config->Fonts["Disassembly"] = tableFont;
        config->Fonts["HexDump"] = tableFont;
        config->Fonts["Stack"] = tableFont;
        config->Fonts["Registers"] = tableFont;
        config->Fonts["Log"] = tableFont;
    }

    config->Colors["DisassemblyBreakpointColor"] = QColor(static_cast<QRgb>(0x000000));
    config->Colors["DisassemblyBreakpointBackgroundColor"] = QColor(0xFF0000);

    config->Colors["HexDumpByte00Color"] = accent;
    config->Colors["HexDumpByte7FColor"] = QColor(0xe06c75);
    config->Colors["HexDumpByteFFColor"] = QColor(0xe06c75);
    config->Colors["HexDumpByteIsPrintColor"] = QColor(0x98c379);

    config->Colors["RegistersBackgroundColor"] = bg;
    config->Colors["RegistersLabelColor"] = disabled;
    config->Colors["RegistersArgumentLabelColor"] = accent;
    config->Colors["RegistersColor"] = text;
    config->Colors["RegistersModifiedColor"] = QColor(0xFF0000);
    config->Colors["RegistersSelectionColor"] = bgHover;
    config->Colors["RegistersExtraInfoColor"] = disabled;

    config->Colors["StackCspBackgroundColor"] = Qt::transparent;
    config->Colors["StackCspColor"] = QColor(0xA6F93E);
    config->Colors["StackAddressColor"] = QColor(0xA0A0A0);
    config->Colors["StackAddressBackgroundColor"] = Qt::transparent;
    config->Colors["StackSelectedAddressColor"] = text;
    config->Colors["StackSelectedAddressBackgroundColor"] = Qt::transparent;
    config->Colors["StackInactiveTextColor"] = QColor(0xA0A0A0);
    config->Colors["StackSelectionColor"] = bgHover;
    config->Colors["StackReturnToColor"] = QColor(0xF55F86);

    QColor mnemonic(0xc678dd);
    QColor call(0x61afef);
    QColor jump(0x98c379);
    QColor ret(0xe06c75);
    QColor number(0xd19a66);
    QColor reg(0xe06c75);
    QColor comment(0x646464);
    QColor constant(0x56b6c2);

    auto setColorPair = [&](const QString & name, QColor fg, QColor cbg = Qt::transparent)
    {
        config->Colors[name + "Color"] = fg;
        config->Colors[name + "BackgroundColor"] = cbg;
    };

    setColorPair("InstructionComma", text);
    setColorPair("InstructionPrefix", mnemonic);
    setColorPair("InstructionUncategorized", text);
    setColorPair("InstructionAddress", accent);
    setColorPair("InstructionValue", number);
    setColorPair("TraceNewValue", QColor(0xFF0000));
    setColorPair("InstructionMnemonic", mnemonic);
    setColorPair("InstructionPushPop", mnemonic);
    setColorPair("InstructionCall", call);
    setColorPair("InstructionRet", ret);
    setColorPair("InstructionConditionalJump", jump);
    setColorPair("InstructionUnconditionalJump", jump);
    setColorPair("InstructionNop", comment);
    setColorPair("InstructionFar", ret);
    setColorPair("InstructionInt3", ret);
    setColorPair("InstructionUnusual", ret);
    setColorPair("InstructionMemorySize", comment);
    setColorPair("InstructionMemorySegment", constant);
    setColorPair("InstructionMemoryBrackets", text);
    setColorPair("InstructionMemoryStackBrackets", constant);
    setColorPair("InstructionMemoryBaseRegister", reg);
    setColorPair("InstructionMemoryIndexRegister", reg);
    setColorPair("InstructionMemoryScale", number);
    setColorPair("InstructionMemoryOperator", text);
    setColorPair("InstructionGeneralRegister", reg);
    setColorPair("InstructionFpuRegister", constant);
    setColorPair("InstructionMmxRegister", constant);
    setColorPair("InstructionXmmRegister", constant);
    setColorPair("InstructionYmmRegister", constant);
    setColorPair("InstructionZmmRegister", constant);
}
