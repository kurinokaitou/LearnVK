function(compile_shader TARGET_NAME SHADERS SHADER_INCLUDE_DIR)

    set(working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GLSLANG_BIN $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe) # ����glslangValidator��λ��

    foreach(SHADER ${SHADERS})  # ����ÿһ��shaderԴ�ļ�
    get_filename_component(SHADER_NAME ${SHADER} NAME)  # ��ȡshader������
    string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})  # �����ɵ�.h�ļ��н��ļ�����'.'����'_'
    string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)    # ���洢���������ݵ�ȫ��vector��������Ϊȫ����д
    set(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/spv/${SHADER_NAME}.spv")    # ���ɵ�.spv�ļ�
    set(CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/cpp/${HEADER_NAME}.h")      # ���ɵ�.h�ļ�

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLANG_BIN} -I${SHADER_INCLUDE_DIR} -V100 -o ${SPV_FILE} ${SHADER}
        DEPENDS ${SHADER}
        WORKING_DIRECTORY "${working_dir}")             # ��ӱ����������Ŀ����ʱִ��

    list(APPEND ALL_GENERATED_SPV_FILES ${SPV_FILE})    

    add_custom_command(
            OUTPUT ${CPP_FILE}
            COMMAND ${CMAKE_COMMAND} -DPATH=${SPV_FILE} -DHEADER="${CPP_FILE}" 
                -DGLOBAL="${GLOBAL_SHADER_VAR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateShaderCPPFile.cmake"
            DEPENDS ${SPV_FILE}
            WORKING_DIRECTORY "${working_dir}")         # ���ִ�н�spvת��Ϊh�ļ���cmake����������

    list(APPEND ALL_GENERATED_CPP_FILES ${CPP_FILE})

    endforeach()
    add_custom_target(${TARGET_NAME}    # ������������ӵ�һ������Ŀ����
        DEPENDS ${ALL_GENERATED_SPV_FILES} ${ALL_GENERATED_CPP_FILES} SOURCES ${SHADERS})
endfunction()