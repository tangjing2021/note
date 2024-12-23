# Main_Commond
## 共同操作
```bash
commond -j number #设置最大并行构建任务数。
commond --cores 8 #指定并行度
commond --arg name value  #为 Nix 表达式中的函数参数指定值
commond --argstr name value #为字符串参数指定值
commond [path] -A name  #构建指定路径下default.nix里面的属性name
commond -I path #为命令添加路径。搜索资源时会优先在路径下搜索
```
## nix-build
nix-build 命令用于构建由 Nix 表达式描述的派生 (derivations)。
- 构建成功时，在当前目录生成名为 result 的符号链接（symlink），指向构建结果。
- 无路径参数时，使用当前目录下的 default.nix。
- 支持 URL，路径以 http:// 或 https:// 开头时，会下载并解压 。要求包含 default.nix 文件。

## nix-shell
nix-shell 用于构建指定 derivation 的依赖项，但不会构建该 derivation 本身。

-  --pure:
启动一个 纯净环境，清除大部分环境变量，仅保留 HOME、USER 和 DISPLAY。
- --keep name
在纯净环境中保留指定的环境变量。
- --packages / -p packages
在 shell 中引入指定的包。

在 shell.nix 或 default.nix 中定义 shellHook，可以在进入 nix-shell 时执行自定义初始化脚本。如下：
```nix
...
  shellHook = ''
    echo "Welcome to the development environment!"
    export MY_VAR="some-value"
  '';
...
```
Nix 遵循 XDG Base Directory Specification 来管理配置：

    XDG_CONFIG_HOME：默认是 ~/.config
    XDG_STATE_HOME：默认是 ~/.local/state
    XDG_CACHE_HOME：默认是 ~/.cache
## nix-store
- nix-store --add 
 命令用于将指定的路径添加到 Nix 存储中，并输出结果路径。
- nix-store --add-fixed 
 命令用于将指定的路径添加到 Nix store，与 --add 命令不同，路径将使用指定的哈希算法进行注册。
- nix-store --gc 
命令用于运行 Nix store 的垃圾回收，删除那些在文件系统中无法通过“根”路径访问的 Nix store 路径。通过清理不再需要的路径，这个命令帮助保持 Nix store 的干净和节省空间。

# deterministic_checkpoints代码

## shell.nix文件
```nix
pkgs.mkShell {
  packages = [
	...
    pkgs.nix-output-monitor
 ];
```
创建一个nix环境，引入pkgs.nix-output-monitor，包含nom-build等可执行文件。
```bush
nom-build -A checkpoints -j 10  #指定构建当前目录下的default.nix里面的checkpoint属性。
```
使用 nom-build -A 可以执行和监控构建过程中的特定包或目标，并获得比 nix-build 更详细的输出和调试信息。

## default.nix文件(主目录)
### riscv64-cc
```nix
lib-customized = pkgs.callPackage ./lib-customized.nix {};  #引入linkFarmNoEntries函数
  ...
riscv64-cc = pkgs.pkgsCross.riscv64.stdenv.cc;
riscv64-libc-static = pkgs.pkgsCross.riscv64.stdenv.cc.libc.static;
...
spec2006 = pkgs.callPackage ./spec2006 {   #引入spec2006目录下的default.nix构建，一个基于 riscv64 架构的 SPEC CPU 2006 基准测试套件的程序
  inherit riscv64-cc riscv64-fortran riscv64-libc-static;
  inherit riscv64-jemalloc;
};
```
riscv64-cc 是为 riscv64 架构提供的交叉编译 C 编译器，用来在其他架构（如 x86_64 或 aarch64）上编译针对 riscv64 的程序。
### 
```nix
preBuild = ''
      sed -i 's/old/new/g' file
'';
```
preBuild 是在 buildPhase 之前执行的一步，它用于在构建开始前进行一些准备工作。上面是将file中的old全替换成new.
### gen_init_cpio
```nix
gen_init_cpio = pkgs.callPackage ./linux/initramfs/base/gen_init_cpio {};

     #gen_init_cpio下的defalut.nix:
	#		runCommandCC "gen_init_cpio" {
	#		  src = builtins.fetchurl {
	#		    url = "https://github.com/torvalds/linux/raw/f3b2306bea33b3a86ad2df4dcfab53b629e1bc84/usr/gen_init_cpio.c";
	#		    sha256 = "0i938rf0k0wrvpdghpjm4cb6f6ycz6y5y5lgfnh36cdlsabap71h";
	#		  };
	#		} ''
```
**gen_init_cpio 是一个特定于 Linux 的工具，用于生成 initramfs**， 允许开发者自定义 initramfs 的内容，包括所需的文件和目录结构。这里从guthub上下载gen_init_cpoi.c并且进行编译，将包命名为gen_init_cpio。
### initramfs_base
```nix
  initramfs_base = pkgs.callPackage ./linux/initramfs/base {
    inherit gen_init_cpio;
  };
 
#base下的defalut.nix
#	let
#	name = "init.cpio";
#	cpio_list = writeText "cpio_list" ''
#	...
#	'';
#	in runCommand name {} ''
#	 mkdir -p $out
#	  ${gen_init_cpio}/bin/gen_init_cpio -t 0 ${cpio_list} > $out/${name}
''

```
首先创建一个文本文件"cpio.list"，描述要包含在 CPIO 归档中的文件和目录的列表，然后会生成一个名为 init.cpio 的文件，位于 Nix 构建的输出目录中,这个文件包含了 cpio_list 中定义的所有目录和设备节点，可以被用作 Linux 系统的 initramfs。
***


