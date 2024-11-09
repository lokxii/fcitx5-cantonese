# fcitx-cantonese

Integrating [Cantonese-IME](https://github.com/lokxii/Cantonese-IME/) into
fcitx5 addon.

Copy `data/` from Cantonese-IME to `$HOME/.local/share/fcitx5/cantonese/data/`

## Building

```sh
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
sudo make install
```
