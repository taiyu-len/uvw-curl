add_library(sanitize_address INTERFACE)
target_compile_options(sanitize_address INTERFACE -fsanitize=address)
target_link_options(sanitize_address INTERFACE -fsanitize=address)
