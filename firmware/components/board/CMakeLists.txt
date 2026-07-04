# Board component - hardware identity, capability reporting, and pin-map
# selection. The single source of truth for per-board facts.
#
# Exactly one profile source compiles, chosen by the Kconfig board selection.
# Downstream code reads capabilities from board.h and never hardcodes a form
# factor or a raw GPIO number.
set(board_srcs "")

if(CONFIG_FF_BOARD_HANDHELD)
    list(APPEND board_srcs "profiles/board_handheld.c")
elseif(CONFIG_FF_BOARD_DESKTOP)
    list(APPEND board_srcs "profiles/board_desktop.c")
endif()

idf_component_register(
    SRCS ${board_srcs}
    INCLUDE_DIRS "include"
)
