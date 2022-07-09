function(compile_shader TARGET_NAME SHADERS SHADER_INCLUDE_DIR)

    set(working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GLSLANG_BIN $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe) # 设置glslangValidator的位置

    foreach(SHADER ${SHADERS})  # 遍历每一个shader源文件
    get_filename_component(SHADER_NAME ${SHADER} NAME)  # 获取shader的名字
    string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})  # 在生成的.h文件中将文件名的'.'换成'_'
    string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)    # 将存储二进制内容的全局vector对象名改为全部大写
    set(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/spv/${SHADER_NAME}.spv")    # 生成的.spv文件
    set(CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/cpp/${HEADER_NAME}.h")      # 生成的.h文件

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLANG_BIN} -I${SHADER_INCLUDE_DIR} -V100 -o ${SPV_FILE} ${SHADER}
        DEPENDS ${SHADER}
        WORKING_DIRECTORY "${working_dir}")             # 添加编译命令，在项目生成时执行

    list(APPEND ALL_GENERATED_SPV_FILES ${SPV_FILE})    

    add_custom_command(
            OUTPUT ${CPP_FILE}
            COMMAND ${CMAKE_COMMAND} -DPATH=${SPV_FILE} -DHEADER="${CPP_FILE}" 
                -DGLOBAL="${GLOBAL_SHADER_VAR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateShaderCPPFile.cmake"
            DEPENDS ${SPV_FILE}
            WORKING_DIRECTORY "${working_dir}")         # 添加执行将spv转换为h文件的cmake函数的命令

    list(APPEND ALL_GENERATED_CPP_FILES ${CPP_FILE})

    endforeach()
    add_custom_target(${TARGET_NAME}    # 将上述过程添加到一个生成目标中
        DEPENDS ${ALL_GENERATED_SPV_FILES} ${ALL_GENERATED_CPP_FILES} SOURCES ${SHADERS})
endfunction()