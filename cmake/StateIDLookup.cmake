message(STATUS "Generating legacy lookups...")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ByteInputUtils.cmake)

set(MOD_JAR "${WORKING_DIRECTORY}/mod.jar")
set(MOD_EXTRACTED "${WORKING_DIRECTORY}/mod_extracted")

if (NOT EXISTS ${MOD_JAR})
    file (DOWNLOAD
            "https://cdn.modrinth.com/data/NcUtCpym/versions/3FwkAdEs/XaerosWorldMap_1.29.17_Forge_1.14.4.jar"
            "${MOD_JAR}"
            STATUS MOD_STATUS
    )

    if (NOT MOD_STATUS EQUAL 0)
        file(REMOVE "${MOD_JAR}")
        message(FATAL_ERROR "Failed to download mod from https://cdn.modrinth.com/data/1bokaNcj/versions/StqWcPqA/Xaeros_Minimap_25.2.12_Fabric_1.21.8.jar")
    endif ()
endif ()

if (NOT EXISTS ${MOD_EXTRACTED})
    file(ARCHIVE_EXTRACT
            INPUT ${MOD_JAR}
            DESTINATION ${MOD_EXTRACTED}
    )
endif()

# I understand this fucking sucks!!! HOWEVER this is so that the project is BUILDABLE and USABLE
# without building with python & java!!! When you compile without java and python
# you can build a much smaller binary much faster which can still read and write xaero files, it just can't
# render them out. Sometimes this is enough, especially when integrating with a larger system which already
# has color information or something of the sort. I shouldn't need exterior build deps if I don't need them, and
# this is one of those cases where I technically didn't need them like I did with java and python, and so... enjoy

file(READ "${MOD_EXTRACTED}/assets/xaeroworldmap/vanilla_states.dat" VANILLA_STATES HEX)

set(POSITION 0)
set(STATE_INFO "")
set(META_INFO "")
set(LAST_ID -1)
string(LENGTH "${VANILLA_STATES}" VANILLA_LENGTH)
math(EXPR VANILLA_LENGTH "${VANILLA_LENGTH} / 2")

while (NOT ${POSITION} GREATER_EQUAL ${VANILLA_LENGTH})
    read_uint("${VANILLA_STATES}" ${POSITION} 4 ID POSITION)

    math(EXPR BLOCK_ID "${ID} & 4095")
    math(EXPR META_ID "${ID} >> 12 & 1048575")

    math(EXPR SKIPPED "${BLOCK_ID} - (${LAST_ID} + 1)")

    if ((NOT "${META_INFO}" STREQUAL "") AND NOT ${LAST_ID} EQUAL ${BLOCK_ID}) # push last meta_info into state_info
        list(JOIN META_INFO ",\n" META_INFO_JOINED)
        set(META_INFO "")
        list(APPEND STATE_INFO "{${META_INFO_JOINED}}")

        if (NOT ${SKIPPED} LESS_EQUAL 0)
            string(REPEAT "{};" ${SKIPPED} PADDING)
            string(REGEX REPLACE ".$" "" PADDING "${PADDING}")
            list(APPEND STATE_INFO "${PADDING}")
        endif ()
    endif ()

    set(LAST_ID ${BLOCK_ID})

    math(EXPR POSITION "${POSITION} + 3") # skip over initial tag

    math(EXPR POSITION "${POSITION} + 1") # skip over tag type

    read_uint("${VANILLA_STATES}" ${POSITION} 2 TAG_SIZE POSITION)

    read_string("${VANILLA_STATES}" ${POSITION} ${TAG_SIZE} TAG_NAME POSITION)

    set(PROPERTIES "")

    if ("${TAG_NAME}" STREQUAL "Properties")
        while(1 EQUAL 1)
            read_uint("${VANILLA_STATES}" ${POSITION} 1 TAG_TYPE POSITION)
            if (TAG_TYPE EQUAL 0) # end of properties
                break()
            endif ()

            read_uint("${VANILLA_STATES}" ${POSITION} 2 TAG_SIZE POSITION)

            read_string("${VANILLA_STATES}" ${POSITION} ${TAG_SIZE} TAG_NAME POSITION)

            read_uint("${VANILLA_STATES}" ${POSITION} 2 VALUE_SIZE POSITION)

            read_string("${VANILLA_STATES}" ${POSITION} ${VALUE_SIZE} VALUE POSITION)

            list(APPEND PROPERTIES "{\"${TAG_NAME}\",\"${VALUE}\"}")
        endwhile ()

        math(EXPR POSITION "${POSITION} + 1 + 2 + 4") # skip over known info

    endif ()

    read_uint("${VANILLA_STATES}" ${POSITION} 2 NAME_SIZE POSITION)

    read_string("${VANILLA_STATES}" ${POSITION} ${NAME_SIZE} BLOCK_NAME POSITION)

    string(REPLACE "minecraft:" "" BLOCK_NAME "${BLOCK_NAME}")

    math(EXPR POSITION "${POSITION} + 1") # skip over known end bytes

    string(LENGTH "${PROPERTIES}" PROPERTIES_LENGTH)
    if (${PROPERTIES_LENGTH} GREATER 0)
        list(JOIN PROPERTIES "," PROPERTIES)
    endif ()

    list(APPEND META_INFO "BlockState{\"${BLOCK_NAME}\",nbt::tag_compound{${PROPERTIES}}}")

endwhile ()

if (NOT "${META_INFO}" STREQUAL "") # push last meta_info into state_info
    list(JOIN META_INFO ",\n" META_INFO_JOINED)
    list(APPEND STATE_INFO "{${META_INFO_JOINED}}")
endif ()

list(JOIN STATE_INFO ",\n" STATE_INFO)

set(STATE_ID_CONTENT
        "#include \"lookups/LegacyCompatibility.hpp\"
namespace xaero {

const StateIDLookup stateIDLookup = {${STATE_INFO}}\;const std::size_t stateIDLookupSize = ${BLOCK_ID} + 1\;}")

file(WRITE "${STATE_ID_LOOKUP}" ${STATE_ID_CONTENT})

message(STATUS "Generated legacy lookups")
