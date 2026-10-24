idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
                       REQUIRES board_config platform hardware_probe log)
