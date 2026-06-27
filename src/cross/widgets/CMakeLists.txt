set(widgets_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../gui/Src")

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zydis_wrapper
    ${CMAKE_CURRENT_BINARY_DIR}/zydis_wrapper
)

add_subdirectory(
    ${widgets_SOURCE_DIR}/ThirdPartyLibs/md4c
    ${CMAKE_CURRENT_BINARY_DIR}/md4c
)

include(Qt.cmake)

set(hook_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bridgelist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MagicMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MiscUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiscUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Imports.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RegisterContext.h
)

set(widgets_SOURCES
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.h
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.h
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.cpp
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.h
    ${widgets_SOURCE_DIR}/BasicView/HexDump.cpp
    ${widgets_SOURCE_DIR}/BasicView/HexDump.h
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.cpp
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.h
    ${widgets_SOURCE_DIR}/BasicView/StdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/StdTable.h
    ${widgets_SOURCE_DIR}/BasicView/StdIconTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/StdIconTable.h
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.cpp
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.h
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.cpp
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.h
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.cpp
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.h
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.h
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.ui
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.cpp
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.ui
    ${widgets_SOURCE_DIR}/Gui/TypeWidget.cpp
    ${widgets_SOURCE_DIR}/Gui/TypeWidget.h
    ${widgets_SOURCE_DIR}/Gui/WordEditDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/WordEditDialog.h
    ${widgets_SOURCE_DIR}/Gui/WordEditDialog.ui
    ${widgets_SOURCE_DIR}/Gui/RichTextItemDelegate.cpp
    ${widgets_SOURCE_DIR}/Gui/RichTextItemDelegate.h
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.h
    ${widgets_SOURCE_DIR}/Gui/ReleaseNotesDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ImageTextBrowser.cpp
    ${widgets_SOURCE_DIR}/Gui/ImageTextBrowser.h
    ${widgets_SOURCE_DIR}/Gui/MemoryMapView.cpp
    ${widgets_SOURCE_DIR}/Gui/MemoryMapView.h
    ${widgets_SOURCE_DIR}/Gui/PageMemoryRights.cpp
    ${widgets_SOURCE_DIR}/Gui/PageMemoryRights.h
    ${widgets_SOURCE_DIR}/Gui/PageMemoryRights.ui
    ${widgets_SOURCE_DIR}/Gui/VirtualModDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/VirtualModDialog.h
    ${widgets_SOURCE_DIR}/Gui/VirtualModDialog.ui
    ${widgets_SOURCE_DIR}/Gui/GotoDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/GotoDialog.h
    ${widgets_SOURCE_DIR}/Gui/GotoDialog.ui
    ${widgets_SOURCE_DIR}/Gui/HexEditDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/HexEditDialog.h
    ${widgets_SOURCE_DIR}/Gui/HexEditDialog.ui
    ${widgets_SOURCE_DIR}/Gui/HexLineEdit.cpp
    ${widgets_SOURCE_DIR}/Gui/HexLineEdit.h
    ${widgets_SOURCE_DIR}/Gui/HexLineEdit.ui
    ${widgets_SOURCE_DIR}/Gui/CodepageSelectionDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/CodepageSelectionDialog.h
    ${widgets_SOURCE_DIR}/Gui/CodepageSelectionDialog.ui
    ${widgets_SOURCE_DIR}/Gui/BreakpointsView.cpp
    ${widgets_SOURCE_DIR}/Gui/BreakpointsView.h
    ${widgets_SOURCE_DIR}/Gui/EditBreakpointDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/EditBreakpointDialog.h
    ${widgets_SOURCE_DIR}/Gui/EditBreakpointDialog.ui
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.cpp
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.h
    ${widgets_SOURCE_DIR}/Utils/ActionHelpers.h
    ${widgets_SOURCE_DIR}/Utils/CachedFontMetrics.h
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.cpp
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.h
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.cpp
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.h
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.cpp
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.h
    ${widgets_SOURCE_DIR}/Utils/MethodInvoker.h
    ${widgets_SOURCE_DIR}/Utils/HexValidator.cpp
    ${widgets_SOURCE_DIR}/Utils/HexValidator.h
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.cpp
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.h
    ${widgets_SOURCE_DIR}/Utils/ValidateExpressionThread.cpp
    ${widgets_SOURCE_DIR}/Utils/ValidateExpressionThread.h
    ${widgets_SOURCE_DIR}/Utils/VaHistory.h
    ${widgets_SOURCE_DIR}/Utils/SymbolAutoCompleteModel.cpp
    ${widgets_SOURCE_DIR}/Utils/SymbolAutoCompleteModel.h
    ${widgets_SOURCE_DIR}/Utils/Breakpoints.cpp
    ${widgets_SOURCE_DIR}/Utils/Breakpoints.h
    ${widgets_SOURCE_DIR}/Gui/RegistersView.cpp
    ${widgets_SOURCE_DIR}/Gui/RegistersView.h
    ${widgets_SOURCE_DIR}/Accessible/AccessibleRegistersView.cpp
    ${widgets_SOURCE_DIR}/Accessible/AccessibleRegistersView.h
    ${widgets_SOURCE_DIR}/QHexEdit/ArrayCommand.cpp
    ${widgets_SOURCE_DIR}/QHexEdit/ArrayCommand.h
    ${widgets_SOURCE_DIR}/QHexEdit/QHexEdit.cpp
    ${widgets_SOURCE_DIR}/QHexEdit/QHexEdit.h
    ${widgets_SOURCE_DIR}/QHexEdit/QHexEditPrivate.cpp
    ${widgets_SOURCE_DIR}/QHexEdit/QHexEditPrivate.h
    ${widgets_SOURCE_DIR}/QHexEdit/XByteArray.cpp
    ${widgets_SOURCE_DIR}/QHexEdit/XByteArray.h
)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# These two canonical sources live in src/gui/Src/Utils, so their #include "StringUtil.h" /
# "MiscUtil.h" resolve to the canonical Utils headers sitting beside them: for a quoted include the
# compiler searches the including file's own directory first, which outranks even the BEFORE -I
# above that points every other TU at the cross shim headers. Those canonical headers redeclare the
# same free functions as the cross shim's StringUtil.h/MiscUtil.h. Compiled alone that is fine, but
# a unity batch merges these files with shim-using TUs, landing both copies in one TU and causing
# duplicate-declaration errors. Keep them out of unity so each sees only one header.
set_source_files_properties(
    ${widgets_SOURCE_DIR}/Utils/Breakpoints.cpp
    ${widgets_SOURCE_DIR}/Utils/SymbolAutoCompleteModel.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

add_library(x64dbg_widgets STATIC
    ${hook_SOURCES}
    ${widgets_SOURCES}
    resources.qrc
)
add_library(x64dbg::widgets ALIAS x64dbg_widgets)

target_include_directories(x64dbg_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${widgets_SOURCE_DIR}
    ${widgets_SOURCE_DIR}/BasicView
    ${widgets_SOURCE_DIR}/Disassembler
    ${widgets_SOURCE_DIR}/Gui
    ${widgets_SOURCE_DIR}/Utils
    ${widgets_SOURCE_DIR}/QHexEdit
)

source_group(TREE ${widgets_SOURCE_DIR} FILES ${widgets_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX hook FILES ${hook_SOURCES})

target_compile_definitions(x64dbg_widgets PRIVATE
    QT_NO_DEPRECATED_WARNINGS
)

target_link_libraries(x64dbg_widgets PUBLIC
    ${QT_LIBRARIES}
    zydis_wrapper
    md4c-html
)

# https://doc.qt.io/qt-6/wasm.html#asyncify
if(EMSCRIPTEN)
    #target_link_options(${CMKR_TARGET} PUBLIC -sASYNCIFY -Os)
endif()
