# download lib
set(BUILD_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

# ImGui
message(STATUS "Setup imgui")
add_library(imgui STATIC)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG 96839b445e32e46d87a44fd43a9cdd60c806f7e1  # the latest commit in the branch `docking`
  SOURCE_DIR ${BUILD_LIB_DIR}/imgui
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_INCLUDE ${BUILD_LIB_DIR}/imgui/include) # copy necessary files to ./include
file(MAKE_DIRECTORY ${IMGUI_INCLUDE})
set(_imgui_src_list imconfig.h imgui.cpp imgui.h imgui_draw.cpp backends/imgui_impl_glfw.h
    backends/imgui_impl_glfw.cpp backends/imgui_impl_vulkan.cpp backends/imgui_impl_vulkan.h imgui_internal.h
    imgui_tables.cpp imgui_widgets.cpp imstb_rectpack.h imstb_textedit.h imstb_truetype.h)
foreach (orig IN LISTS _imgui_src_list)
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
target_compile_options(tinygltf PRIVATE -Wno-missing-field-initializers)

# # std  # loaded in tinygltf
# message(STATUS "Setup stb")
# FetchContent_Declare(
#   stb
#   GIT_REPOSITORY https://github.com/nothings/stb.git
#   GIT_TAG f4a71b13373436a2866c5d68f8f80ac6f0bc1ffe
#   SOURCE_DIR ${BUILD_LIB_DIR}/stb
# )
# FetchContent_MakeAvailable(stb)
# set(STB_INCLUDE ${BUILD_LIB_DIR}/stb)
# target_include_directories(${LIB_NAME} PUBLIC ${STB_INCLUDE})

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

# download asset
set(ASSET_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets)
message(STATUS "Assets: sponza")
FetchContent_Declare(
  sponza
  URL https://themaister.net/sponza-gltf-pbr/sponza-gltf-pbr.zip
  SOURCE_DIR ${ASSET_DIR}/sponza
)
FetchContent_MakeAvailable(sponza)
