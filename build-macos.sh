# Turn off Mac OS verbose logging
export NSUnbufferedIO=NO

# Check if the build directory exists
if [ ! -d "build" ]; then
  echo "Creating build directory and copying .dylib files..."
  mkdir -p build

  # Copy .dylib files from vendor to build directory
  cp -P vendor/SDL3/macos/lib/*.dylib build/
  cp -P vendor/SDL3_ttf/macos/lib/*.dylib build/
  cp -P vendor/SDL3_mixer/macos/lib/*.dylib build/
fi

g++ -g -Wno-switch \
  -std=c++11 \
  -I vendor/SDL3/macos/include -L vendor/SDL3/macos/lib \
  -I vendor/SDL3_ttf/macos/include -L vendor/SDL3_ttf/macos/lib \
  -I vendor/GLAD/include \
  -I vendor/stb_image/include \
  -I vendor/glm/include \
  -o build/main \
  src/main.cpp \
  vendor/GLAD/src/glad.cpp \
  -lSDL3 -lSDL3_ttf -lSDL3_mixer \
  -ldl -framework OpenGL \
  -Wl,-headerpad_max_install_names  # Increase header padding for install_name_tool

# SDL3
install_name_tool -add_rpath @executable_path/../vendor/SDL3/macos/lib build/main

# SDL3_ttf
install_name_tool -add_rpath @executable_path/../vendor/SDL3_ttf/macos/lib build/main
# Correct library reference to match actual file
install_name_tool -change @rpath/libSDL3_ttf.0.dylib @rpath/libSDL3_ttf.0.0.0.dylib build/main

#SDL3_mixer
install_name_tool -add_rpath @executable_path/../vendor/SDL3_mixer/macos/lib build/main
# Correct library reference to match actual file
install_name_tool -change @rpath/libSDL3_mixer.0.dylib @rpath/libSDL3_mixer.0.0.0.dylib build/main