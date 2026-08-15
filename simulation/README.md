# SimDataPlayer —— 录制 CSV 模拟播放器

用录制好的 CSV 数据文件，在其他设备上以与真实设备完全相同的帧率产生
`scandata` 全局量（amp/tof/beamValid/si/beam、robot_*、robot_ipoc、
longmen、m_start），用于在没有真实机器人和超声硬件时模拟扫描运行。

## 数据格式（本机录制 CSV）

```
X,Y,Z,A,B,C,SI,AMP_1,TOF_1,AMP_2,TOF_2,...,AMP_49,TOF_49,BEAM,LX,LY
```

- 每行 = 一帧，录制时按机器人 IPOC 变化写入（约 250 行/秒）
- `BEAM/LX/LY` 只有首行有值
- CSV 中不含 IPOC，由播放器按帧合成（每帧 +4，模拟 1kHz 硬件计数器）

## 帧率还原

- **机器人 250Hz**：每行回放间隔默认 4ms，`robot_ipoc` 每帧 +4
- **超声 180Hz**：CSV 每行已携带与机器人同步的 AMP/TOF 快照（录制时超声源
  约 180Hz，其变化节奏已内嵌在 250Hz 行数据中），逐行回放即完全还原录制序列，
  无需单独 180Hz 定时器

## 编译（任意装有 Qt 的机器）

```bat
cd simulation
qmake SimDataPlayer.pro
jom   （或 nmake）
```

## 运行

```bat
SimDataPlayer.exe "C:/Users/23714/Downloads/scan_20260717_141034.csv" "C:/Users/23714/Downloads/scan_20260717_133933.csv"
```

不传参数默认使用上面两个 Downloads 文件。多个文件按顺序连续回放。
每秒打印一次 `[Sim]` 状态，结束后自动退出。

## 集成进 SoundScan（在另一台设备上驱动整条渲染链路）

1. 把 `SimDataPlayer.h/cpp` 加入 `SoundScan.pro`（SOURCES/HEADERS）。
2. 需要时构造播放器并加载录制文件。
3. 在“扫描开始”（或模拟模式开关）处调用 `player->start()`；
   `m_start` 会自动置 true，Scan 的 1ms IPOC 轮询会读到
   `robot_ipoc` 变化并驱动在线网格 → 点云 → 渲染。
4. 真实输入（udpserver / ViewModel）与模拟器二选一，不要同时写同一组全局量。

## 注意

- 独立构建时 `sim_main.cpp` 定义 scandata 全局量；集成进 SoundScan 时由
  `3DScan/scandata.cpp` 提供定义，播放器只使用 extern 声明。
- `beamValid` 由播放器根据 `amp/tof` 非零推导；如需更精确的板外波束判定，
  可在录制端输出该列后扩展解析。
