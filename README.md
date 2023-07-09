# fix-wechat-shadow

## 声明

代码基于 https://12101111.github.io/block-wine-wechat-black-window/ ，修复了原代码不区分阴影窗口和其他窗口，一律屏蔽微信菜单、小程序的 bug。原理是屏蔽窗口前先判断尺寸，看是否过小或比例不当，怀疑是菜单的话则不屏蔽。

## 使用方法

### prepare libs
```
sudo apt install libxcb-util-dev libxcb-icccm4-dev
```

### 编译 & 运行

```
gcc wxshadow.c -lxcb -lxcb-util -lxcb-icccm -o xwechathide
./xwechathide
```

### 其他
建议加入开机启动，几乎不占用 CPU 资源
