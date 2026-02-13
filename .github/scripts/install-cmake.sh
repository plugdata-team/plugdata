case $(uname -m) in
    x86_64)   curl -L https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-x86_64.sh --output ./do-install-cmake.sh ;;
    arm)      curl -L https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-aarch64.sh --output ./do-install-cmake.sh ;;
    arm64)    curl -L https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-aarch64.sh --output ./do-install-cmake.sh ;;
    aarch64)  curl -L https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-aarch64.sh --output ./do-install-cmake.sh ;;
esac

chmod +x do-install-cmake.sh
./do-install-cmake.sh --skip-license --prefix=/usr
