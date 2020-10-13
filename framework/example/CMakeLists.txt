add_executable(FrameworkExample "main.cpp")
target_link_libraries(FrameworkExample PRIVATE Framework)
target_compile_features(FrameworkExample PRIVATE cxx_std_17) # C++17
