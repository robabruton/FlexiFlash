# Handheld application entry component.
#
# `main` logs the compiled board profile and build limits for the scaffold.
idf_component_register(
    SRCS "ff_main.c"
    INCLUDE_DIRS "."
    REQUIRES board config
)
