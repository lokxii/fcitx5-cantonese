# fcitx-cantonese

Integrating [Cantonese-IME](https://github.com/lokxii/Cantonese-IME/) into
fcitx5 addon.

Copy `data/` from Cantonese-IME to `$HOME/.local/share/fcitx5/cantonese/data/`

## Building

```sh
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

Add `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to generate `compile_commands.json`
for development.
