cmake_minimum_required(VERSION 3.20)

project("Game of life")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY Game_of_life )
set(SDL3 ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3)



if(NOT SDL3)
    message(FATAL_ERROR "Please, specify SDL3 directory via -SDL3=<dir>")
endif()
    message(STATUS "SDL3 dir is ${SDL3}, change via -SDL3=<dir>")

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

add_executable(Game_of_life) # WIN32 - hide the console like this add_executable(app WIN32)

target_sources(Game_of_life
PRIVATE
    main.cpp
)

target_include_directories(Game_of_life PUBLIC "${SDL3}/include")

# В команде target_link_libraries нужно указать полный путь к файлу библиотеки
if(EXISTS "${SDL3}/lib/libSDL3.dll.a")
    target_link_libraries(Game_of_life "${SDL3}/lib/libSDL3.dll.a" ${SYSTEM_LIBS})
else()
    target_link_libraries(Game_of_life "$SDL3}/lib/libSDL3_test.a" ${SYSTEM_LIBS})
endif() 


target_compile_definitions(Game_of_life PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_options(Game_of_life PRIVATE -static)

add_custom_command(TARGET Game_of_life
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different  "${SDL3}/bin/SDL3.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    VERBATIM
   
    COMMENT "This command will be add SDL3.dll"
    
)

#POST_BUILD

#cmake -G "MinGW Makefiles" -S . -B build - SDL3=D:/Cpp_projects/game_of_life/SDL_3S
#cmake -G "MinGW Makefiles" -S . -B dist SDL3 = D:\Cpp_projects\game_of_life\SDL_3