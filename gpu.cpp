#pragma leco app
#pragma leco add_shader "gpu.comp"

import print;
import stubby;
import vee;
import voo;

int main() try {
  auto img = stbi::load("image.png").take([](auto msg) { die(msg); });

  voo::device_and_queue dq { "gpu" };

  const unsigned map_sz = img.width * img.height;
  const unsigned buf_sz = map_sz * sizeof(float);
  voo::host_buffer b1 { dq, buf_sz };
  voo::host_buffer b2 { dq, buf_sz };

  auto dsl = vee::create_descriptor_set_layout({
    vee::dsl_compute_storage(),
    vee::dsl_compute_storage(),
  });
  auto pl = vee::create_pipeline_layout({ *dsl });

  auto dpool = vee::create_descriptor_pool(2, { vee::storage_buffer(4) });

  auto ds_f = vee::allocate_descriptor_set(*dpool, *dsl);
  vee::update_descriptor_set_with_storage(ds_f, 0, b1.buffer());
  vee::update_descriptor_set_with_storage(ds_f, 1, b2.buffer());

  auto ds_b = vee::allocate_descriptor_set(*dpool, *dsl);
  vee::update_descriptor_set_with_storage(ds_b, 0, b2.buffer());
  vee::update_descriptor_set_with_storage(ds_b, 1, b1.buffer());

  auto kern = vee::create_shader_module_from_resource("gpu.comp.spv");
  auto p = vee::create_compute_pipeline(*pl, *kern, "main");

  auto cp = vee::create_command_pool(dq.queue_family());
  auto cb = vee::allocate_primary_command_buffer(*cp);
  auto f = vee::create_fence_reset();

  {
    voo::mapmem mm { b1.memory() };
    auto p = static_cast<float *>(*mm);

    for (auto i = 0; i < map_sz; i++) {
      p[i] = 256.0 - (*img.data)[i * 4];
    }
  }

  {
    vee::begin_cmd_buf_one_time_submit(cb);
    //vee::cmd_bind_c_pipeline(cb, *p);
    //vee::cmd_bind_c_descriptor_set(cb, *pl, 0, ds_f);
    //vee::cmd_dispatch(cb, 1024, 1024, 1);
    vee::end_cmd_buf(cb);
  }
  dq.queue()->queue_submit({
    .fence = *f,
    .command_buffer = cb
  });
  vee::device_wait_idle();

  {
    auto pix = reinterpret_cast<stbi::pixel *>(*img.data);

    voo::mapmem mm { b1.memory() };
    auto p = static_cast<float *>(*mm);
    for (auto i = 0; i < map_sz; i++) {
      unsigned char cc = p[i] * 255;
      pix[i] = { cc, cc, cc, 255 };
    }

    stbi::write_rgba_unsafe("out/image.png", img.width, img.height, pix);
  }
} catch (...) {
  return 1;
}
