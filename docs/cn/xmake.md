# xmake 编译

- 编译 wfrest 库

```
xmake
```

- 编译并运行测试

```
xmake -g test

xmake run -g test
```

- 编译并运行示例

```
xmake -g example

xmake run -g example
```

- 编译并运行基准测试

```
xmake -g benchmark

xmake run -g benchmark
```

## 运行

`xmake run -h` 可以查看您可以运行的目标

选择一个目标运行，例如：

```
xmake run 21_mysql
```

## xmake 安装

```
sudo xmake install
```

## 编译静态/共享库

```
// 编译静态库
xmake f -k static
xmake -r
```

```
// 编译共享库
xmake f -k shared
xmake -r
```

`提示：-r 表示 -rebuild（重新构建）`

## 构建选项

`xmake f --help` 可以查看我们定义的选项。

```
命令选项（项目配置）：

        --wfrest_inc=WFREST_INC                              wfrest inc (默认: /Users/chanchan/Documents/pro/wfrest/_include)
        --wfrest_lib=WFREST_LIB                              wfrest lib (默认: /Users/chanchan/Documents/pro/wfrest/_lib)
        --memcheck=[y|n]                                     valgrind 内存检查
```

您可以使用以下命令裁剪或集成各个组件

```
xmake f --memcheck=y
xmake -r
``` 