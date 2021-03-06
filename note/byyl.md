# Note #

## 语言部分 ##

### 1. 变量及其属性 ###

> 变量是对一个(或若干个)存储单元的抽象，赋值语句则是修改存储单元内容的抽象。

变量的属性

1. 作用域 可访问该变量的程序范围
    - 静态作用域绑定:按照程序的语法结构定义变量的作用域。
    - 动态作用域绑定:按照程序的执行动态地定义变量的作用域。
2. 生存期 一个存储区绑定于一个变量的时间区间
3. 值 即变量对应存储区单元的内容
4. 类型 变量的类型是与变量相关联的值的类,以及对这些值进行的操作的说明。

### 2. 虚拟机的概念 ###

> 虚拟机是由实际机器加软件实现的机器

实际机器 M1  
执行汇编语言的虚拟机 M2 = M1 + 汇编程序  
执行高级语言的虚拟机 M3 = M2 + 编译程序

### 3. 程序单元及单元实例 ###

> 程序单元: 程序执行过程中的独立调用单元。  
> 单元表示：
> - 在编译时，单元表示是该单元的源程序。
> - 运行时，单元表示由一个代码段和一个活动记录组成，称为单元实例

> `活动记录` 执行单元所需要的信息,以及该单元的局部变量所绑定的数据对象的存储区。  
> `非局部变量(Nonlocal Variable)` 一个程序单元可以引用未被本单元说明而被其它单元说明的变量。  
> `全局变量(Global Variable)` 各个程序单元都可以引用的变量  
> `副作用` 对绑定于一个非局部变量的对象进行修改时，将产生副作用。  
> `程序单元的引用环境(Referencing Environment)` 局部变量+非局部变量  
> `直接递归(Direct Recursion)` 一个程序单元自己调用自己  
> `间接递归(Indirect Recursion)` 一个程序单源调用别的程序单元，再由别的程序单源调用这个程序单元。一个程序单元可能有多个实例，同一程序单元的不同实例的代码段是相同的，活动记录不同。

### 4. 数据类型的作用 ###

> 数据类型实质上是对存储器中所存储的数据进行的抽象。它包含了一组值的集合和一组操作。

数据类型的作用

- 实现了数据抽象
- 使程序员从机器的具体特征中解脱出来
- 提高了编程效率

数据类型的分类

- 内部类型：语言定义的
- 自定义类型：用户定义的

### 5. 数据聚合的六种方式 ###

1. 笛卡尔积（结构）
2. 有限映像（射）（数组）
3. 序列 （串）
4. 递归（指针）
5. 判定或（联合）
6. 幂集 （集合构造）

### 6. 抽象数据类型的条件 ###

抽象数据类型的特性：

1. 在允许实现这个新类型的程序单元中，建立与表示有关的具体操作。
2. 对使用这个新类型的程序单元来说，新类型的表示是隐蔽的。

> 内部类型是对(硬件)二进制位串的抽象。用户定义类型是对内部类型和已定义的用户定义类型作为基本表示的抽象。

> 内部类型的基本表示是不可见的，而用户自定义类型的基本表示是可见的。  

具有信息屏蔽、封装、继承等特性的类型为抽象数据类型。它是以内部类型和用户定义类型为基本表示的更高层次抽象，它的基本表示也是不可见的。

### 7. 类型检查及分类（静态和动态） ###

> 对数据对象的类型和使用的操作是否匹配的一致性检查称为类型检查

- 动态检查 编译时就能进行的检查

- 静态检查 运行时才能进行的检查

### 8. 何谓类型等价（结构和名字） ###

1. 名字等价     两个变量的类型名一样
2. 结构等价     两个变量的类型具有相同的结构

### 9. 语句级控制结构（顺序、选择和重复） ###

- 顺序（sequencing）
- 选择（selection）
- 重复（repetition）

### 10. 单元级控制结构（4个名字） ###

- 显式调用
- 异常处理
- 协同程序
- 并发单元

### 11. 副作用（非局部变量。。。）和别名的概念 ###

> 副作用 非局部变量的修改  
降低了可读性，限制了数学运算律的使用，影响优化

> 别名 两个变量表示同一数据对象

### 12. 语言的定义 ###

> 程序设计语言是用来描述计算机所执行的算法的形式表示。由两部分组成，语法和语义。

> 语法： 定义形成形式上正确语言句子的规则
  - 用文法描述
  - 用识别图（语法图）描述
> 语义： 定义语言合法句子的含义

### 13. 文法的定义 ###

> G=(VT,VN,S,P)

```what is this
    V* VN V*, V*
```

### 14. 文法的分类 ###

- 0型文法（短文法）  
    V* VN V*, V*
- 1型文法 (上下文有关文法)  
    ││<=│β│（S→例外）
- 2型文法 (上下文无关文法)  
    A→
- 3型文法 (正则文法)
    A→或A→B VT

### 15. 语法描诉的基本用途（使用者、设计者、实现者三个角度） ###

- 表达语言设计者的意图和设计目标;
- 指导语言的使用者如何写一个正确的程序；
- 指导语言的实现者如何编写一个语法检查程序来识别所有合法的程序。

### 16. 抽象机 ###

> 描述语义。它由一个指令指针、一个存储器组成。存储器分为数据存储器和代码存储器。

### 17. 推导、句型、句子、句柄、语言 ###

- 规范推倒： 每次被替换的非终结符都是最右边的
- 规范规约： 最左规约

### 18. 语法树、推倒树 ###

**************

- 汇编程序: 将汇编语言的程序翻译为机器语言的程序
- 编译程序: 将高级语言程序翻译为低级语言的程序
- 绑定：一个实体（或对象）与其某种属性建立起某种联系的过程,称为绑定。

**************

## 编译部分 ##

### 编译、翻译的概念 ###

翻译:将一种语言编写的程序转换成完全等效的另一种语言编写的程序的过程称为翻译（translate）；在计算机中，翻译由一个程序来实现，称为翻译程序(translator)

编译：将高级语言程序翻译为低级语言程序。

### 词法分析器的功能 ###

输入字符串，根据词法规则识别出单词符号。  

### 单词符号的分类（标识符、基本字、常数、运算符、界符） ###

- 标识符
- 基本子
- 常数
- 运算符
- 界符

### 状态转换图 ###

节点代表状态，节点之间的有向边代表状态之间的转换，有向边上标记的字符代表状态转换的条件。

### 公共左因子、左递归的消除 ###

 。。。。


### FIRST集、FOLLOW集、预测分析表的构造 ###

................................

### 短语、直接短语、句柄、素短语 ###

短语： 任何子树的边缘都是根的短语  
直接短语： 只有一代的子树的边缘是根的直接短语  
句柄：一个句型的最左直接短语  
素短语：包含终结符的最小短语
最左素短语：句型中最左面的素短语  

### 算符优先文法的构造 FIRSTVT LASTVT ###

FIRSTVT: 任一非终结符P FIRSTVT（P）是P的所有可能推倒的第一个终结符组成的集合
LASTVT: 任一非终结符P LASTVT（P）是P的所有可能推倒的最后一个终结符组成的集合

### LR（0） SLR（1） ###


### 何谓语法制导翻译，翻译结果（四元式三地址式）、语义子程序 ###

语义分析：  

- 语义检查
  - 主要进行一致性检查和越界检查。
- 语义处理
  - 说明语句 等级信息（符号表）
  - 执行语句 生成中间代码

> 语法制导翻译：在语法分析过程中，根据每个产生式对应的语义子程序（语义动作）进行翻译（生成中间代码）的方法称为语法制导翻译。  


#### 1. B ####

B -> i

```u
    B.T:=ip;
    gen(jnz,entry(i),-,0);
    B.F:=ip;
    gen(j,-,-,0)
```

B -> i1 rop i2

```u
    B.T = ip
    gen(jrop,entry(i1),entry(i2),0)
    B.F = ip
    gen(j,-,-,0)
```

#### 2. if B then S1 ####

M -> if B then

```n
    backpatch(B.T, ip)
    M.CHAIN = B.F
```

S -> M S1

```n
    S.CHAIN = merge(S1.CHAIN, M.CHAIN)
```

#### 3. if B then S1 else S2 ####

M -> if B then

```n
    backpatch(B.T, ip)
    M.CHAIN = B.F
```

N -> M S1 else

```n
    p = ip
    gen(j,-,-,0)
    backpatch(M.CHAIN, ip)
    N.CHAIN = merge(S1.CHIAN, p)

```

S -> N S2

```n
    S.CHAIN = merge(N.CHIAN, S2.CHAIN)
```

#### 4. while B do S1 ####

W -> while

```u
    W.CODE = ip
```

D -> W B do

```u
    backpatch(B.T, ip)
    D.CHAIN = B.F
    D.CODE = W.CODE
```

S -> D S1

```u
    backpatch(S1.CHAIN, D.CODE)
    gen(j,-,-,D.CODE)
    S.CHAIN = D.CHAIN
```

#### for i:=1 to N do S1 ####

F -> for i:=1 to N do

```u
    gen(:=,1,-,i)
    F.AGAIN = ip
    F.PLACE = entry(i)
    gen(j<=,i,N,F.PLACE+2)
    F.CHAIN = ip
    gen(j,-,-,0)
```

S -> F S1

``` u
    backpatch(S1.CHAIN, ip)
    gen(+,F.PLACE,1,F.PLACE)
    gen(j,-,-,F.AGAIN)
    S.CHAIN = F.CHAIN;
```

 do S while E

D -> do
D.CODE = ip

W -> D S while
W.CODE = D.CODE
backpach(S.CHAIN, ip)

S -> W E
S.CHAIN = E.F
backpatch(E.T, W.CODE)




优化是一种等价的，有效的程序变换  
`等价` 不改变程序的运行结果  
`有效` 时空效率高

`源程序优化` 数据结构和算法  
`编译优化` 中间代码优化和目标代码优化  
`中间代码优化` 局部优化和全局优化c




### 局部优化的方法 ###

> 基本块： 只有一个入口和一个出口的一段语句序列。

局部优化：在基本块内的优化

1. `合并已知量`
    ```u
        A = OP B
        A = B OP C
    ```
    若B和C为常数，在编译时计算出来 `A -> T`
2. `删除公共子表达式`
    ```u
        A:=B+C*D
        U:=V-C*D
    ```
    C和D进行了同样的运算，`T:=C*D, A:=B+T, U=V-T`
3. `删除无用赋值`
    ```u
        A = B + C
        A = M + N
    ```
    第一条是多余的
4. `删除死代码`

### 全局优化的方法 ###

> **`循环`** 循环是程序流图中具有唯一入口的强连通子图

> **`必经节点集`**

> **`回边`**

循环的优化：

1. `代码外提`
2. `强度削弱` 基本归纳变量，同族归纳变量
3. `删除循环变量`

### 寄存器的分配原则 ###

。。。。。。。。。。。。。。。。。

### 活动记录的内容 ###

> 活动及录是一个程序单元的数据空间。一个程序单元被激活时，对它的信息管理是通过相应的活动记录来实现的。  

`活动记录的内容`：

- 返回地址（被调用单元活动结束后返回调用单元）
- 动态链接（恢复活动记录）
- 静态链接（引用非局部环境）
- 现场保护（保存机器状态）
- 参数个数（由调用单元传递给被调用单元的参数个数）
- 形式单元（为被调用单元的形参分配存储空间，以备调用单元传递实参信息）
- 局部变量
- 临时变量

**`静态存储分配`** 编译时进行存储分配，将变量绑定于一个固定的存储区域  
**`动态存储分配`** 运行时完成存储分配。（栈分配，堆分配）

静态分配时，活动及录的实际地址在编译时确定。动态分配时，对一个局部变量分配的地址是在活动记录上的位移（OFFSET）。

### 变量的存储分配 ###

#### 1. 静 态变量 ####

> 如果每个单元的活动及录，以及每个变量的存储位置在编译时均可确定，在运行时不会改变，那么存储分配可在编译时完成。静态分配将变量绑定于一个存储区，不管在程序单元的哪一次活动中，这些变量都绑定于相同的存储区，这类变量称为静态变量。

#### 2. 半静态变量 ####

> 如果一个语言允许程序单元的递归调用，即一个程序单元可以被多次递归的激活，就要为该程序单元建立多个活动及录。变量在单元每次被激活时动态地绑定于刚才建立的活动记录。这类变量的长度必须是编译时可确定的，因而活动及录的长度、变量在活动记录中的相对位置都是编译时可确定的。运行时，一旦该单元被激活，相应的活动及录便获得物理存储区D，变量便绑定于D+offset，这类变量称为半静态变量。

#### 3. 半动态变量 ####

> 例如动态数组，长度在编译时不确定。活动及录的描述符包含：指向它在活动及录首址的指针，每一维的上下界。

#### 4. 动态变量 ####

> 数据对象的长度在运行时仍可能改变

### 三种分配方式（静态、栈式、堆分配） ###

#### 1. 静态为配 ####

> 只允许静态变量，变量与存储器的绑定关系在编译时确定。不允许递归调用，不允许动态数组，不允许动态类型的数据变量。每个程序单元对应一个单元实例（一个代码段，一个活动记录）

#### 2. 栈式分配 ####

> 对于半静态和半动态变量，所有的局部变量在单元激活时隐式建立，活动记录长度或静态可确定或激活时可确定。同时，各单元之间调用关系满足先进后出关系。

#### 2. 堆式分配 ####

> 动态变量长度个数在执行中动态改变




### 仅含半静态变量的栈式分配（条件、过程调用语句的翻译、返回语句的翻译） ###

（动态作用于也是这个）
CALL P

```u
    1. 保存返回地址
        D[free] = ip + 5
    2. 保存主调过程的 current
        D[free + 1] = current
    3. 建立 P 的c urrent
        current = free
    4. 调整 free
        free = free + L
    5. 修改 ip
        ip = P 的代码段首地址
```

RETURN

```u
    1. 恢复 free
        free = current
    2. 恢复 current
        current = D[current + 1]
    3. 返回
        ip = D[free]
```

### 允许过程嵌套定义栈式分配（条件、过程调用语句的翻译、返回语句的翻译） ###

对非局部变量，引用的应是最近的“嵌套外层”中说明的变量

静态连接：指向最近外层的最新活动记录的指针，它处于活动记录位移为2的存储单元中

静态作用域下：

```u
A CALL B
    1. D[free] = ip + 6
    2. D[free + 1] = current
    3. D[free + 2] = f(na-nb+1)
    4. current = free
    5. free += L
    6. ip = B 的代码段首地址

B RETURN A
    1. free = current
    2. current = D[current + 1]
    3. ip = D[free]
```


### 静态作用于规则、动态作用于规则 ###

> 静态作用于规则 最近外层是静态嵌套外层

> 动态作用域规则 最近外层
是动态调用外层（此时与静态链与动态链一致）

### 数据参数传递的几种方式（传值、传地址、名调用) ###

被调用单元的参数成为形参，调用单元的参数成为实参

#### 引址调用 ####

> 把实参的地址传递给相应的形参。对形参的任何引用和赋值都被处理为间接访问。

#### 值调用 ####

1. 传值
2. 得结果
3. 传值得结果

#### 名调用 ####

> 调用的作用相当于把被调函数的函数单元超抄写到调用成都单元调用出现的地方。

**************

## 自上而下 ##

### 回溯分析法 ###

### 递归下降分析法 ###

在不含左递归和公共左因子的文法G中，可能构造一个不带回溯的自上而下的分析程序，由一组递归过程组成，每一个过程对应文法的一个非终结符。

### 预测分析法 ###

预测分析法组成：

- 下推栈
- 预测分析表
- 控制程序

LL（1）文法 对输入串从左到右扫描并自上而下分析，仅仅利用当前的非终结符和向前查看一个输入的符号就能唯一确定采取什么动作。

如果文法G的预测分析表M不含多重定义入口，那么G是LL（1）文法。

## 自下而上 ##

### 算符优先分析法 ###

算符文法： 不含空产生式且任何产生式的右部都不含两个相连的非终结符。

### LR 分析法 ###

从左到有扫面，自下而上进行规约。  

- 分析表
- 分析栈（状态栈、符号栈）
- 控制程序