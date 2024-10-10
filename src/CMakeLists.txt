# Glob all .cc files in the directory
file(GLOB LIB_SOURCES "*.cc")

# Define the debug library
add_library(minnow_debug STATIC ${LIB_SOURCES})

# Define the sanitized library with AddressSanitizer support
add_library(minnow_sanitized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(minnow_sanitized PUBLIC ${SANITIZING_FLAGS} -fsanitize=address)
target_link_options(minnow_sanitized PUBLIC -fsanitize=address)

# Define the optimized library with -O2 optimization flag
add_library(minnow_optimized EXCLUDE_FROM_ALL STATIC ${LIB_SOURCES})
target_compile_options(minnow_optimized PUBLIC "-O2")
