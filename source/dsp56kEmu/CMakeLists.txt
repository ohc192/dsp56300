cmake_minimum_required(VERSION 3.10)
project(dsp56kEmu)

add_library(dsp56kEmu STATIC)

set(SOURCES
agu.cpp agu.h
audio.cpp audio.h
buildconfig.h
assert.cpp assert.h
disasm.cpp disasm.h
dsp.cpp dsp.h dsp_ops.inl
error.cpp error.h
essi.cpp essi.h
esai.cpp esai.h
fastmath.h
hi08.h
instructioncache.cpp instructioncache.h
interrupts.h
logging.cpp logging.h
memory.cpp memory.h
omfloader.cpp omfloader.h
opcodes.cpp opcodes.h
opcodefields.h
opcodeinfo.cpp opcodeinfo.h
opcodetypes.h
ops.cpp ops.h
peripherals.cpp peripherals.h
registers.cpp registers.h
ringbuffer.h
semaphore.h
staticArray.h 
types.cpp types.h
unittests.cpp unittests.h
utils.cpp utils.h
)

target_sources(dsp56kEmu PRIVATE ${SOURCES})
source_group("source" FILES ${SOURCES})

if(WIN32)
	target_link_libraries(dsp56kEmu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../dsp56k/CM56300.lib)
endif()
