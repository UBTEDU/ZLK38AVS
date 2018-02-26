add_executable(SampleApp 
    main.cpp 
    PortAudioMicrophoneWrapper.cpp 
    InteractionManager.cpp 
    UserInputManager.cpp
    ConsolePrinter.cpp
    GuiRenderer.cpp
    UIManager.cpp
    KeywordObserver.cpp
    ConnectionObserver.cpp
    SampleApplication.cpp
    I2cLib.cpp
    LedDriver.cpp
    Zle38Avs.cpp)

target_include_directories(SampleApp PUBLIC 
    "${SampleApp_SOURCE_DIR}/include"
    "${MediaPlayer_SOURCE_DIR}/include"
    "${DefaultClient_SOURCE_DIR}/include"
    "${AudioResources_SOURCE_DIR}/include"
    "${PORTAUDIO_INCLUDE_DIR}")

include_directories(
    "${CMAKE_SOURCE_DIR}/../../../vproc_sdk/drivers/hbi/inc"
    "${CMAKE_SOURCE_DIR}/../../../vproc_sdk/include"
    "${CMAKE_SOURCE_DIR}/../../../vproc_sdk/platform/raspberry/include"
)

set(hbi_libs
    "${CMAKE_SOURCE_DIR}/../../../vproc_sdk/libs/hbi_u.o"
    "${CMAKE_SOURCE_DIR}/../../../vproc_sdk/libs/hbi_tw.o"
)

target_link_libraries(SampleApp 
    DefaultClient
    AuthDelegate 
    MediaPlayer 
    ${hbi_libs} 
    "${PORTAUDIO_LIB_PATH}")

if(KITTAI_KEY_WORD_DETECTOR)
    target_link_libraries(SampleApp 
        KITTAI)
endif()

if(SENSORY_KEY_WORD_DETECTOR)
    target_link_libraries(SampleApp SENSORY)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(SampleApp
        "-framework CoreAudio" 
        "-framework AudioToolbox" 
        "-framework AudioUnit" 
        "-framework CoreServices" 
        "-framework Carbon")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(SampleApp
        rt m pthread asound)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(FATAL_ERROR "Windows is unsupported at the moment")  
endif()
