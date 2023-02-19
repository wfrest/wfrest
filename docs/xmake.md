# xmake compiling

- compile wfrest library

```
xmake
```

- compile && run test

```
xmake -g test

xmake run -g test
```

- compile && run example

```
xmake -g example

xmake run -g example
```

-  compile && run benchmark

```
xmake -g benchmark

xmake run -g benchmark
```

## running

`xmake run -h` can see which targets you can run

Select a target to run, for instance:

```
xmake run 21_mysql
```

## xmake install

```
sudo xmake install
```

## Compile static / shared library

```
// compile static lib
xmake f -k static
xmake -r
```

```
// compile shard lib
xmake f -k shared
xmake -r
```

`tips : -r means -rebuild`

## build options

`xmake f --help` can see our defined options.

```
Command options (Project Configuration):

        --wfrest_inc=WFREST_INC                              wfrest inc (default: /Users/chanchan/Documents/pro/wfrest/_include)
        --wfrest_lib=WFREST_LIB                              wfrest lib (default: /Users/chanchan/Documents/pro/wfrest/_lib)
        --memcheck=[y|n]                                     valgrind memcheck
```

You can cut or integrate each components with the following commands

```
xmake f --memcheck=y
xmake -r
```
