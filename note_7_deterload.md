# Deterload框架流程
![整体框架](https://github.com/tangjing2021/note/blob/main/img/deterload/deterload_%E6%95%B4%E4%BD%93%E6%9E%B6%E6%9E%84.png)
## nix的确定性
### 沙箱构建
Nix 构建过程在沙箱中进行，屏蔽了原环境的影响，例如配置、环境变量、依赖等。
### 环境管理
Nix 通过.nix文件显示的设置相关环境，任何依赖、配置、环境变量都可以被固定。
### 哈希验证
在开始构建前，Nix 可以检查输入(依赖、源代码、环境变量等)的哈希值，确保它们的内容与预期一致。

### 哈希敏感
Nix 存储（Nix store，通常位于 /nix/store）中的每个文件和目录路径都带有一个哈希值，例如：
```nix
/nix/store/hash-hello-2.10
```
其中 hash 是基于构建输入（如源代码、依赖项、构建脚本等）的哈希值。如果构建输入发生任何细微变化，这个哈希值都会改变，生成的新路径也会不同。如果某个依赖的内容或构建过程发生变化，所有依赖于它的包都会受到影响并重新生成。

**好处**
- 可重现性：不同机器上相同的输入可以得到完全相同的结果。
- 隔离性：不同版本的包可以共存，不会相互干扰。
- 一致性：依赖关系被精确控制，避免了依赖地狱问题。

**Nix 的回收机制**
- GC 根是 Nix store 中的某些特定路径或引用，Nix 会保证这些路径及其依赖的所有内容不会被回收。
- 使用nix-collect-garbage清理不再需要的构建结果，该命令会删除所有不再由任何 GC 根引用的路径。可以添加 --delete-older-than 选项来删除超过特定时间的旧版本
- 可以通过定期运行回收命令来自动清理空间。
## 整体流程

## imgBuilder
负责生成benchmark负载所需的镜像。
![imgBuilder](https://github.com/tangjing2021/note/blob/main/img/deterload/imgBuilder.png)
### opensbi
OpenSBI（Open Supervisor Binary Interface）是用于 RISC-V 架构的固件，负责在系统启动时初始化硬件并提供基本服务。
**功能：**
- 启动linux内核和 initramfs。
- 提供与硬件交互的sbi接口。

### dts
设备树文件DTS是描述硬件配置的文本文件，它定义了系统中各个硬件组件的信息，比如 CPU、内存等。用于给opensbi和linux内核识别硬件配置。

common-build它会将 DTS 文件编译成设备树二进制（DTB，Device Tree Blob），以供 OpenSBI 和 Linux 内核使用。

**用处**
- 硬件描述：
OpenSBI 需要知道系统的硬件配置，比如串口在哪个地址、内存布局如何，这些信息都通过设备树提供。
- 标准化接口：
设备树为不同的硬件平台提供了标准化的硬件描述方式，使得 OpenSBI 和 Linux 能够在不同硬件上无缝运行。
- 动态配置：
通过修改设备树，可以在不改变 OpenSBI 或 Linux 源代码的情况下，调整硬件配置。
### Linux 
Linux 内核负责在引导完成后提供操作系统环境,加载 initramfs 并启动其中的 run.sh 脚本
### Initramfs 
Initramfs（Initial RAM Filesystem）是启动时加载的内存根文件系统，用于初始化系统环境，提供了一个基础操作系统环境，包含运行 Benchmark 所需的库、工具和脚本（如 run.sh），保证 Benchmark 能够在隔离环境中独立运行。
### Overlays
在基础环境的基础上，应用 overlays 添加或修改特定依赖。

### run.sh
启动benchmark的脚本。
## cptBuilder
对benchmark进行切片操作。
![cptBuilder](https://github.com/tangjing2021/note/blob/main/img/deterload/cptBuilder.png)
### QEMU、NEMU 
用于模拟和仿真的工具。
### stage1-profiling
阶段1，使用仿真器来跑benchmark，将benchmark进行分段，每一段生成一个基本块向量BBV。
- 基本块：指的是指令流中，一段单进单出的指令，尺度较小。
- 分段：这里用的是固定分段，大小约为100M，在分段的交接处可能会为了保持基本块逻辑上的连贯额上下波动几个指令，影响不大。
- 基本块向量BBV：统计所有基本块在一段分段里的权重。元素长度为程序的基本块个数。每一项都是指定基本块在该分段的调用次数乘以基本块的指令数，在此基础上可以进行归一化处理。
- 降维：程序的基本块数量很大，为了降低计算量可以对基本块进行投影降维，一般降到15维即可。
### stage2-cluster
阶段2，对产生的所有BBV进行聚类分析，并输出每个聚类的代表点。 
- 聚类算法k-means：不断迭代，直至聚类中心稳定。
- 代表点选取：为每个聚类选取一个能够代表整个聚类的BBV。一般默认选取距离聚类中心最近的BBV，还有其他的数学选取方案，如基于高斯分布。
- BIC：贝叶斯信息准则，用于评定k个聚类的聚类方案的好坏。
- simpoint：用于该阶段的聚类处理。
### stage3-checkpoint
阶段3，根据代表点产生切片。
使用仿真器再次运行benchmark，找到代表点对应的片段，记录该片段的现场。





