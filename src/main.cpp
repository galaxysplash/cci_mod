#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stack>

int main(int argc, const char *argv[]) {
  if (argc > 1) {
    const auto project_path = std::filesystem::current_path();

    for (size_t argi = 1; argi < argc; ++argi) {
      const auto mod_name = argv[argi];

      const std::filesystem::path mod_path =
          std::filesystem::current_path() / "src" / mod_name;

      if (std::filesystem::create_directory(mod_path)) {
        const auto src_path = mod_path / "src",
                   include_path = mod_path / "include";

        std::filesystem::create_directory(src_path);
        std::filesystem::create_directory(include_path);

        const auto cmake_lists_content{std::string{
                                           R"(cmake_minimum_required(VERSION 3.28)
project()"} + mod_name + std::string{R"( VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
      )"}};

        std::ofstream file;
        file.open(mod_path / "CMakeLists.txt");
        file << cmake_lists_content.c_str();
        file.close();

        std::stack<std::filesystem::path> directories;

        std::filesystem::path current_path = mod_path;

        while (true) {
          if (std::filesystem::exists(current_path / "CMakeLists.txt")) {
            directories.push(current_path.filename());
            current_path = current_path.parent_path().parent_path();
          } else {
            break;
          }
        }

        std::stack<std::filesystem::path> directories_copy = directories;
        std::stack<std::filesystem::path> directories_second_copy = directories;

        std::filesystem::path current_include_namespace_path = include_path;

        for (;!directories.empty(); directories.pop()) {
          current_include_namespace_path /= directories.top();
          std::filesystem::create_directory(current_include_namespace_path);
        }
        std::stringstream stream;

        const auto project_cmakelists_txt = project_path / "CMakeLists.txt";
        std::string previous_cmakelists_txt_content;
        std::ifstream cmake_read_lib_stream;
        cmake_read_lib_stream.open(project_cmakelists_txt);
        char cmake_read_char;
        while (true) {
          cmake_read_char = cmake_read_lib_stream.get();
          if (!cmake_read_lib_stream.eof()) {
          previous_cmakelists_txt_content.push_back(cmake_read_char);
          }
          else {
            break;
          }
        }

        cmake_read_lib_stream.close();
        std::ofstream cmake_add_lib_stream;

        cmake_add_lib_stream.open(project_cmakelists_txt);
        cmake_add_lib_stream << previous_cmakelists_txt_content << "\n\n# " << mod_name
                             << "\nadd_subdirectory(src/" << mod_name << ")\n"
                             << "target_link_libraries(${PROJECT_NAME} PRIVATE "
                             << mod_name << ")";
        cmake_add_lib_stream.close();
        std::stringstream namespace_openings, low_spacer, spacer, namespace_closings;

        {
          size_t opening_count = 0;
          for (;directories_second_copy.size() > 1; directories_second_copy.pop()) {
            namespace_openings << "namespace " << directories_second_copy.top().c_str() << " {\n";

            for (size_t i = 0; i < opening_count + 1; ++i) {
              namespace_openings << "  ";
            }

            ++opening_count;
          }
          for (size_t i = 0; i < opening_count; ++i) {
            low_spacer << "  ";
          }
          for (size_t i = 0; i < opening_count + 1; ++i) {
            spacer << "  ";
          }
          for (; opening_count > 0; --opening_count) {
            for (size_t i = 0; i < opening_count - 1; ++i) {
              namespace_closings << "  ";
            }
            namespace_closings << "}\n";
          }
        }
        std::string class_name_with_underscores, class_name;
        for (size_t i = 0; i < std::string_view{mod_name}.size(); ++i) {
          if (i == 0) {
            class_name_with_underscores +=
              static_cast<char>(std::toupper(std::string_view{mod_name}[i]));
          } else {
            class_name_with_underscores += std::string_view{mod_name}[i];
          }
        }
        {
          bool previous_underscore = false;
          for (const auto &e : class_name_with_underscores) {
            if (previous_underscore) {
              class_name += static_cast<char>(std::toupper(e));
              previous_underscore = false;
              continue;
            }

            if (e == '_') {
              previous_underscore = true;
            }
            else {
              class_name += e;
            }
          }
        }

        std::ofstream mod_header_stream;
        const auto mod_header_path = (current_include_namespace_path / class_name).string() + ".h";

        mod_header_stream.open(mod_header_path);

        mod_header_stream << "#pragma once\n\n"
        << namespace_openings.str()  << "class " << class_name << " {\n" << low_spacer.str() << "public:\n" << spacer.str() << "\n" << low_spacer.str() << "};\n" << namespace_closings.str();
        mod_header_stream.close();
        std::ofstream mod_cpp_stream;
        const std::string src_file_path = (src_path / class_name).string() + ".cpp";

        std::stringstream non_absolute_include_path;
        for (;!directories_copy.empty(); directories_copy.pop()) {
          non_absolute_include_path << directories_copy.top().string() << "/";
        }
        non_absolute_include_path << class_name << ".h";
        mod_cpp_stream.open(src_file_path);
        mod_cpp_stream << "#include <" << non_absolute_include_path.str() << ">\n\n" << namespace_openings.str() << "\n" << namespace_closings.str();
      }
    }
  }
  else {
    std::cout << "give your module a name, e.g. with 'cci_mod xyz'.";
  }
}