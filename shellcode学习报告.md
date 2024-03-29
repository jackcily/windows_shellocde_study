###                                                shellcode学习报告

#### shellcode编写方法

---

shellcode的编写是有一定规律的，因此简单的shellcode编写是有格式的，为了方便shellcode的调试，采用在c语言中`内联汇编`的方式。

以下实验均为 `vs2019 x86` 调试环境。

假设要将如下代码中的 system函数的调用转换为shellcode的调用。

```c
#include <windows.h>
int main ()
{
    system(“dir”);
    return 0;
}
```

为了实现函数调用，首先要知道函数在内存中的地址。（由于`ASLR`的原因，函数的内存地址在每台机器上可能会不一样）可以使用动态加载dll的方式获取，[代码在此 sys_inline_shellcode.c](https://github.com/jackcily/windows_shellocde_study/raw/master/file/sys_inline_shellcode.c)。

执行结果如下：

![1](https://github.com/jackcily/windows_shellocde_study/raw/master/file/system.JPG)



分析上面的shellcode，编写顺序大致如下：

首先保存栈指针和寄存器内容，用于出栈的操作。

```asm
push ebp
mov ebp,esp
```

保存完寄存器以后，使用xor对寄存器ebx进行清零操作。因为要存储 dir 这个字符串，需要占用四个字节的存储空间。

```asm
xor  ebx,ebx
push ebx
```

然后将字符存入对应的位置。

```asm
mov  byte ptr [ebp-04h],64h
mov  byte ptr [ebp-03h],69h
mov  byte ptr [ebp-02h],72h
```

最后使用lea获取字符串起始地址，入栈，调用函数即可。

```asm
lea  ebx, [ebp-04h]
push ebx
mov ebx,0x74323b10
call ebx
```



如果字符串过长一个字节一个字节的复制过于复杂，可以使用将字符串改写为十六进制然后存入寄存器，再复制到栈中的方式（内存中）。此处的 dir，转换成16进制就是64 69 72，windows操作系统内存中存储为小端序，故转换后为0x00726964（高位补0）。

```asm
mov  ebx,0x00726964
mov  dword ptr[ebp-04h],ebx
```



#### shellcode生成方法

---

为了提取对应shellcode的机器码，在vs2019下进入调试状态，打开反汇编窗口，即可获取反汇编代码，但是采用这种方法获取的shellcode却无法执行成功，<font color = "red">原因暂时还不清</font>。[代码在此 sys_inline_shellcode2.c](https://github.com/jackcily/windows_shellocde_study/raw/master/file/sys_inline_shellcode2.c)



![get_shellcode](https://github.com/jackcily/windows_shellocde_study/raw/master/file/get_shellcode.JPG)



#### 独立shellcode编写

---

为了能编写出独立运行的shellcode，必须实现的就是获取函数地址 , <font color="red">需要一段代码能够自己定位任意函数地址</font>。

要调用一个函数，首先要在内存中载入动态链接库，那么必须知道LoadLibrary()函数地址。

获取一个函数的地址必须使用GetProcAddress()函数，而这个函数存在于“kernel32.dll”中，这个dll是默认一定会加载到内存中的。

那么实现一段代码能够自己定位任意函数地址的代码步骤如下:

- 找到kernel32.dll在内存中的位置
- 找到kernel32.dll的导出表
- 找到kernel32.dll的导出表中的GetProcAddress函数地址
- 使用GetProcAddress查找LoadLibrary函数的地址
- 使用LoadLibrary来加载需要函数所在的动态链接库
- 使用GetProcAddress获取需要函数地址并调用
- 查找ExitProcess函数的地址并调用



**kenel32的定位方式不止一种，可以使用PEB结构定位kernel的地址。**

这种方式的核心思想是在 PEB映射的模块中，kernel32.dll 通常是在 InInitializationOrderModuleList 中的第二个。

此处实地考证一下，在vs中打印出当前工程所执行进程所加载的所有模块 ,[代码在此 all_module.cpp]()。

发现dll的确是其中链接的第2个模块，打印结果如下：

![show_dll](D:\Administrator\desktop_bak\Desktop\windows_shellocde_study\file\show_dll.JPG)



**为了获得当前PEB的起始地址，可以通过FS段寄存器。**（32位系统，64位中使用GS）

因为OS完成加载后，FS段寄存器指向当前的TEB结构，通过TEB结构的偏移0x30 (ProcessEnvironmentBlock 当前进程的PEB指针) 处获得PEB的起始地址。

```asm
mov eax,fs:[0x30]
mov PEB,eax
```

获取当前线程所在进程的PEB结构体指针以后，可以通过指针指向获取到kernel32.dll的dllbase，具体分析可以参照[Windows平台shellcode开发入门（二）--- 进程环境块（PEB）](https://www.freebuf.com/articles/system/94774.html)

八行汇编就可以搞定这个过程。

```asm
xor ecx, ecx
mov eax, fs:[ecx + 0x30]; EAX = PEB
mov eax, [eax + 0xc]; EAX = PEB->Ldr 指针跳转从InMemoryOrderModuleList->InMemoryOrderLinks1
mov esi, [eax + 0x14]; ESI = PEB->Ldr.InMemOrder  dll双端链表head 
lodsd; EAX = Second module  指针跳转从InMemoryOrderLinks1->InMemoryOrderLinks2
xchg eax, esi; EAX = ESI, ESI = EAX;此时esi中是第一个模块的起点list
lodsd; EAX = Third(kernel32)  指针跳转从InMemoryOrderLinks2->InMemoryOrderLinks3
mov ebx, [eax + 0x10]; EBX = Base address kernel32.dll
```

**获取PEB起始地址以后，通过解析PE文件结构定位函数入口地址**。详细分析见[Windows平台shellcode开发入门（二）---PE文件格式](https://www.freebuf.com/articles/system/94774.html)

综上可以写出一个自定位函数地址的shellcode，此处是[system函数自定位示例代码 funcAddr.cpp]()。



#### shellcode实验

------

拟复现的shellcode链接在此[Windows/x86 - URLDownloadToFileA](https://www.exploit-db.com/shellcodes/40094)

#####　实验环境

windows7_x86 / vs2017

ubuntu18.04_server / apache2

拟在windws7中下载ubuntu18.04中的软件，但是shellcode执行失败，程序暂时还未调通。

于是决定自己实现一下下载执行器的功能。

首先假设已经获取URLDownloadToFileA函数地址，使用C语言内联函数[调用代码在此urldownload_inline.cpp]()

结合shellcode函数自定位实现下载功能[downloader_inline.cpp]()

再次修改实现下载执行功能[downloader_and_exec_inline.cpp]()

接下来需要将内联汇编代码转换为二进制代码,代码编写参考此处[从汇编代码提取Shellcode的简单实现](http://www.myhack58.com/Article/html/3/68/2011/29739.htm)

[具体代码在此 asm_2_hex.cpp]()

生成完成后在vs2019 x86下执行代码[在此exec_hex.cpp]()。





#### 参考资料

- [Windows x64 Shellcode](http://mcdermottcybersecurity.com/articles/windows-x64-shellcode#the-code)

- [PEB和TEB](https://www.cnblogs.com/hanfenglun/archive/2009/03/20/1417506.html)

- [PEB TEB结构体使用](https://blog.csdn.net/chriz_w/article/details/52096552)

- [段寄存器和8种地址寻址方式](https://blog.csdn.net/judyge/article/details/52337096)

- [枚举进程中的模块](https://blog.csdn.net/lanuage/article/details/72331277)

- [Windows下Shellcode编写详解](https://xz.aliyun.com/t/2108)

- [Windows平台shellcode开发入门](https://www.freebuf.com/articles/system/94774.html)

- [C++下载指定Url网络地址上的文件-Windows Api](https://blog.csdn.net/HW140701/article/details/78207490)

- [从汇编代码提取Shellcode的简单实现](http://www.myhack58.com/Article/html/3/68/2011/29739.htm)

  

#### 问题

- 已经获取了汇编asm，如何转换为可执行的exe文件？（怎么执行shellcode）

  可以使用vs中的功能直接将整段函数代码直接写到文件中。

  具体步骤为首先规定函数编译顺序，然后将指定位置中的函数写入字符串中。

- vs2019中直接将shellcode存储成字符串无法执行？

  因为字符串默认只有可读权限，需要修改字符串对应地址为可读可执行权限。具体示例如下：

  ```c++
  unsigned char shellcode[] = "\x55\x8B\xEC\x81\xEC\xC0\x00\x00\x00\x53\x56\x57\x8D\xBD\x40\xFF\xFF\xFF\xB9\x30\x00\x00\x00\xB8\xCC\xCC\xCC\xCC\xF3\xAB\x55\x8B\xEC\x81\xEC\xC0\x00\x00\x00\x55\x8B\xEC\x83\xEC\x04\x33\xDB\xBB\x64\x69\x72\x00\x89\x5D\xFC\x8D\x5D\xFC\x53\xBB\x10\x3B\x32\x74\xFF\xD3\x83\xC4\x04\x5B\x5D\x5F\x5E\x5B\x81\xC4\xC0\x00\x00\x00\x3B\xEC\xE8\xB3\xFA\xFF\xFF\x8B\xE5\x5D\xC3";
     
  DWORD lp;
  VirtualProtect(shellcode, sizeof(shellcode), PAGE_EXECUTE_READWRITE, &lp);
  ```

  也可以参考[C语言执行shellcode的五种方法](https://blog.csdn.net/liujiayu2/article/details/78294752)

- 32位寄存器的寻址方式怎么计算？

  首先需要区分基址寄存器、变址寄存器、段寄存器
  
- 内联汇编代码如何转换为真正的十六进制编码shellcode？

  可以直接从内存中读取

  




#### 背景知识

- 一些汇编指令的解释

  ```asm
  lodsw 等价于 mov eax,[esi] , add esi 4h
  lea  esi, [ebp - 04h] 等价于 mov esi,ebp - 04h
  ```

  



#TODO

探索shellcode检测原理，https://github.com/PyAna/PyAna
