#pragma leco tool

import dotz;
import hai;
import print;
import stubby;

constexpr auto min(auto f) { return f; }
constexpr auto min(auto f, auto ... o) {
  return dotz::min(f, min(o...));
}

int main() {
  auto img = stbi::load("image.png").take([](auto msg) { die(msg); });

  hai::array<float> sdf { static_cast<unsigned>(img.width * img.height) };
  hai::array<float> sdf2 { static_cast<unsigned>(img.width * img.height) };
  for (auto i = 0; i < sdf.size(); i++) {
    sdf[i] = sdf2[i] = 256.0 - (*img.data)[i * 4];
  } 


  auto p1 = sdf.begin();
  auto p2 = sdf2.begin();
  for (auto i = 0; i < 256; i++) {
    const auto p = [&](auto x, auto y) { return p1[y * img.width + x]; };
    for (auto y = 1; y < img.height - 2; y++) {
      for (auto x = 1; x < img.width - 2; x++) {
        p2[y * img.width + x] =
          min(p(x, y), 
              p(x + 1, y) + 1, p(x - 1, y) + 1,
              p(x, y + 1) + 1, p(x, y - 1) + 1,
              p(x + 1, y - 1) + 1.4, p(x - 1, y - 1) + 1.4,
              p(x + 1, y + 1) + 1.4, p(x - 1, y + 1) + 1.4);
      }
    }
    auto pp = p2;
    p2 = p1;
    p1 = pp;
  }

  auto pix = reinterpret_cast<stbi::pixel *>(*img.data);
  for (auto i = 0; i < sdf.size(); i++) {
    auto c = p1[i];
    unsigned char cc = 0;
    if (c < 5) cc = 255;
    if (c < 3) cc = 0;
    if (c == 0) cc = 255;
    pix[i] = { cc, cc, cc, 255 };
  }
  stbi::write_rgba_unsafe("out/image.png", img.width, img.height, pix);
}
