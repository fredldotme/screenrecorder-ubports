set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(PLUGIN "LomiriScreenRecordController")

ADD_DEFINITIONS(-DQT_NO_DEBUG)

set(
    SRC
    captures/capture.h
    encoders/encoder.h
    muxers/mux.h
    plugin.cpp
    controller.cpp
    buffer.cpp
    bufferqueue.cpp
    encoders/android_h264.cpp
    captures/mir.cpp
    muxers/mp4.cpp
    screen_recorder.cpp
    indicator.cpp
)

set(CMAKE_AUTOMOC ON)

include_directories(
  ${HYBRIS_MEDIA_INCLDUE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
  ${MIRCLIENT_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${LOMIRIURLDISPATCHER_INCLUDE_DIRS}
)


add_library(${PLUGIN} MODULE ${SRC})
set_target_properties(${PLUGIN} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGIN})
target_link_libraries(${PLUGIN}
  ${HYBRIS_MEDIA_LDFLAGS}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${MIRCLIENT_LDFLAGS}
  ${MIRCLIENT_LIBRARIES}
  ${GLIB_LDFLAGS}
  ${GLIB_LIBRARIES}
  ${LOMIRIURLDISPATCHER_LDFLAGS}
  ${LOMIRIURLDISPATCHER_LIBRARIES}
  ${AVCODEC_LDFLAGS}
  ${AVCODEC_LIBRARIES}
  avcodec
)
qt5_use_modules(${PLUGIN} Qml Quick DBus Multimedia)

execute_process(
    COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
    OUTPUT_VARIABLE ARCH_TRIPLET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(TARGETS ${PLUGIN} DESTINATION ${QT_IMPORTS_DIR}/${PLUGIN}/)
install(FILES qmldir DESTINATION ${QT_IMPORTS_DIR}/${PLUGIN}/)
