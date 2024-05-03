cmake_minimum_required(VERSION 3.0.0)

project(TestCrsfProto)

set (USER_SRC
    ../../Lib/Crsf/Crsf.c
    ../../Lib/Crsf/crc.c
    ../../Lib/Crsf/Crsf.h
    ../../Lib/Crsf/CrsfInternal.h
    main.c
)

set (USER_PATH
    ../../Lib/Crsf
)

include_directories(${USER_PATH})

add_executable(${PROJECT_NAME} ${USER_SRC})