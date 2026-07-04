# Desktop application entry component.
#
# `main` logs the compiled board profile for the scaffold and depends only on
# the board contract.
idf_component_register(
    SRCS "ff_main.c"
    INCLUDE_DIRS "."
    REQUIRES board
)
