# download lib
set(BUILD_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

# ImGui
message(STATUS "Setup imgui")
add_library(imgui STATIC)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG 96839b445e32e46d87a44fd43a9cdd60c806f7e1 # the latest commit in the branch `docking`
  SOURCE_DIR ${BUILD_LIB_DIR}/imgui
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_INCLUDE ${BUILD_LIB_DIR}/imgui/include) # copy necessary files to ./include
file(MAKE_DIRECTORY ${IMGUI_INCLUDE})
set(_imgui_src_list imconfig.h imgui.cpp imgui.h imgui_draw.cpp backends/imgui_impl_glfw.h
  backends/imgui_impl_glfw.cpp backends/imgui_impl_vulkan.cpp backends/imgui_impl_vulkan.h imgui_internal.h
  imgui_tables.cpp imgui_widgets.cpp imstb_rectpack.h imstb_textedit.h imstb_truetype.h)

foreach(orig IN LISTS _imgui_src_list)
  get_filename_component(dest ${orig} NAME)
  file(COPY_FILE ${BUILD_LIB_DIR}/imgui/${orig} ${IMGUI_INCLUDE}/${dest} ONLY_IF_DIFFERENT)
  target_sources(imgui PRIVATE ${IMGUI_INCLUDE}/${dest})
endforeach()

target_include_directories(${LIB_NAME} PUBLIC ${IMGUI_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC imgui)

# tiny gltf
message(STATUS "Setup tinygltf")
FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.8.20
  SOURCE_DIR ${BUILD_LIB_DIR}/tinygltf
)
FetchContent_MakeAvailable(tinygltf)
set(TINYGLTF_INCLUDE ${BUILD_LIB_DIR}/tinygltf)
target_include_directories(${LIB_NAME} PUBLIC ${TINYGLTF_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC tinygltf)
target_compile_definitions(${LIB_NAME} PUBLIC "TINYGLTF_USE_CPP14")
target_compile_options(tinygltf PUBLIC -Wno-missing-field-initializers)

# tiny exr
message(STATUS "Setup tinyexr")
FetchContent_Declare(
  tinyexr
  GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
  GIT_TAG 6b8b66f9caf887f8d05e0b7a7e48e0343a5b3d62
  SOURCE_DIR ${BUILD_LIB_DIR}/tinyexr
)
FetchContent_MakeAvailable(tinyexr)
set(TINYEXR_INCLUDE ${BUILD_LIB_DIR}/tinyexr)
target_include_directories(${LIB_NAME} PUBLIC ${TINYEXR_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC tinyexr)

# spdlog
message(STATUS "Setup spdlog")
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
  SOURCE_DIR ${BUILD_LIB_DIR}/spdlog
)
FetchContent_MakeAvailable(spdlog)
set(SPDLOG_INCLUDE ${BUILD_LIB_DIR}/spdlog/include)
target_include_directories(${LIB_NAME} PUBLIC ${SPDLOG_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC spdlog)

# fmt
message(STATUS "Setup fmt")
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1
  SOURCE_DIR ${BUILD_LIB_DIR}/fmt
)
FetchContent_MakeAvailable(fmt)
set(FMT_INCLUDE ${BUILD_LIB_DIR}/fmt/include)
target_include_directories(${LIB_NAME} PUBLIC ${SPDLOG_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC fmt)

# json
message(STATUS "Setup json")
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  SOURCE_DIR ${BUILD_LIB_DIR}/json
)
FetchContent_MakeAvailable(json)
set(JSON_INCLUDE ${BUILD_LIB_DIR}/json/include)
target_include_directories(${LIB_NAME} PUBLIC ${JSON_INCLUDE})
target_link_libraries(${LIB_NAME} PUBLIC nlohmann_json::nlohmann_json)

# Catch2
message(STATUS "Setup catch2")
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
  SOURCE_DIR ${BUILD_LIB_DIR}/catch2
)
FetchContent_MakeAvailable(Catch2)
target_include_directories(${TEST_NAME} PUBLIC ${BUILD_LIB_DIR}/catch2/src/)
target_link_libraries(${TEST_NAME} PUBLIC Catch2::Catch2)

# download asset
set(ASSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets)

message(STATUS "Assets: gltf-samples")
FetchContent_Declare(
  gltf-samples
  GIT_REPOSITORY https://github.com/KhronosGroup/glTF-Sample-Models.git
  GIT_TAG d7a3cc8e51d7c573771ae77a57f16b0662a905c6
  SOURCE_DIR ${ASSET_DIR}/gltf-samples
)
FetchContent_MakeAvailable(gltf-samples)

# cubemaps
message(STATUS "Assets: gum trees")
file(DOWNLOAD https://dl.polyhaven.org/file/ph-assets/HDRIs/exr/4k/gum_trees_4k.exr ${ASSET_DIR}/cubemap/gum_trees_4k.exr)
