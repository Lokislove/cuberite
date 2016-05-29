cmake_minimum_required (VERSION 2.8.7)
project(Cuberite)
include(ExternalProject)

externalproject_add(lua_native
	SOURCE_DIR ../lib/lua
	CMAKE_GENERATOR ${CMAKE_GENERATOR}
	CMAKE_ARGS -DANDROID_CROSSCOMPILE=TRUE
	BUILD_COMMAND
		${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
	INSTALL_COMMAND ""
)
externalproject_add(tolua_native
	SOURCE_DIR ../lib/tolua++
	CMAKE_GENERATOR ${CMAKE_GENERATOR}
	BUILD_COMMAND
		${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
	INSTALL_COMMAND ""
)

# Tell libevent that Android has pthread (not auto detected due to NDK implementation problems)
# Tell Lua to not use unimplemented Android locale features
add_definitions(-Dgetlocaledecpoint\(\)="'.'")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # Add c++14 support here since we're not calling set_flags()

add_library(AndroidNative jni/app-android.cpp)
add_dependencies(tolua_native lua_native)
add_dependencies(AndroidNative tolua_native)
target_link_libraries(AndroidNative log dl)

include_directories(
	../lib/
	../src/
	../lib/jsoncpp/include/
	../lib/polarssl/include/
	../lib/sqlitecpp/include/
	../lib/sqlitecpp/sqlite3/
	../lib/libevent/include/
)

# (Hopefully) temporary workaround for issue described in https://github.com/taka-no-me/android-cmake/issues/81
if("${ANDROID_STL}" STREQUAL c++_static)
	set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY} -latomic")
endif()

# Probably not the recommended method, but other ways don't seem to generate libraries in libs/armeabi
add_subdirectory(../ Cuberite)

get_directory_property(BINDING_OUTPUTS DIRECTORY ../src/Bindings DEFINITION BINDING_OUTPUTS)
ADD_CUSTOM_COMMAND(
		OUTPUT ${BINDING_OUTPUTS}

		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tolua_native-prefix/src/tolua_native-build/Release/tolua -L BindingsProcessor.lua -o  Bindings.cpp -H  Bindings.h AllToLua.pkg
		WORKING_DIRECTORY ../src/Bindings
)
add_custom_target(BindingDependencies ALL DEPENDS ${BINDING_OUTPUTS})
add_dependencies(AndroidNative BindingDependencies)
