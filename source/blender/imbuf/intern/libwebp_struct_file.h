#define sandbox_fields_reflection_libwebp_class_WebPBitstreamFeatures(f, g, ...) \
  f(int, width, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, height, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, has_alpha, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, has_animation, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, format, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32_t[5], pad, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_libwebp_class_WebPDecoderConfig(f, g, ...) \
  f(WebPBitstreamFeatures, input, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(WebPDecBuffer, output, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(WebPDecoderOptions, options, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_libwebp_union_BufferU(f, g, ...) \
  f(WebPRGBABuffer, RGBA, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(WebPYUVABuffer, YUVA, FIELD_NORMAL, ##__VA_ARGS__) g() \

#define sandbox_fields_reflection_libwebp_class_WebPRGBABuffer(f, g, ...) \
  f(uint8_t*, rgba, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, stride, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, size, FIELD_NORMAL, ##__VA_ARGS__) g() \


#define sandbox_fields_reflection_libwebp_class_WebPYUVABuffer(f, g, ...) \
  f(uint8_t*, y, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8_t*, u, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8_t*, v, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8_t*, a, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, y_stride, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, u_stride, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, v_stride, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, a_stride, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, y_size, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, u_size, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, v_size, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, a_size, FIELD_NORMAL, ##__VA_ARGS__) g()

union DecBufferUnion {
  WebPRGBABuffer RGBA;
  WebPYUVABuffer YUVA;
};

#define sandbox_fields_reflection_libwebp_class_WebPDecBuffer(f, g, ...) \
  f(WEBP_CSP_MODE, colorspace, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, width, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, height, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, is_external_memory, FIELD_NORMAL, ##__VA_ARGS__) g() \
  /* union is 80 bytes, aligned to 8 bytes */ \
  f(uint64_t[10], u, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32_t[4], pad, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8_t*, private_memory, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_libwebp_class_WebPDecoderOptions(f, g, ...) \
  f(int, bypass_filtering, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, no_fancy_upsampling, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, use_cropping, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, crop_left, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, crop_top, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, crop_width, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, crop_height, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, use_scaling, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, scaled_width, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, scaled_height, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, use_threads, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, dithering_strength, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, flip, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int, alpha_dithering_strength, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32_t[5], pad, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_libwebp_class_WebPData(f, g, ...) \
  f(const uint8_t*, bytes, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t, size, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_libwebp_allClasses(f, ...) \
  f(WebPBitstreamFeatures, libwebp, ##__VA_ARGS__) \
  f(WebPDecoderConfig, libwebp, ##__VA_ARGS__) \
  f(WebPDecBuffer, libwebp, ##__VA_ARGS__) \
  f(WebPDecoderOptions, libwebp, ##__VA_ARGS__) \
  f(WebPRGBABuffer, libwebp, ##__VA_ARGS__) \
  f(WebPYUVABuffer, libwebp, ##__VA_ARGS__) \
  f(WebPData, libwebp, ##__VA_ARGS__)
