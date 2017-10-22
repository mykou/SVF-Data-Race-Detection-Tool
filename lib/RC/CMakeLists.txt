##########################
######### librc ##########
##########################
file(GLOB SOURCES
	"StaticAnalysis/*.cpp"
	"RCInstr/*.cpp"
)

add_library(librc STATIC ${SOURCES})

set_target_properties(librc PROPERTIES OUTPUT_NAME "rc")


##########################
####### RTLCommon ########
##########################
file(GLOB SOURCES
	"RTLCommon/*.cpp"
)

add_library(rtlcommon STATIC ${SOURCES})

set_target_properties(rtlcommon PROPERTIES COMPILE_FLAGS "-fPIC")



##########################
######## DCIRTL ##########
##########################
file(GLOB SOURCES
	"DCIRTL/*.cpp"
)

add_library(dcirtl MODULE ${SOURCES})

target_link_libraries(dcirtl rtlcommon)



##########################
######### TSRTL ##########
##########################
file(GLOB SOURCES
	"TSRTL/*.cpp"
)

add_library(tsrtl MODULE ${SOURCES})

target_link_libraries(tsrtl rtlcommon)

