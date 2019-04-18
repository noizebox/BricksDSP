add_executable(synth_voice synth_voice.cpp)
target_link_libraries(synth_voice sndfile bricks_dsp)

add_executable(fixture fixture.cpp)
target_link_libraries(fixture sndfile bricks_dsp)

add_executable(jack_fixture jack_fixture.cpp)
target_link_libraries(jack_fixture jack bricks_dsp)