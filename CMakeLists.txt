cmake_minimum_required(VERSION 3.19)
project(Hauken LANGUAGES C CXX)

#set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Concurrent Gui Network NetworkAuth SerialPort HttpServer Multimedia Positioning PrintSupport Widgets Mqtt)

qt_standard_project_setup()

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/icons/favicon.rc")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(GitVersion)
get_version_from_git()
project(Hauken VERSION ${PROJECT_VERSION}.0)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
  @ONLY)

configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_SOURCE_DIR}/version.h
)

qt_add_executable(Hauken WIN32 MACOSX_BUNDLE ${app_icon_resource_windows} ${CMAKE_CURRENT_BINARY_DIR}/version.rc

    SimpleMail/SimpleMail
    SimpleMail/emailaddress.cpp SimpleMail/emailaddress.h SimpleMail/emailaddress_p.h
    SimpleMail/mimeattachment.cpp SimpleMail/mimeattachment.h
    SimpleMail/mimecontentformatter.cpp SimpleMail/mimecontentformatter.h
    SimpleMail/mimefile.cpp SimpleMail/mimefile.h
    SimpleMail/mimehtml.cpp SimpleMail/mimehtml.h
    SimpleMail/mimeinlinefile.cpp SimpleMail/mimeinlinefile.h
    SimpleMail/mimemessage.cpp SimpleMail/mimemessage.h SimpleMail/mimemessage_p.h
    SimpleMail/mimemultipart.cpp SimpleMail/mimemultipart.h SimpleMail/mimemultipart_p.h
    SimpleMail/mimepart.cpp SimpleMail/mimepart.h SimpleMail/mimepart_p.h
    SimpleMail/mimetext.cpp SimpleMail/mimetext.h
    SimpleMail/quotedprintable.cpp SimpleMail/quotedprintable.h
    SimpleMail/sender.cpp SimpleMail/sender.h SimpleMail/sender_p.h
    SimpleMail/server.cpp SimpleMail/server.h SimpleMail/server_p.h
    SimpleMail/serverreply.cpp SimpleMail/serverreply.h SimpleMail/serverreply_p.h
    SimpleMail/smtpexports.h
    accesshandler.cpp accesshandler.h
    ai.cpp ai.h
    arduino.cpp arduino.h
    arduinooptions.cpp arduinooptions.h
    autorecorderoptions.cpp autorecorderoptions.h
    cameraoptions.cpp cameraoptions.h
    camerarecorder.cpp camerarecorder.h
    config.cpp config.h
    customplotcontroller.cpp customplotcontroller.h
    datastreambaseclass.cpp datastreambaseclass.h
    emailoptions.cpp emailoptions.h
    generaloptions.cpp generaloptions.h
    geolimit.cpp geolimit.h
    geolimitoptions.cpp geolimitoptions.h
    gnssanalyzer.cpp gnssanalyzer.h
    gnssdevice.cpp gnssdevice.h
    gnssdisplay.cpp gnssdisplay.h
    gnssoptions.cpp gnssoptions.h
    instrumentlist.cpp instrumentlist.h
    iqoptions.cpp iqoptions.h
    led/ledindicator.cpp led/ledindicator.h
    main.cpp
    mainwindow.cpp mainwindow.h
    measurementdevice.cpp measurementdevice.h
    mqtt.cpp mqtt.h
    mqttoptions.cpp mqttoptions.h
    notifications.cpp notifications.h
    oauthfileuploader.cpp oauthfileuploader.h
    oauthoptions.cpp oauthoptions.h
    optionsbaseclass.cpp optionsbaseclass.h
    pmrtablewdg.cpp pmrtablewdg.h
    positionreport.cpp positionreport.h
    positionreportoptions.cpp positionreportoptions.h
    qcustomplot.cpp qcustomplot.h
    read1809data.cpp read1809data.h
    receiveroptions.cpp receiveroptions.h
    restapi.cpp restapi.h
    sdefoptions.cpp sdefoptions.h
    sdefrecorder.cpp sdefrecorder.h
    tcpdatastream.cpp tcpdatastream.h
    traceanalyzer.cpp traceanalyzer.h
    tracebuffer.cpp tracebuffer.h
    typedefs.h
    udpdatastream.cpp udpdatastream.h
    versionupdater.cpp versionupdater.h
    vifstreamtcp.cpp vifstreamtcp.h
    vifstreamudp.cpp vifstreamudp.h
    waterfall.cpp waterfall.h
    network.h network.cpp
    iqplot.h iqplot.cpp
)

target_link_libraries(Hauken PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::NetworkAuth
    Qt6::SerialPort
    Qt6::Multimedia
    Qt6::Positioning
    Qt6::PrintSupport
    Qt6::Widgets
    Qt6::HttpServer
    Qt6::Mqtt
)

if (WIN32)
target_include_directories(Hauken PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/zlib
    ${CMAKE_CURRENT_SOURCE_DIR}/include/quazip
)
elseif(UNIX)
  target_include_directories(Hauken PRIVATE
      /usr/include/opencv4
      #${CMAKE_CURRENT_SOURCE_DIR}/include/zlib
      /usr/include/QuaZip-Qt6-1.4/quazip
  )
endif()

if (WIN32)
target_link_libraries(Hauken PRIVATE
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfftw3-3.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_core490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_dnn490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_highgui490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_imgcodecs490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_imgproc490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_videoio490.lib
    optimized ${CMAKE_CURRENT_SOURCE_DIR}/libs/quazip1-qt6.lib
)

target_link_libraries(Hauken PRIVATE
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfftw3-3.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_core490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_dnn490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_highgui490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_imgcodecs490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_imgproc490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/opencv_videoio490d.lib
    debug ${CMAKE_CURRENT_SOURCE_DIR}/libs/quazip1-qt6d.lib
)
elseif(UNIX)
  target_link_libraries(Hauken PRIVATE
      optimized fftw3
      optimized opencv_core
      optimized opencv_dnn
      optimized opencv_highgui
      optimized opencv_imgcodecs
      optimized opencv_imgproc
      optimized opencv_videoio
      optimized quazip1-qt6
  )

  target_link_libraries(Hauken PRIVATE
      debug fftw3
      debug opencv_core
      debug opencv_dnn
      debug opencv_highgui
      debug opencv_imgcodecs
      debug opencv_imgproc
      debug opencv_videoio
      debug quazip1-qt6
  )
endif()

install(TARGETS Hauken
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Hauken
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})
