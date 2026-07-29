<div align="center">

# 🍎 BAC_APPLE — Bacteria Simulation from Bad Apple Frames

[![Language](https://img.shields.io/badge/C%2B%2B-SDL2-blue?style=for-the-badge&logo=cplusplus)]()
[![Category](https://img.shields.io/badge/Category-Generative%20Art%20%2F%20Simulation-orange?style=for-the-badge)]()
[![License](https://img.shields.io/badge/License-Open-brightgreen?style=for-the-badge)](LICENSE.md)
[![Stars](https://img.shields.io/github/stars/Jirnyak/bac_apple?style=for-the-badge&color=gold)]()

> **Bacteria colonies grow and die according to the luminosity values extracted from "Bad Apple!!" video frames — generative life simulation driven by animation.**

[▶️ Build & Run](#getting-started) &nbsp;·&nbsp; [🐛 Issues](../../issues)

</div>

---

## 📖 About

**BAC_APPLE** reads frames from the iconic *Bad Apple!!* video, extracts per-pixel luminosity values, and uses them to seed and control a real-time bacteria colony simulation. Dark pixels spawn bacteria, bright pixels kill them — the entire animation drives the biology.

The result: a living, evolving colony that grows and collapses in perfect synchrony with the shadow-art animation.

---

## ✨ Features

| Feature | Description |
|---|---|
| 🎬 **Frame-Driven Biology** | Each video frame maps luminosity → bacteria spawn/death probability |
| 🦠 **Colony Dynamics** | Bacteria reproduce, compete for resources, and die based on environmental conditions |
| ⚡ **Real-Time C++** | High-performance simulation loop — thousands of agents updated per frame |
| 🎨 **Generative Visuals** | The animation and the bacteria colony are the same thing |

---

## 🔨 Getting Started

```bash
git clone https://github.com/Jirnyak/bac_apple.git
cd bac_apple
make
./bac_apple
```

---

## 📜 License

**Open License** — Jirnyak. See [LICENSE.md](LICENSE.md).

---

<details>
<summary>🇷🇺 Русская Версия</summary>

**BAC_APPLE** — симуляция бактериальных колоний, управляемых кадрами видео *Bad Apple!!*. Яркость пикселей определяет вероятность рождения и гибели бактерий. Тёмные пиксели — жизнь, светлые — смерть. Анимация и биология — одно целое.

</details>
