/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup imbuf
 */

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux.h>

#include "BLI_fileops.h"
#include "BLI_mmap.h"

#include "IMB_allocimbuf.hh"
#include "IMB_colormanagement.hh"
#include "IMB_filetype.hh"
#include "IMB_imbuf.hh"
#include "IMB_imbuf_types.hh"

#include "MEM_guardedalloc.h"

#include "CLG_log.h"

// We're going to use RLBox in a single-threaded environment.
// #define RLBOX_SINGLE_THREADED_INVOCATIONS
// The fixed configuration line we need to use for the noop sandbox.
// It specifies that all calls into the sandbox are resolved statically.
#define RLBOX_USE_STATIC_CALLS() rlbox_noop_sandbox_lookup_symbol

#include "rlbox.hpp"
#include "rlbox_noop_sandbox.hpp"

using namespace rlbox;

// Define base type for mylib using the noop sandbox
RLBOX_DEFINE_BASE_TYPES_FOR(libwebp, noop);

#include "libwebp_struct_file.h"
rlbox_load_structs_from_library(libwebp);

static rlbox_sandbox_libwebp sandbox;
static bool sandbox_initialized = false;

static inline void init_sandbox()
{
  if (!sandbox_initialized) {
    sandbox.create_sandbox();
    sandbox_initialized = true;
  }
}

static inline void destroy_sandbox()
{
  if (sandbox_initialized) {
    sandbox.destroy_sandbox();
    sandbox_initialized = false;
  }
}

static CLG_LogRef LOG = {"image.webp"};

bool imb_is_a_webp(const uchar *mem, size_t size)
{
  init_sandbox();

  auto tainted_mem = sandbox.malloc_in_sandbox<uchar>(size);
  memcpy(sandbox, tainted_mem, mem, size);

  bool ret = sandbox.invoke_sandbox_function(WebPGetInfo, tainted_mem, size, nullptr, nullptr)
                 .copy_and_verify([](int result) { return result != 0; });

  sandbox.free_in_sandbox(tainted_mem);

  destroy_sandbox();
  return ret;
}

ImBuf *imb_loadwebp(const uchar *mem, size_t size, int flags, ImFileColorSpace & /*r_colorspace*/)
{
  if (!imb_is_a_webp(mem, size)) {
    return nullptr;
  }

  init_sandbox();

  auto tainted_mem = sandbox.malloc_in_sandbox<uchar>(size);
  memcpy(sandbox, tainted_mem, mem, size);

  auto tainted_features = sandbox.malloc_in_sandbox<WebPBitstreamFeatures>();
  auto raw_features = tainted_features.unverified_safe_pointer_because(
      sizeof(WebPBitstreamFeatures), "just allocated");
  new (raw_features) WebPBitstreamFeatures();
  auto features_status = sandbox.invoke_sandbox_function(
      WebPGetFeatures, tainted_mem, size, tainted_features);
  if (features_status.copy_and_verify(
          [](VP8StatusCode result) { return result != VP8_STATUS_OK; }))
  {
    CLOG_ERROR(&LOG, "Failed to parse features");
    destroy_sandbox();
    return nullptr;
  }

  int width = tainted_features->width.copy_and_verify([](int w) { return w; });
  int height = tainted_features->height.copy_and_verify([](int h) { return h; });
  bool has_alpha = tainted_features->has_alpha.copy_and_verify([](int a) { return a != 0; });

  const int planes = has_alpha ? 32 : 24;
  ImBuf *ibuf = IMB_allocImBuf(width, height, planes, 0);

  if (ibuf == nullptr) {
    CLOG_ERROR(&LOG, "Failed to allocate image memory");
    return nullptr;
  }

  if ((flags & IB_test) == 0) {
    ibuf->ftype = IMB_FTYPE_WEBP;
    IMB_alloc_byte_pixels(ibuf);
    /* Flip the image during decoding to match Blender. */
    uchar *last_row = ibuf->byte_buffer.data + (4 * size_t(ibuf->y - 1) * size_t(ibuf->x));

    auto tainted_last_row = sandbox.malloc_in_sandbox<uchar>(4 * size_t(ibuf->x) *
                                                             size_t(ibuf->y));

    auto decode_status = sandbox.invoke_sandbox_function(WebPDecodeRGBAInto,
                                                         tainted_mem,
                                                         size,
                                                         tainted_last_row,
                                                         size_t(ibuf->x) * ibuf->y * 4,
                                                         -4 * ibuf->x);

    if (decode_status == nullptr) {
      CLOG_ERROR(&LOG, "Failed to decode image");
    }

    std::unique_ptr<uchar[]> copied_last_row = tainted_last_row.copy_and_verify_range(
        [&](std::unique_ptr<uchar[]> val) { return val; }, size);
    memcpy(last_row, copied_last_row.get(), 4 * size_t(ibuf->x) * size_t(ibuf->y));
  }

  sandbox.free_in_sandbox(tainted_features);
  sandbox.free_in_sandbox(tainted_mem);

  destroy_sandbox();

  return ibuf;
}

ImBuf *imb_load_filepath_thumbnail_webp(const char *filepath,
                                        const int /*flags*/,
                                        const size_t max_thumb_size,
                                        ImFileColorSpace & /*r_colorspace*/,
                                        size_t *r_width,
                                        size_t *r_height)
{
  const int file = BLI_open(filepath, O_BINARY | O_RDONLY, 0);
  if (file == -1) {
    return nullptr;
  }

  imb_mmap_lock();
  auto tainted_mmap_file = sandbox.invoke_sandbox_function(BLI_mmap_open, file);
  imb_mmap_unlock();
  close(file);
  if (tainted_mmap_file == nullptr) {
    return nullptr;
  }

  auto tainted_data = sandbox_reinterpret_cast<const char *>(
      sandbox.invoke_sandbox_function(BLI_mmap_get_pointer, tainted_mmap_file));
  // auto data_size = sandbox.invoke_sandbox_function(BLI_mmap_get_length, tainted_mmap_file)
  //                      .copy_and_verify([](size_t size) { return size; });

  auto tainted_config = sandbox.malloc_in_sandbox<WebPDecoderConfig>();
  auto raw_config = tainted_config.unverified_safe_pointer_because(sizeof(WebPDecoderConfig),
                                                                   "just allocated");
  if (tainted_data == nullptr ||
      !sandbox.invoke_sandbox_function(WebPInitDecoderConfig, tainted_config)
           .copy_and_verify([](int s) { return s; }) /* ||
           TODO: put back, remove temp
       sandbox.invoke_sandbox_function(WebPGetFeatures, tainted_data, data_size, tainted_config->input)
          .copy_and_verify([](int s) { return s != VP8_STATUS_OK; }) */ )
  {
    CLOG_ERROR(&LOG, "Invalid file");
    imb_mmap_lock();
    sandbox.invoke_sandbox_function(BLI_mmap_free, tainted_mmap_file);
    imb_mmap_unlock();
    return nullptr;
  }

  auto width = tainted_config->input.width.copy_and_verify([](int w) { return w; });
  auto height = tainted_config->input.height.copy_and_verify([](int h) { return h; });

  /* Return full size of the image. */
  *r_width = size_t(width);
  *r_height = size_t(height);

  const float scale = float(max_thumb_size) / std::max(width, height);
  const int dest_w = std::max(int(width * scale), 1);
  const int dest_h = std::max(int(height * scale), 1);

  ImBuf *ibuf = IMB_allocImBuf(dest_w, dest_h, 32, IB_byte_data);
  if (ibuf == nullptr) {
    CLOG_ERROR(&LOG, "Failed to allocate image memory");
    imb_mmap_lock();
    sandbox.invoke_sandbox_function(BLI_mmap_free, tainted_mmap_file);
    imb_mmap_unlock();
    return nullptr;
  }

  auto &config = *raw_config;
  config.options.no_fancy_upsampling = 1;
  config.options.use_scaling = 1;
  config.options.scaled_width = dest_w;
  config.options.scaled_height = dest_h;
  config.options.bypass_filtering = 1;
  config.options.use_threads = 0;
  config.options.flip = 1;
  config.output.is_external_memory = 1;
  config.output.colorspace = MODE_RGBA;
  config.output.u.RGBA.rgba = ibuf->byte_buffer.data;
  config.output.u.RGBA.stride = 4 * ibuf->x;
  config.output.u.RGBA.size = size_t(config.output.u.RGBA.stride) * size_t(ibuf->y);

  // TODO
  if (true /* sandbox.invoke_sandbox_function(WebPDecode, tainted_data, data_size, tainted_config)
          .copy_and_verify([](int s) { return s != VP8_STATUS_OK; }) */)
  {
    CLOG_ERROR(&LOG, "Failed to decode image");
    IMB_freeImBuf(ibuf);

    imb_mmap_lock();
    sandbox.invoke_sandbox_function(BLI_mmap_free, tainted_mmap_file);
    imb_mmap_unlock();
    return nullptr;
  }

  /* Free the output buffer. */
  // TODO
  // sandbox.invoke_sandbox_function(WebPFreeDecBuffer, tainted_config->output);

  imb_mmap_lock();
  sandbox.invoke_sandbox_function(BLI_mmap_free, tainted_mmap_file);
  imb_mmap_unlock();

  return ibuf;
}

bool imb_savewebp(ImBuf *ibuf, const char *filepath, int /*flags*/)
{
  const uint limit = 16383;
  if (ibuf->x > limit || ibuf->y > limit) {
    CLOG_ERROR(&LOG, "image x/y exceeds %u", limit);
    return false;
  }

  const int bytesperpixel = (ibuf->planes + 7) >> 3;
  uchar *last_row;
  size_t encoded_data_size;

  auto tainted_encoded_data = sandbox.malloc_in_sandbox<uchar *>();
  if (bytesperpixel == 3) {
    /* We must convert the ImBuf RGBA buffer to RGB as WebP expects a RGB buffer. */
    const size_t num_pixels = IMB_get_pixel_count(ibuf);
    const uint8_t *rgba_rect = ibuf->byte_buffer.data;
    uint8_t *rgb_rect = MEM_malloc_arrayN<uint8_t>(num_pixels * 3, "webp rgb_rect");
    for (size_t i = 0; i < num_pixels; i++) {
      rgb_rect[i * 3 + 0] = rgba_rect[i * 4 + 0];
      rgb_rect[i * 3 + 1] = rgba_rect[i * 4 + 1];
      rgb_rect[i * 3 + 2] = rgba_rect[i * 4 + 2];
    }

    last_row = (uchar *)(rgb_rect + (size_t(ibuf->y - 1) * size_t(ibuf->x) * 3));

    auto row = sandbox.malloc_in_sandbox<uint8_t>(num_pixels * 3);
    memcpy(sandbox, row, last_row, num_pixels * 3);

    if (ibuf->foptions.quality == 100.0f) {
      encoded_data_size =
          sandbox
              .invoke_sandbox_function(
                  WebPEncodeLosslessRGB, row, ibuf->x, ibuf->y, -3 * ibuf->x, tainted_encoded_data)
              .copy_and_verify([](size_t size) { return size; });
    }
    else {
      encoded_data_size = sandbox
                              .invoke_sandbox_function(WebPEncodeRGB,
                                                       row,
                                                       ibuf->x,
                                                       ibuf->y,
                                                       -3 * ibuf->x,
                                                       ibuf->foptions.quality,
                                                       tainted_encoded_data)
                              .copy_and_verify([](size_t size) { return size; });
    }
    MEM_freeN(rgb_rect);
  }
  else if (bytesperpixel == 4) {
    const size_t num_pixels = IMB_get_pixel_count(ibuf);
    last_row = ibuf->byte_buffer.data + 4 * size_t(ibuf->y - 1) * size_t(ibuf->x);
    auto row = sandbox.malloc_in_sandbox<uint8_t>(num_pixels * 4);
    memcpy(sandbox, row, last_row, num_pixels * 4);

    if (ibuf->foptions.quality == 100.0f) {
      encoded_data_size = sandbox
                              .invoke_sandbox_function(WebPEncodeLosslessRGBA,
                                                       row,
                                                       ibuf->x,
                                                       ibuf->y,
                                                       -4 * ibuf->x,
                                                       tainted_encoded_data)
                              .copy_and_verify([](size_t size) { return size; });
    }
    else {
      encoded_data_size = sandbox
                              .invoke_sandbox_function(WebPEncodeRGBA,
                                                       row,
                                                       ibuf->x,
                                                       ibuf->y,
                                                       -4 * ibuf->x,
                                                       ibuf->foptions.quality,
                                                       tainted_encoded_data)
                              .copy_and_verify([](size_t size) { return size; });
    }
  }
  else {
    CLOG_ERROR(&LOG, "Unsupported bytes per pixel: %d for file: '%s'", bytesperpixel, filepath);
    sandbox.free_in_sandbox(tainted_encoded_data);
    return false;
  }

  // TODO
  // if (*tainted_encoded_data == nullptr) {
  //   sandbox.free_in_sandbox(tainted_encoded_data);
  //   return false;
  // }

  auto tainted_mux = sandbox.invoke_sandbox_function(WebPMuxNew);
  auto image_data = sandbox.malloc_in_sandbox<WebPData>();
  // TODO
  // image_data->bytes = tainted_encoded_data;
  image_data->size = encoded_data_size;
  sandbox.invoke_sandbox_function(
      WebPMuxSetImage, tainted_mux, image_data, false /* Don't copy data */);

  /* Write ICC profile if there is one associated with the colorspace. */
  const ColorSpace *colorspace = ibuf->byte_buffer.colorspace;
  if (colorspace) {
    blender::Vector<char> icc_profile = IMB_colormanagement_space_icc_profile(colorspace);
    if (!icc_profile.is_empty()) {
      // TODO
      // WebPData icc_chunk = {reinterpret_cast<const uint8_t *>(icc_profile.data()),
      //                       size_t(icc_profile.size())};
      auto icc_chunk = sandbox.malloc_in_sandbox<WebPData>();
      auto iccp_string = sandbox.malloc_in_sandbox<char>(5);
      strncpy(sandbox, iccp_string, "ICCP", 5);
      sandbox.invoke_sandbox_function(
          WebPMuxSetChunk, tainted_mux, iccp_string, icc_chunk, true /* copy data */);
    }
  }

  /* Assemble image and metadata. */
  auto tainted_output_data = sandbox.malloc_in_sandbox<WebPData>();
  if (sandbox.invoke_sandbox_function(WebPMuxAssemble, tainted_mux, tainted_output_data)
          .copy_and_verify([](WebPMuxError result) { return result != WEBP_MUX_OK; }))
  {
    CLOG_ERROR(&LOG, "Error in mux assemble writing file: '%s'", filepath);
    sandbox.invoke_sandbox_function(WebPMuxDelete, tainted_mux);
    sandbox.invoke_sandbox_function(WebPFree, tainted_encoded_data);
    sandbox.free_in_sandbox(tainted_encoded_data);
    sandbox.free_in_sandbox(tainted_output_data);
    return false;
  }

  /* Write to file. */
  bool ok = true;
  FILE *fp = BLI_fopen(filepath, "wb");
  if (fp) {
    // TODO
    // if (fwrite(tainted_output_data->bytes, tainted_output_data->size, 1, fp) != 1) {
    //   CLOG_ERROR(&LOG, "Unknown error writing file: '%s'", filepath);
    //   ok = false;
    // }

    fclose(fp);
  }
  else {
    ok = false;
    CLOG_ERROR(&LOG, "Cannot open file for writing: '%s'", filepath);
  }

  sandbox.invoke_sandbox_function(WebPMuxDelete, tainted_mux);
  sandbox.invoke_sandbox_function(WebPFree, *tainted_encoded_data);
  sandbox.invoke_sandbox_function(WebPDataClear, tainted_output_data);
  sandbox.free_in_sandbox(tainted_encoded_data);
  sandbox.free_in_sandbox(tainted_output_data);

  return ok;
}
