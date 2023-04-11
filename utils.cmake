# Various CMake utilities.


# Strips the suffix from the given path (given that it matches).
# The output variable is untouched, if the suffix does not match the given path.
#
# @arg initialPath - the path to strip the suffix from.
# @arg suffix - path suffix (only makes sense when it's relative).
# @arg outVarName - name of the variable to write the result to.
function(strip_path_suffix initialPath suffix outVarName)
  while(TRUE)
    cmake_path(GET suffix FILENAME suffixFilename)
    cmake_path(GET initialPath FILENAME pathFilename)

    if (NOT suffixFilename)
      break()
    endif()
    if (NOT (suffixFilename STREQUAL pathFilename))
      return()
    endif()

    cmake_path(GET initialPath PARENT_PATH initialPath)
    cmake_path(GET suffix PARENT_PATH suffix)
  endwhile()

  set(${outVarName} "${initialPath}" PARENT_SCOPE)
endfunction()
