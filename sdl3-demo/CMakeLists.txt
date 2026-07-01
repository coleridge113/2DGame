set(SDL3_DIR "/opt/homebrew/Cellar/sdl3/3.4.10/lib/cmake/SDL3")
find_package(SDL3 REQUIRED)

add_executable(sdl3-demo "sdl3-demo.cpp")

target_compile_features(sdl3-demo PRIVATE cxx_std_20)
target_link_libraries(sdl3-demo PRIVATE SDL3::SDL3)

add_custom_target(run
    COMMAND sdl3-demo
    DEPENDS sdl3-demo
    COMMENT "Launching 2D Game..."
)
