set(LIB_SRC
    # ./
    vlux/app.cpp
    vlux/camera.cpp
    vlux/control.cpp
    vlux/frame_timer.cpp
    vlux/gui.cpp
    vlux/light.cpp
    vlux/transform.cpp
    vlux/uniform_buffer.cpp
    # ./common
    vlux/common/buffer.cpp
    vlux/common/command_buffer.cpp
    vlux/common/command_pool.cpp
    vlux/common/compute_pipeline.cpp
    vlux/common/descriptor_pool.cpp
    vlux/common/descriptor_set_layout.cpp
    vlux/common/descriptor_sets.cpp
    vlux/common/frame_buffer.cpp
    vlux/common/graphics_pipeline.cpp
    vlux/common/image.cpp
    vlux/common/pipeline_layout.cpp
    vlux/common/queue.cpp
    vlux/common/render_pass.cpp
    vlux/common/utils.cpp
    # ./cubemap
    vlux/cubemap/cubemap.cpp
    # ./device_resource
    vlux/device_resource/debug_messenger.cpp
    vlux/device_resource/device.cpp
    vlux/device_resource/device_resource.cpp
    vlux/device_resource/instance.cpp
    vlux/device_resource/swapchain.cpp
    vlux/device_resource/sync_object.cpp
    vlux/device_resource/surface.cpp
    vlux/device_resource/window.cpp
    # ./draw
    vlux/draw/draw_strategy.cpp
    vlux/draw/rasterize/rasterize.cpp
    vlux/draw/raytracing/raytracing.cpp
    # ./model
    vlux/model/index.cpp
    vlux/model/gltf.cpp
    vlux/model/model.cpp
    vlux/model/vertex.cpp
    # ./postprocess
    vlux/postprocess/tonemapping.cpp
    # ./scene
    vlux/scene/scene.cpp
    vlux/shader/shader.cpp
    # ./texture
    vlux/texture/texture.cpp
    vlux/texture/texture_sampler.cpp
    # ./utils
    vlux/utils/debug.h
    vlux/utils/io.cpp
    vlux/utils/path.h
    vlux/utils/string.h
    vlux/utils/timer.h
)

target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_sources(${LIB_NAME} PUBLIC ${LIB_SRC})
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/vlux)
target_precompile_headers(${LIB_NAME}
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vlux/pch.h)
target_compile_definitions(${LIB_NAME} PUBLIC "UNICODE" "_UNICODE")