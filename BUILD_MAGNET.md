# The Magnet Toy — 编译说明

## 仓库

```
https://github.com/T307-coder/The-Magnet-Toy
```

## 分支

| 分支 | 用途 |
|------|------|
| `the-electric-magnet-toy` | **磁铁模组**（你要编译的） |
| `3d-tpt` | 3D 视口（另一个项目，与 magnet 无关） |

```bash
git clone https://github.com/T307-coder/The-Magnet-Toy.git
cd The-Magnet-Toy
git checkout the-electric-magnet-toy
```

## 环境要求

- **Windows**（当前只支持 Windows 构建）
- **Visual Studio 2022**（含 C++ 桌面开发工作负载）
- **Python 3**（用于 meson）
- **meson + ninja**（通过 pip 安装）

```powershell
pip install meson ninja
```

## 编译

### Debug 版本（开发用）

```powershell
meson setup build-debug --buildtype=debug
ninja -C build-debug
```

输出：`build-debug\powder.exe`

### Release 静态版本（分发用，无 DLL 依赖）

```powershell
meson setup build-static -Dstatic=prebuilt --buildtype=release
ninja -C build-static
```

输出：`build-static\powder.exe`（~7 MB，单文件可运行）

## 运行

```powershell
.\build-debug\powder.exe     # Debug
# 或
.\build-static\powder.exe    # Release 静态版
```

## 主要文件说明

| 文件 | 说明 |
|------|------|
| `src/simulation/MagnetismCommon.h` | 磁场计算核心 |
| `src/simulation/MagnetismCommon.cpp` | |
| `src/simulation/elements/BRMT.cpp` | BRMT 元素行为 |
| `src/simulation/elements/PROT.cpp` | PROt 元素行为 |
| `src/simulation/elements/ELEC.cpp` | ELEC 元素行为 |
| `src/simulation/Simulation.cpp` | create_part, 磁场 FFT 求解 |
| `src/simulation/Particle.h` | 粒子结构（含 z, vz 字段） |
| `src/SimulationConfig.h` | 窗口尺寸、CELL 等常量 |
| `src/gui/game/GameController.cpp` | 主控制器 Tick() |
| `src/gui/game/QuickOptions.cpp` | 快捷选项菜单 |

## 注意事项

- **不要**合并 `3d-tpt` 分支的代码（那是 3D 视口项目，含 CubeTest.cpp 等）
- 磁铁模组使用 `XRES=384, YRES=384`（96×96 格，每格 4px）
- `NPART = XRES × YRES = 147456` 最大粒子数
- 依赖 FFTW3F 库（已在预编译依赖中）
- GitHub Actions CI 配置文件在 `.github/workflows/` 下
