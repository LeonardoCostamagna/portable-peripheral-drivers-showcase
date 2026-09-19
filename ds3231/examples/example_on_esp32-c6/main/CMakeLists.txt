# Register the 'main' component and directly include the driver's source code and header files
idf_component_register(
    SRCS 
        "main.c"
        "../../../src/ds3231.c"
    INCLUDE_DIRS 
        "."
        "../../../inc" 
)