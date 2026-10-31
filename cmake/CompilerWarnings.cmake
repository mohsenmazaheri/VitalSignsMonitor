# -----------------------------------------------------------------------------
#  vsm_set_warnings(<target>)
#  Applies the project's warning policy to one of OUR targets. Third-party code
#  (GoogleTest, Qt) is never compiled with these flags.
# -----------------------------------------------------------------------------
function(vsm_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4             # high warning level
            /permissive-    # standard-conforming mode
            /utf-8          # source and execution charset are UTF-8
            /Zc:__cplusplus # report the real C++ standard in __cplusplus
        )
        if(VSM_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wconversion -Wsign-conversion
            -Wold-style-cast
            -Woverloaded-virtual
        )
        if(VSM_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
