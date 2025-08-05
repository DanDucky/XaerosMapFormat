function(read_uint STRING BEGIN SIZE OUTPUT POSITION_OUTPUT)
    # convert from string index to hex string index
    math(EXPR BEGIN "${BEGIN} * 2")
    math(EXPR SIZE "${SIZE} * 2")

    string(SUBSTRING "${STRING}" ${BEGIN} ${SIZE} DATA)

    math(EXPR VALUE "0x${DATA}")
    math(EXPR NEW_POS "(${BEGIN} / 2) + (${SIZE} / 2)")

    set(${OUTPUT} "${VALUE}" PARENT_SCOPE)
    set(${POSITION_OUTPUT} "${NEW_POS}" PARENT_SCOPE)
endfunction()

function(read_string STRING BEGIN SIZE OUTPUT POSITION_OUTPUT)
    # convert from string index to hex string index
    math(EXPR BEGIN "${BEGIN} * 2")
    math(EXPR SIZE "${SIZE} * 2")

    string(SUBSTRING "${STRING}" ${BEGIN} ${SIZE} VALUE)
    math(EXPR NEW_POS "(${BEGIN} / 2) + (${SIZE} / 2)")

    math(EXPR NUM_BYTES "${SIZE} / 2 - 1")

    set(OUTPUT_STRING "")

    foreach (I RANGE 0 ${NUM_BYTES})
        math(EXPR CURRENT_POS "${BEGIN} / 2 + ${I}")
        read_uint("${STRING}" ${CURRENT_POS} 1 CHAR _)

        string(ASCII "${CHAR}" CHAR)
        string(APPEND OUTPUT_STRING ${CHAR})
    endforeach ()

    set(${OUTPUT} "${OUTPUT_STRING}" PARENT_SCOPE)
    set(${POSITION_OUTPUT} "${NEW_POS}" PARENT_SCOPE)
endfunction()
