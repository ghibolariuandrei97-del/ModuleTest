cmake_minimum_required(VERSION 3.15)
project(mymodule CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

add_library(mymodule SHARED sdk/amxxmodule.cpp hudsprites.cpp)

target_include_directories(mymodule PRIVATE
  sdk
  deps/amxmodx/public
  deps/amxmodx/public/amtl
  deps/hlsdk/common
  deps/hlsdk/dlls
  deps/hlsdk/engine
  deps/hlsdk/pm_shared
  deps/hlsdk/public
  deps/metamod-hl1/metamod)

target_compile_definitions(mymodule PRIVATE WIN32 HAVE_STDINT_H _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
target_compile_options(hudsprites PRIVATE /W0)

set_target_properties(hudsprites PROPERTIES PREFIX "" OUTPUT_NAME "hudsprites_amxx")
