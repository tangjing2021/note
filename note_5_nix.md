# 语言结构
## 断言
通常用于检查功能和依赖项上的某些要求是否成立。
```nix
assert e1; e2
```
其中e1是一个表达式，其计算结果应为布尔值。如果计算结果为true，则返回e2；否则表达式计算将中止并打印回溯。

## with表达式​
```nix
with e1; e2
```
将集合e1引入表达式e2的词法作用域。如果当前作用域中已经存在一个绑定，与 with 引入的绑定同名，那么 with 不会掩盖（覆盖）已有的绑定。相反，当前作用域的绑定会优先使用。

## 路径和字符串连接
路径+字符串 ，将路径与字符串连接起来。结果是一条路径。`该字符串不能具有引用存储路径的字符串上下文`,如：
```nix
let
  storePath = "/nix/store/abc123-some-package";
  additionalPath = "bin";
in
  storePath + "/" + additionalPath  # 错误！
```
 storePath 指向了一个 Nix 存储路径，而 Nix 不允许将存储路径与普通字符串拼接，以避免产生不可预期的行为或错误。如果需要动态构造路径，可以使用 toString 或确保字符串不指向存储路径。
```nix
let
  basePath = /nix/store/abc123-some-package;
  subPath = "bin";
in
  toString basePath + "/" + subPath
```

# Derivation 
构建过程的核心概念，它定义了如何构建软件包或其他构建目标，Derivation 是 Nix 中对构建过程的抽象，它定义了构建所需的依赖、构建步骤、构建输出以及如何将这些输出整合成最终结果。
## 主要包含
    name 和 version：指定软件包的名称和版本。
    src：源代码的位置，可以是本地路径、远程下载链接或其他来源。
    buildInputs：构建过程所需的依赖包，通常是编译器、库、工具等。这些依赖会在构建时注入到构建环境中。
    builder：用于实际构建的脚本或程序。
    meta：用于描述该包的一些元信息，比如许可证、维护者、描述等。
    outputs：构建结果，包括最终的可执行文件、库或其他文件，这些通常会被存放在 /nix/store 中。
## 实践：
```nix
#file.nix 将一个.cpp编译成可执行文件
let
  nixpkgs = import <nixpkgs> {};  # 引入 nixpkgs
  licenses = nixpkgs.lib.licenses;  # 引入 licenses 模块
in

nixpkgs.stdenv.mkDerivation rec {
  pname = "hello";
  version = "1.0";
  name = "${pname}-${version}";
  src = /home/tj/CLionProjects/hello;  # 直接使用本地路径
  outputs = [ "out" ];#在构建完成后，Nix 会在 /nix/store/ 中为该构建生成一个目录，并将 out 路径指向那个实际的目录。
  buildInputs = [
    nixpkgs.gcc
    nixpkgs.coreutils  # 提供常用工具
    nixpkgs.bash  # 提供 bash 支持
  ];
  #builder = /home/tj/build.sh;  # 使用自定义构建脚本
  meta = with nixpkgs; {
    description = "A simple Hello World program written in C";
    license = licenses.gpl3;
  };
  # 定义构建步骤
  buildPhase = ''   #在nix中构建脚本
    echo "Starting build process..."
    mkdir -p $out
    g++ -o $out/hello main.cpp  # 编译 c++程序,c用gcc
  '';
  # 安装步骤
  installPhase = ''
    mkdir -p $out/bin
    mv $out/hello $out/bin/  # 将编译后的文件移动到 $out/bin 目录
  '';
}
```

```bush
#build.sh
#!/bin/bash

# 进入 Nix 构建环境
source $stdenv/setup

echo "Starting build process in Nix environment..."

# 进入源代码目录
echo "Current working directory: $(pwd)"
echo "Source directory: $src"
cd $src
echo "Current working directory: $(pwd)"

mkdir -p $out

# 使用 gcc 编译程序
g++ -o $out/hello main.cpp
if [ $? -eq 0 ]; then
  echo "Compilation successful."
else
  echo "Compilation failed."
  exit 1
fi

```



# 关于nix-shell
```bash
#短暂设置环境变量 NIX_PATH，并将其中的 nixpkgs 绑定到指定的本地路径，通过这种方式，Nix 会在指定的路径中查找 nixpkgs，而不是默认的在线源或其他路径
export NIX_PATH=nixpkgs=/home/.../nixpkgs   

#查看nix环境变量
echo $NIX_PATH

#在指定的路径中查找 nixpkgs
nix-shell -I nixpkgs=/home/tj/mygit/nixpkgs ... .nix

#命令用于将一个新的 Nix 渠道添加到你的系统配置中。渠道是一个指定的源，Nix 会从该源下载包和更新。如果源发生变化，Nix 会重新从新的源下载并计算依赖
nix-channel --add https://nixos.org/channels/nixos-23.05 nixpkgs

#更新频道，以便同步
nix-channel --update

#查看
nix-channel --list
```
