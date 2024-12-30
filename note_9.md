# 一 checkpoint代码
## 1.1 ./linux/common-build.nix
### 1.1.1 ARCH
```bash
export ARCH=riscv
```
设置内核构建的目标架构为 RISC-V，用于告诉内核构建系统应该为哪个架构生成内核代码。
### 1.1.2 RISCV_ROOTFS_HOME
RISCV_ROOTFS_HOME 是设置 RISC-V 根文件系统的路径，通常在交叉编译时需要指定该路径，尤其是在内核构建时，它会用于指定根文件系统的位置。
### 1.1.3 CROSS_COMPILE
```bash
export CROSS_COMPILE=riscv64-unknown-linux-gnu-
```
CROSS_COMPILE 设置了交叉编译工具链的前缀。此处指定的前缀 riscv64-unknown-linux-gnu- 是用于 RISC-V 64 位架构的交叉编译工具链的标准前缀。

### 1.1.4 makeFlags
makeFlags 主要的作用是确保在 make 命令执行时，传递给 make 的环境变量或参数优先是在 makeFlags 中定义的，其次才是依赖于 buildPhase 中的 Bash 环境设置。


## 1.2 dts
是描述硬件设备树的源代码文件，用于在 Linux 内核或其他嵌入式系统中描述硬件配置。DTS 文件最终会被编译成 DTB（Device Tree Blob）文件，在系统启动时被加载，帮助引导程序和内核初始化硬件。
```nix
  buildInputs = [
    dtc
  ];
  buildPhase = ''
    cd dts
    dtc -O dtb -o ${name} system.dts
  '';
```
- buildInputs：导入dtc工具，由于编译。
- buildPhase： 把香山dtc下的system.dts编译成dtb。

# 二构建test练习

## 2.1交叉编译
### 2.1.1 获取代码
```nix
  src = fetchzip{
    url = "https://raw.githubusercontent.com/tangjing2021/note/refs/heads/main/hello_checkpoint.zip";
    sha256 = "2fdfc873d8d2588eb92d2e75be124f54f3515502f4e51218d528733f6dd7dbe9";
  };
```
从github上下载对应的压缩包，并且进行解压。其中的main.cpp就是需要处理的代码。
### 2.2.2 编译工具
```nix
  riscv64-cc = pkgs.pkgsCross.riscv64.stdenv.cc;
  riscv64-libc-static = pkgs.pkgsCross.riscv64.stdenv.cc.libc.static;
  binutils = pkgs.pkgsCross.riscv64.binutils;
  riscv64-jemalloc = pkgs.pkgsCross.riscv64.jemalloc;
```
- stdenv.cc 是标准环境中的 C 编译器，它是交叉编译工具链的一部分，通常用于构建 C 语言源代码。
-  stdenv.cc.libc.static 是交叉编译工具链中静态 C 库的路径，通常这是一个包含标准库（如 libc.a）的静态库，用于静态链接。
-  binutils: 是一组工具，包含汇编器（as）、链接器（ld）、目标文件操作工具（如 objcopy、nm 等），它们在编译过程中是必需的。
-  riscv64-jemalloc: 指向jemalloc库的一个版本。jemalloc 是一个高效的内存分配器，常用于需要高效内存管理的应用中。

```nix
customJemalloc = riscv64-jemalloc.overrideAttrs (oldAttrs: {
    configureFlags = (oldAttrs.configureFlags or []) ++ [
      "--enable-static"
      "--disable-shared"
    ];
    postInstall = ''
      ${oldAttrs.postInstall or ""}
      cp -v lib/libjemalloc.a $out/lib/
    '';
  });
```
- 在原jemalloc上进行自定义配置。
- --enable-static: 启用静态库的构建。
- --disable-shared: 禁用共享库的构建。
-  postInstall: 将库保存到对应位置。 
### 2.3.3 编译
```nix
  crossConfig = {
    host = "x86_64-linux";  # 宿主机架构
    target = "riscv64-linux-gnu";  # 目标架构
    crossGcc = riscv64-cc;  # 指定交叉编译器
    crossBinutils = binutils;  #交叉编译所需要的工具
  };
	...
  buildInputs = [
    riscv64-cc
    riscv64-libc-static
    customJemalloc
  ];
  ...
   buildPhase = ''
    mkdir -p $out/bin
    export CC=${riscv64-cc}/bin/riscv64-unknown-linux-gnu-g++
    # 使用交叉编译器编译源代码，静态链接
    $CC main.cpp -o $out/bin/Main -static -ljemalloc -L${customJemalloc}/lib
  '';
```
- crossConfig: 描述交叉编译的配置。
- buildInputs: 为编译过程导入相关工具。
- buildPhase: 执行编译。（这里是静态链接，为了后续产生基本块向量）。


