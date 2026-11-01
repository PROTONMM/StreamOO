set(sources "streamoo_flac_header.c" "streamoo_flac_parse.c" "streamoo_decoder.c" "streamoo_decode_stream.c")
if(CONFIG_STREAMOO_FLAC_MICRO)
    list(APPEND sources "streamoo_flac_micro.cpp")
    foreach(name crc frame_header pcm_packing flac_decoder decorrelation lpc)
        list(APPEND sources "third_party/micro-flac/src/${name}.cpp")
    endforeach()
    list(APPEND sources "third_party/micro-flac/src/xtensa/lpc_32_xtensa.S"
                        "third_party/micro-flac/src/xtensa/lpc_64_xtensa.S")
elseif(CONFIG_STREAMOO_FLAC_DR)
    list(APPEND sources "streamoo_flac_dr.c")
endif()
idf_component_register(SRCS ${sources} INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "." "third_party/micro-flac/include" "third_party/micro-flac/src"
                       REQUIRES esp_audio_codec pcm_adapter compressed_source PRIV_REQUIRES log heap)
if(CONFIG_STREAMOO_FLAC_MICRO)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE MICRO_FLAC_DISABLE_OGG MICRO_FLAC_MEMORY_PSRAM_ONLY)
    target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_14)
endif()
