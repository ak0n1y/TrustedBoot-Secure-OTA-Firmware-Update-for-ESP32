idf_component_register(
    SRCS "custom_verify.c"
    INCLUDE_DIRS "include"
    REQUIRES bootloader bootloader_support
)

set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-u bootloader_hooks_include")
