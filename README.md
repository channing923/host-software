# 鸡你太美

<p align="center">
  <img src="plotjuggler.png" width="180" alt="鸡你太美图标">
</p>

“鸡你太美”是一款基于 Qt 5 的多通道测量、设备配置和时序数据分析平台。
本项目基于 [PlotJuggler 3.17](https://github.com/PlotJuggler/PlotJuggler) 进行定制开发。

## 主要功能

- 加载 CSV、ULog、MCAP 等数据文件。
- 接收 WebSocket、UDP、MQTT、ZeroMQ 等实时数据流。
- 支持多窗口、多曲线和大规模时序数据展示。
- 提供缩放、测量、统计和曲线变换功能。
- 支持 Lua 与 Python 自定义函数。
- 支持通过插件扩展数据源、解析器和工具箱。
- 提供多通道测量与设备配置界面。

![软件界面](docs/plotjuggler3.gif)

## Linux 编译

安装 Qt 5、CMake 和编译工具后，在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build --target plotjuggler -j2
```

启动程序：

```bash
./build/bin/plotjuggler
```

更完整的依赖和编译说明见 [COMPILE.md](COMPILE.md)。

## 注册为 Linux 桌面应用

项目提供了 [jinitaimei.desktop](jinitaimei.desktop)。在项目根目录执行：

```bash
desktop-file-install \
  --dir=~/.local/share/applications \
  --set-key=Exec \
  --set-value="$(pwd)/build/bin/plotjuggler" \
  --set-icon="$(pwd)/plotjuggler.png" \
  --rebuild-mime-info-cache \
  jinitaimei.desktop
```

安装后可从应用菜单搜索“鸡你太美”，也可以执行：

```bash
gtk-launch jinitaimei
```

## Windows 版本

Linux 构建产物不能直接在 Windows 使用。Windows 版本需要使用 MSVC、Windows 版 Qt
和对应的运行库重新编译。

仓库包含 Windows 自动构建配置：

```text
.github/workflows/windows.yaml
```

如果主仓库托管在 Gitee，可以继续使用 Gitee 管理代码，同时将代码推送到一个 GitHub
镜像仓库，由 GitHub Actions 生成 Windows x64 安装包：

```bash
git remote add github https://github.com/你的用户名/你的仓库.git
git push github main
```

随后在 GitHub 仓库的 **Actions → windows → Run workflow** 中启动构建并下载制品。

也可以在 Windows 本机按照 [COMPILE.md](COMPILE.md) 的说明使用 Conan 或 vcpkg 编译。

## 输出文件

| 平台 | 文件或目录 |
| --- | --- |
| Linux 主程序 | `build/bin/plotjuggler` |
| Linux 桌面入口 | `jinitaimei.desktop` |
| 通用 PNG 图标 | `plotjuggler.png` |
| Windows 图标 | `plotjuggler.ico` |
| Windows 主程序 | `build/bin/Release/plotjuggler.exe` |

程序内部可执行文件名仍保留为 `plotjuggler`，用户可见的软件名称、窗口标题、安装程序和
桌面快捷方式均显示为“鸡你太美”。

## 开源来源与许可证

本项目是在 PlotJuggler 基础上的定制版本。PlotJuggler 由 Davide Faconti 及其社区贡献者开发：

- 上游项目：https://github.com/PlotJuggler/PlotJuggler
- 上游网站：https://www.plotjuggler.io

项目遵循 [Mozilla Public License 2.0](LICENSE.md)。部分第三方依赖（包括 Qt）使用
GNU Lesser General Public License。分发软件时请同时遵守相关第三方许可证。
