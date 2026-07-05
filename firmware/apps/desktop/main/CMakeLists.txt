# Desktop application entry component.
#
# `main` logs the compiled board profile and shared firmware contracts.
idf_component_register(
    SRCS "ff_main.c"
    INCLUDE_DIRS "."
    REQUIRES board config target_db
)
