#-------------------------------------------------------------------------------
# 收集源文件并添加到目标
#-------------------------------------------------------------------------------
function(add_target_compile_files target)
    # 添加项目源码目录到目标的私有包含目录
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # 收集源文件（使用 CONFIGURE_DEPENDS 自动更新）
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/infer/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/infer/*.cu"
    )

    # 将收集到的源文件添加到目标
    target_sources(${target} PRIVATE ${SOURCES})
endfunction()

#-------------------------------------------------------------------------------
# 配置目标的通用属性
#-------------------------------------------------------------------------------
function(configure_target_common_properties target)
    # 设置目标的通用属性
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${target}"
        CXX_VISIBILITY_PRESET "hidden"
    )
    # 配置 CUDA 和 TensorRT 相关属性
    configure_cuda_trt_target(${target})
    # 设置目标的编译选项
    set_target_compile_options(${target})
endfunction()

#-------------------------------------------------------------------------------
# trtyolo 库
#-------------------------------------------------------------------------------
add_library(trtyolo SHARED)
add_target_compile_files(trtyolo)
configure_target_common_properties(trtyolo)

# 安装头文件到 include/deploy 目录
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/infer/trtyolo.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# 安装库文件
install(TARGETS trtyolo
    EXPORT trtyoloTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  # 安装到 lib 目录
    # ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # 安装到 lib 目录
    # RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}  # 安装到 lib 目录
)

# 子库编译完成后立即安装到指定目录
add_custom_command(TARGET trtyolo POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR} --prefix ${CMAKE_INSTALL_PREFIX} --config ${CMAKE_BUILD_TYPE}
    COMMENT "Installing trtyolo library and headers to ${CMAKE_INSTALL_PREFIX}"
)
