##################
#  Source Files  #
##################

set(SYNTH_VOICE_SOURCE_FILES synth_voice.cpp)
add_executable(synth_voice ${SYNTH_VOICE_SOURCE_FILES})
target_link_libraries(synth_voice sndfile bricks_dsp)

set(FIXTURE_SOURCE_FILES fixture.cpp)
add_executable(fixture ${FIXTURE_SOURCE_FILES})
target_link_libraries(fixture sndfile bricks_dsp)

set(JACK_FIXTURE_SOURCE_FILES jack_fixture.cpp)
add_executable(jack_fixture ${JACK_FIXTURE_SOURCE_FILES})
target_link_libraries(jack_fixture jack bricks_dsp)