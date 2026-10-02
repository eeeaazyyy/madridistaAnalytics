function(madridista_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(${target} PRIVATE
                -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror)
    endif()
endfunction()
