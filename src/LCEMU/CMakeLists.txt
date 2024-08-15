add_library(LCEMU
	ext_main.h
	ext_main.cpp
	gui/key_sequence.h
	gui/key_sequence.cpp
	utils.h
	utils.cpp
	gui/gui_utils.h
	gui/gui_utils.cpp
)

set_property(TARGET LCEMU PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(LCEMU PUBLIC "../")

target_link_libraries(LCEMU PRIVATE
	CemuAudio
	CemuCafe
	CemuCommon
	CemuComponents
	CemuConfig
	CemuInput
	CemuResource
	CemuGui
	CemuUtil
	Boost::headers
)