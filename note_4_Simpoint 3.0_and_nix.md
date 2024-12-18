# Simpoint 3.0 的优化

## 1. 高效的聚类搜索：
SimPoint 3.0在聚类时，采用了二分查找来选择聚类数量k，而不是传统的遍历所有可能的k值。这一改进通过减少搜索空间，显著提高了算法效率，使得模拟过程可以更快地完成。

## 2. 支持可变长度区间：
SimPoint 3.0扩展了支持可变长度区间的能力。在以前的版本中，SimPoint使用固定长度区间，意味着每个区间代表固定数量的指令执行时间。而在SimPoint 3.0中，不同区间可以包含不同数量的动态指令，这使得模拟更加灵活和精确。由于区间长度不同，聚类过程中需要考虑每个区间的权重，即每个区间的“重要性”或所代表的指令数。

## 3. 大规模区间处理的加速：
为了加速SimPoint在处理大量输入时（数十万到百万个区间）的执行，待聚类的区间集合进行了子采样。在聚类后，未被选中的区间会根据其与最近聚类的距离被分配到相应的阶段。


### 原始步骤： 
假设有N个区间，SimPoint 的原始方法会对所有这N个区间进行聚类计算。这会导致计算量巨大，尤其是区间数目很多时，处理速度会很慢。
### 引入子采样后：
 在进行聚类之前，SimPoint 3.0会从这些N个区间中随机选择一个较小的子集（比如选择M个区间，M远小于N）进行聚类。然后，SimPoint会根据聚类结果，将未被选择的区间（即N-M个区间）分配到对应的相似阶段（phase）。这个分配过程是通过计算这些未选区间与已选区间聚类的距离来完成的。
### k-means的收敛机制：
- 迭代上限：k-means会在达到用户指定的最大迭代次数（默认为100次）时停止运行；
- 收敛条件：若聚类中心（centroids）在连续迭代中不再变化，则算法提前收敛。
- 默认设置：
SimPoint默认设置允许100次迭代，但这一值可以调整甚至取消限制。
当聚类的区间数远大于聚类的簇数时，可能需要超过100次迭代才能收敛。
- 实验观察：
针对SPEC 2000基准测试中所有程序的实验（10百万指令区间、k=30、15维、10个随机初始化），仅1.1%的运行超过了100次迭代。绝大部分程序都在100次迭代内收敛。

## 4. 减少模拟点的数量：
为了进一步减少模拟时间，SimPoint 3.0提供了一个新选项，只输出那些代表程序执行大部分时间的模拟点。这些模拟点的选择基于聚类的结果，目标是只选取那些在整个执行过程中占主导地位的区间。通过排除权重较小的模拟点，可以显著减少模拟时间，同时仅对分析精度产生轻微影响。

## 5. 常见陷阱

在确保准确使用SimPoint的模拟点时，有几个重要的潜在问题值得注意。
- MaxK的选择：
在进行模拟时，MaxK的设置直接影响模拟的时间和精度。设置一个较低的MaxK可以减少模拟时间，但可能会将一些具有不同特征的区间归为同一个聚类，降低准确性。如果希望精度更高，可以选择增加MaxK的值，并结合-coveragePct选项来控制最终选出的模拟点数量，从而在保证准确性的同时减少模拟时间。

- 错一位问题：
SimPoint 3.0从0开始计数区间，而不是1，这可能导致理解上的错误。**区间编号和聚类编号都是从0开始的**。

- 可重复性和区间选择：
为了确保模拟结果的可重复性，用户需要保证每次模拟中区间的编号和指令计数一致。通过SimPoint提供的区间编号，用户可以轻松追踪每个模拟点的准确位置，并根据需要计算指令数。

## 6. 如何提高SimPoint模拟的可重复性
在不同的模拟环境中，指令计数可能会受多种因素的影响（硬件差异、缓存效应、操作系统等），导致同一程序在不同实验中的指令数量有所波动。而程序计数器（PC）所标记的执行位置则更加关注程序的逻辑控制流，因此它比指令计数更加稳定。

### 使用程序计数器（StartPC）：
它能够确保每次模拟的起始点在每个执行区间的一致性，避免了仅使用指令计数时可能产生的误差。程序计数器的值可以在不同的执行周期中重复出现，因此需要跟踪每个程序计数器的值及其出现次数，以确保模拟点的准确性。

### StartPC和调用次数：
对于每个区间，SimPoint会选择一个StartPC，这个PC值在执行中会被触发一定次数。记录下PC值及其触发次数，能够帮助用户精确定位每个区间的开始时间，避免因模拟环境变化而带来的细微差异。

### 确保一致的模拟环境：
使用StartPC时，必须确保二进制文件和共享库在每次模拟运行中加载到相同的地址位置。这对于确保多个运行之间的一致性至关重要，因为程序地址流（包括指令和全局数据）必须保持一致，否则可能影响模拟的准确性和可重复性。实际操作如下：
- 禁用ASLR：可以通过操作系统的设置禁用ASLR，确保每次运行时程序和共享库加载到相同的内存地址。
- 固定内存地址：某些系统和编译选项允许程序加载到固定的内存地址，这样可以避免加载顺序变化对程序计数器的影响。
- 环境一致性：确保每次运行时使用相同的硬件、操作系统环境以及编译选项，这样可以最大限度地减少环境变化对结果的影响。
	


## 关于CPI误差
在进行架构设计空间探索时，CPI误差（即与基准值的偏差）并不是最关键的因素。更重要的是确保在不同架构之间，CPI误差的一致性。即，虽然在某些情况下，CPI误差可能存在一定偏差，但这种偏差在不同的架构之间应当是一致的。这使得在对比不同架构时，所得到的相对误差不会因为误差的变化而产生不一致的结论。



#[Simpoint基础ppt](https://github.com/tangjing2021/note/blob/main/Simpoint%E5%9F%BA%E7%A1%80.pptx)

---

# Nix相关

`import` 表达式以其他 Nix 文件的路径为参数，返回该 Nix 文件的求值结果。

`import` 的参数如果为文件夹路径，那么会返回该文件夹下的 default.nix 文件的执行结果。

列表使用中括号闭合，空格分隔元素，一个列表允许包含不同类型的值：
```nix
[ 123 ./foo.nix "abc" (f { x = y; }) ]
```

## 属性集

属性名可以是标识符或字符串。标识符必须以字母或下划线开头，可以包含字母、数字、下划线、撇号（'）或连接符（-）。
```nix
{
  x = 123;
  text = "Hello";
  y = f { bla = 456; };
}
```
使用 . 访问各个属性，使用 or 关键字，可以在属性选择中提供默认，属性名也支持字符串插值：
```nix
{ a = "Foo"; b = "Bar"; }.c or "Xyzzy"

let
bar = "foo"; 
in
{ foo = 123; }.${bar}
```
如果一个集合的 __functor 属性的值是可调用的（即它本身是一个函数或是其中一个集合的 __functor 属性的值是可调用的），那么它就可以像函数一样被应用，首先传入的是集合本身，例如：
```nix
let add = { __functor = self: x: x + self.x; };
    inc = add // { x = 1; };        #将属性集 {x=1;} 作为 self 传入 add
in
 	inc 1           #从第二个参数开始传入
```

## 模块系统
一个成熟的模块大概由三个部分组成：导入（imports）、选项（options）与配置（config，或者叫做定义）。

被包含的模块只有 options 是对外部可见的，里面定义的函数与常量都是在本地作用域定义的，对其他文件不可见。同时，被 imports 组织的模块集合中的任意模块都能访问任意模块的 options，也就是说，只要是被 imports 组织的模块，其 options 是全局可见的。
