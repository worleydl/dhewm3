cmake_minimum_required(VERSION 3.14)
include(FetchContent)

# Setup UWP platform
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define project
project(dhewm3-uwp LANGUAGES CXX)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${PROJECT_NAME})

# Fetch shared UWP deps
FetchContent_Declare(UwpLibs
    GIT_REPOSITORY https://github.com/worleydl/uwp-dep.git
    GIT_TAG 62bdf65ce2aeb6546d76610a75b592bb577cc8ca
)
FetchContent_MakeAvailable(UwpLibs)


# Config for dependency paths
set(DHEWM3CORE CACHE PATH "")
set(DHEWM3DLL CACHE PATH "")
set(DHEWM3LIBS CACHE PATH "")

add_executable(${PROJECT_NAME} WIN32
	main.cpp
)

set(PkgAssets
	Assets/LockScreenLogo.scale-200.png
	Assets/Square44x44Logo.targetsize-24_altform-unplated.png
	Assets/Square44x44Logo.scale-200.png
	Assets/Square150x150Logo.scale-200.png
	Assets/StoreLogo.png
)

set(BinDeps
	${DHEWM3DLL}/base.dll
	${DHEWM3DLL}/d3xp.dll

	${DHEWM3LIBS}/bin/libcurl-4.dll
	${DHEWM3LIBS}/bin/libjpeg-8.dll
	${DHEWM3LIBS}/bin/libogg-0.dll
	${DHEWM3LIBS}/bin/libvorbis-0.dll
	${DHEWM3LIBS}/bin/libvorbisenc-2.dll
	${DHEWM3LIBS}/bin/libvorbisfile-3.dll
	${DHEWM3LIBS}/bin/OpenAL32.dll
	${DHEWM3LIBS}/bin/zlib1.dll

	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/dxil.dll
	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/libgallium_wgl.dll
	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/libglapi.dll
	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/opengl32.dll
	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/SDL2.dll
	${CMAKE_BINARY_DIR}/_deps/uwplibs-src/x64/bin/z-1.dll

	Package.appxmanifest
)

set(BinLibs
	${DHEWM3CORE}/dhewm3.lib
	${DHEWM3CORE}/idlib.lib

	${DHEWM3LIBS}/lib/libcurl.lib
	${DHEWM3LIBS}/lib/openal32.lib
)

source_group("BinDeps" FILES ${BinDeps})
source_group("PkgAssets" FILES ${PkgAssets})

set_source_files_properties(${BinDeps} PROPERTIES
	VS_COPY_TO_OUT_DIR Always
	VS_DEPLOYMENT_CONTENT TRUE
	VS_DEPLOYMENT_LOCATION "."
)

set_source_files_properties(${PkgAssets} PROPERTIES
	#VS_COPY_TO_OUT_DIR Always
	VS_DEPLOYMENT_CONTENT TRUE
	VS_DEPLOYMENT_LOCATION "Assets"
)

target_sources(${PROJECT_NAME} PRIVATE
	${BinDeps}
	${PkgAssets}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${BinLibs} SDL2 WindowsApp.lib OneCore.lib)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.19041.0)
