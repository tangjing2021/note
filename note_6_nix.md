# Advanced Attributes
为 Nix 提供了更细粒度的控制，使得构建过程更加灵活，确保输出的构建结果符合预期，并避免不必要的副作用或环境污染。
## 一 控制运行时依赖
为生成的构建结果在运行时提供需要的外部资源或文件路径。
### 构建结果：
指构建任务生成的最终产物，比如可执行文件、脚本、配置文件或静态资源。
### 构建时依赖：
是为了完成构建任务而需要的工具、库或资源，但它们可能不是产物运行过程所需要的。
```nix
...
buildInputs = [gcc glibc];  #gcc只在编译阶段需要，之后就不需要了
...
```
### 运行时依赖
是生成的产物在实际运行过程中需要加载或使用的内容。
### 相关操作
`allowedReferences`相当于一个白名单，运行时的依赖除了名单上的，其他的不允许直接含有，如果有则会构建失败。
```nix
...
allowedRequisites = [ glibc ]; #输出的二进制文件的运行时依赖 只能 包含 glibc（例如 /nix/store/xyz-glibc/lib/...）及其路径。 
...
```
目的是严格限制构建输出的直接运行时依赖，确保生成的二进制文件只能依赖指定的 glibc 包，而不能无意中依赖其他未声明的路径

`allowedRequisites` 递归检查整个依赖闭包，用于更严格的依赖约束。例如：
```rust
#假设 hello 的依赖闭包是：
hello -> glibc -> libpthread -> ld-linux
```
```nix
allowedRequisites = [ glibc ]; #这意味着整个依赖闭包只能包含 glibc 的路径。
```
如果 libpthread 和 ld-linux 不是显式列在 allowedRequisites 中，Nix 会报错，指出构建结果的闭包中存在未被允许的路径,如下：
```bash
error: output '/nix/store/ayaspay8briniijrhvy73zxkr59i3w4v-hello-1.0' is not allowed to refer to the following paths:
         /nix/store/55byk2fn6548ni8ibgd2dyzpmk4z180w-gcc-12.2.0-lib
         /nix/store/f52mk615m62iq2xs02wcpdh8sw560afm-libunistring-1.1
         /nix/store/jd99cyc0251p0i5y69w8mqjcai8mcq7h-xgcc-12.2.0-libgcc
         /nix/store/sa4iglrbr3kyaivdxc2svpfbfyp98dx8-gcc-12.2.0-libgcc
         /nix/store/wfnx7nlgjcylwfcqk117v0l3dnpj5l1b-libidn2-2.3.4
```
`disallowedReferences` 与allowedReferences相反，相当于一个黑名单，指定了构建输出不允许的直接非法引用（依赖）。

`disallowedRequisites`与allowedRequisites 相反，指定构建结果的闭包中不允许存在的路径。

`exportReferencesGraph` 接受一个路径和名称的列表，每个元素指定了一个要导出引用图的路径。构建器会生成这些路径的引用图并将其存储在构建目录中。如下
```nix
  ...
  exportReferencesGraph = [ "libfoo-graph" libfoo ];
  ...
```
`impureEnvVars` 接受一个环境变量列表，让指定的环境变量从调用环境传递给构建过程。一般在构建时环境会被完全清空，但这个属性允许某些环境变量未经修改地传递给构建器。

`passAsFile` 用于在构建过程中将某些较大的字符串或数据通过临时文件的方式传递给构建环境，而不是通过环境变量，并通过一个以 Path 结尾的环境变量传递文件路径。如下：
```nix
big = ... ;
# 声明 big 应作为文件传递,并且生成环境变量bigPath指向临时文件路径
passAsFile = [ "big" ];
```
`__structuredAttrs` **Nix 的 mkDerivation 函数会对属性进行严格的类型检查和转换**，默认情况下会尝试将属性转换为字符串。当 __structuredAttrs = true 被设置时，Nix 会允许属性保留其原始的结构化形式（如集合、列表等），并且其他属性会以 JSON 格式序列化，并存储在构建器临时目录中的 `.attrs.json` 文件中。如果构建器使用 Bash 脚本，Nix 会自动生成一个 `.attrs.sh` 文件，初始化对应的 Shell 变量。例如：
```nix
...
  __structuredAttrs = true;

  exp1 = {
    s1 = "Hello";
    s2 = "World!;
  };
...
```
属性会被转换为普通变量或 Bash 关联数组：
```bash
...
  echo ${exp1.s1}  #输出Hello
  echo ${exp1.s2}  #输出World
...    
```
`outputChecks` 在多输出场景下，可以对每个输出单独定义检查规则：
```nix
  outputs = [ "out" "dev" "doc" ];
  # 检查规则
  outputChecks.out = {
    allowedRequisites = [ glibc ];  #递归白名单
    maxClosureSize = 256 * 1024 * 1024;  # 主输出闭包大小不得超过 256 MiB
  };
  outputChecks.dev = {
    maxSize = 128 * 1024;  # 开发文件大小不得超过 128 KiB
  };
```

## 二 Nix 的二进制缓存
二进制缓存是 Nix 构建系统的一个核心概念，它存储已经构建好的 Nix 包或存储路径的二进制文件。通过使用二进制缓存，用户可以避免重复构建软件包，从而节省时间和资源。

### 类型
**默认的 Nix 二进制缓存**：https://cache.nixos.org 是 Nix 的官方二进制缓存，提供了大部分常用软件包的二进制文件，默认被 Nix 信任并配置为可信来源。

**自定义二进制缓存**：用户可以配置自己的二进制缓存，用于本地开发或私有环境。

### 二进制缓存的作用
   **加速构建**：
        如果某个包已经在缓存中存在，可以直接下载而无需重新编译，减少重复计算。
        
   **提高可重现性**：
        Nix 使用存储路径的哈希值（与包的输入完全绑定）确保二进制文件的唯一性。只要依赖、配置相同，就能从缓存中获得完全相同的二进制文件。

   **减少本地存储需求**：
        使用缓存可以避免将所有中间结果保存在本地。

   **支持分布式开发**：
        团队或社区可以通过共享二进制缓存高效协作，确保所有人使用相同的构建输出。


## 三 内置函数
### fetchClosure 
用于从二进制缓存中提取一个存储路径的闭包。fetchClosure 提供了三种主要调用方式：

1. 获取内容寻址（Content-Addressed）存储路径：
```nix
builtins.fetchClosure {
  fromStore = "https://cache.nixos.org"; #指定二进制缓存的来源  Nix的官方二进制缓存
  fromPath = /nix/store/ldbhlwhh39wha58rm61bkiiwm6j7211j-git-2.33.1; #要获取的内容寻址存储路径。
}
```
从指定的二进制缓存（fromStore）下载指定路径的内容（fromPath）。然后，它返回一个指向该内容的路径。

2. 获取任意存储路径并将其重写为内容寻址
```nix
builtins.fetchClosure {
  fromStore = "https://team-binary-cache.example.com";
  fromPath = /nix/store/r2jd6ygnmirm2g803mksqqjm4y39yi6i-custom-library-2.0; #输入寻址，进行下载
  toPath = /nix/store/ldbhlwhh39wha58rm61bkiiwm6j7211j-custom-library-2.0;   #重新写入到内容寻址 
}
```
Nix 会从缓存中获取 fromPath 并将其重写为内容寻址的路径 toPath。如果不知道 toPath 的确切值，可以使用以下命令计算：
```bash
nix store make-content-addressed --from https://cache.nixos.org /nix/store/r2jd6ygnmirm2g803mksqqjm4y39yi6i-git-2.33.1
```

3. 直接获取输入寻址存储路径
```nix
builtins.fetchClosure {
  fromStore = "https://cache.nixos.org";
  fromPath = /nix/store/r2jd6ygnmirm2g803mksqqjm4y39yi6i-git-2.33.1;
  inputAddressed = true;
}
```
直接从指定的二进制缓存中获取指定的输入寻址路径。
### fetchGit
```nix
builtins.fetchGit {
  url = "https://github.com/.../....git";
  rev = "...";
  ref = "master";
}
```
ref : 指定 Git 仓库中用于查找修订版的引用。引用通常是一个分支或标签名。例如，master、v1.0。如果未指定，默认使用 HEAD。
rev : 指定要拉取的 Git 修订版（commit）。通常是一个提交的哈希值。如果没有给出，默认会使用 ref 对应的最新提交。
## 四 deterload
### 背景

确定性负载（Deterload）是一个为香山生态（包括 香山处理器、 香山NEMU 和香山GEM5 ）生成确定性工作负载的框架。
### 确定性
确定性意味着无论何时何地，两次构建同一个工作负载，都能得到完全相同的结果。换言之，Deterload 提供了一个高度可控的环境，使生成的工作负载在内容和行为上具备可复现性。
例如：
假设你在两台机器 A 和 B 上使用 Deterload 构建一个名为 benchmark1 的工作负载：
机器 A 是 Windows 系统的虚拟机，机器 B 是运行 Linux 的物理机。
两次运行 nom-build -A benchmark1 后，生成的工作负载镜像 result/benchmark1.img 的内容完全相同，字节级别一致。
这表明构建过程的输入（依赖、源码、编译器等）和行为（时间、随机数种子等）均被严格控制。
### 为什么需要“确定性”
重现成功：如果构建工作负载不具备确定性，即便开发者提供了相同的代码，在不同环境中构建时，结果可能完全不同，导致开发效率低下。

例如：
在一台支持 C++11 的电脑上编写代码，使用了 auto 关键字。在该电脑上进行编译，编译成功。
然后将代码发送到另一台仅支持 C++10 的电脑上进行编译。
因为 C++10 不支持 auto，导致编译失败。

重现错误：开发者能够轻松重现特定的错误和性能问题，无需担心由于环境差异或构建过程中的随机性而无法复现。
### 如何实现“确定性”
使用 Nix 包管理器：
- Nix 是一个功能强大的确定性包管理工具，能够在沙箱环境中构建工作负载，确保所有依赖、输入和构建过程是固定的。
- 它通过精确描述依赖版本、编译器工具链、系统库等，消除环境差异导致的不一致。
- 即使在不同的机器或操作系统上运行，同样的 Nix 表达式总会产生相同的结果。
### 缺陷
 - nix的复杂性较高：
 尽管 Nix 提供了确定性的构建环境，但其学习曲线较陡峭。Nix 的语法和生态系统对许多开发者来说是一个挑战，尤其是在需要自定义配置时。使用 Nix 进行构建、配置和调试都需要对 Nix 表达式和依赖管理有深入了解。
- 构建开销：
Nix 的沙箱构建过程可以使用binary cache进行缓存来降低开销，但首次进行构建时没有相关缓存，开销可能会大一些，特别是在有大量依赖的情况下。
### 未来发展方向
- 简化用户体验：
随着 Nix 和 Deterload 的成熟，未来可以开发更加易于使用的接口和工具，降低使用门槛。通过图形化界面、自动化配置工具或者增强的文档，降低开发者对 Nix 表达式和沙箱机制的依赖，提升开发效率
- 在保持确定性的前提下，进一步优化构建过程中的性能。
