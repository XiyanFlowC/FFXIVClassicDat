# FFXIVClassicDat

这是一个解析 FFXIV 客户端数据文件的项目。目的在于探究是否能够恢复旧FF14中的中文数据，使之能够显示。或替换日文文本为中文文本。

目前，仅可以粗略扫描客户端文件，寻找明显的记录。

目前，不能确定字库文件，请指定黑体等系统字体。

- [x] SSD (数据表) 导入/导出
  - [x] 游戏控制符（0x02开始0x03结束的序列）解析
- [x] SQWT 解密
- [ ] GTEX <-> DDS
- [ ] SEDB 解析/抽出/插入

## 使用
### FFXIVClassicDatConsole
目前通过本程序进行作业。

初始化时，使用-I来指定安装目录。-L来指定对象语言。

※这些设定会被保存，因此不需要每次执行时都调用。

※安装目录的尾部请不要添加斜杠（/）或反斜杠（\）。

※-L指令可接受ja en de fr chs cht，或语言数字代码（0-5）。

※游戏数据中不包括任何cht文本。

例：
```bash
FFXIVClassicDatConsole -I "D:\FFXIV Classic" -L chs
```

使用-S来扫描data文件夹。
```bash
FFXIVClassicDatConsole -S
```

使用-e指令可以导出全部的SSD（在此之前必须首先完成-S）

使用-i指令可以导入全部的SSD（在此之前必须首先完成-S）

-e或-i指令后可以跟随一个参数，该参数指定SSD的文件ID（如```-e 0x01030000```导出游戏的UI数据表，```-i 0x27950000```导入启动器的UI数据表）


