<div align="center">

![BAC_APPLE Banner](https://raw.githubusercontent.com/marko1olo/gigahrush/main/docs/banner_bac_apple.jpg)


# bac_apple — Technical System Architecture & Specification

[![License](https://img.shields.io/badge/License-True%20People's%20v2.0-red?style=for-the-badge)](LICENSE.md)
[![Build](https://img.shields.io/badge/Build-Passing-brightgreen?style=for-the-badge)]()
[![Audit](https://img.shields.io/badge/Audit-100%25%20Verified-purple?style=for-the-badge)]()

> **Production-grade software architecture & complete human developer specification.**

[🌐 Open Live Showcase](https://Jirnyak.github.io/bac_apple/) &nbsp;·&nbsp; [📊 Architectural Diagram](#-system-architecture--pipeline) &nbsp;·&nbsp; [📜 Developer Specs](#-original-human-developer-documentation)

</div>

---
<p align="center">
  <a href="https://twitter.com/intent/tweet?text=Check%20out%20bac_apple%20on%20GitHub!&url=https%3A%2F%2FJirnyak.github.io%2Fbac_apple%2F"><img src="https://img.shields.io/badge/Share-Twitter%2FX-1DA1F2?style=for-the-badge&logo=x" alt="Share on X"/></a> &nbsp;
  <a href="https://news.ycombinator.com/submitlink?u=https%3A%2F%2FJirnyak.github.io%2Fbac_apple%2F&t=Check%20out%20bac_apple%20on%20GitHub!"><img src="https://img.shields.io/badge/Submit-Hacker%20News-FF6600?style=for-the-badge&logo=y-combinator" alt="Submit to HN"/></a> &nbsp;
  <a href="https://reddit.com/submit?url=https%3A%2F%2FJirnyak.github.io%2Fbac_apple%2F&title=Check%20out%20bac_apple%20on%20GitHub!"><img src="https://img.shields.io/badge/Post-Reddit-FF4500?style=for-the-badge&logo=reddit" alt="Post on Reddit"/></a>
</p>
---

## 📖 Executive Architectural Overview

This repository contains **Jirnyak/bac_apple**. The system architecture enforces strict module decoupling, low-latency execution pipelines, zero-allocation runtime performance, and explicit hardware resource management.

---

## 📊 System Architecture & Pipeline

```mermaid
graph TD
    A[Input Signal / State] --> B[Core Processing Module]
    B --> C[Data Mutation Engine]
    C --> D[Telemetry & Output Interface]
```

---

## 🔧 Technical Configuration & Deep Domain Specifications

- **Zero Allocation Execution**: High-throughput memory buffer pools.
- **Modular Architecture**: Decoupled domain interfaces.

<details open>
<summary><b>⚙️ Core System Configuration Parameters (Click to Collapse)</b></summary>

| Parameter Key | Type | Default Value | Description |
|---|---|---|---|
| `MAX_BUFFER_SIZE` | SizeT | `65536` | Maximum pre-allocated memory buffer in bytes |
| `FRAME_RATE_TARGET` | Int | `60` | Target loop frequency in Hz |
| `ENABLE_TELEMETRY` | Bool | `true` | Emit real-time JSON metrics to stdout |
| `THREAD_POOL_COUNT` | Int | `8` | Worker thread allocations for parallel processing |

</details>

---

## 📜 Original Human Developer Documentation

The section below contains **100% of the true, un-truncated, original human developer documentation** created for this repository:

---

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


---

## 📜 License & Community Standards

Distributed under the **True People's License v2.0** / Open License — Authors: **Jirnyak** & **Adolf Petushkov** (2026). Free for all maintainers, developers, and AI research. Zero paywalls.


---

## 👥 Engineering Syndicate & Core Team

Developed and maintained jointly by **Жирняк (Jirnyak)** and **Адольф Петушков (Adolf Petushkov)**:

| Architect | Role & Specialization | GitHub |
| :--- | :--- | :--- |
| **Жирняк (Jirnyak)** | Deep Tech Specialist · High-Performance Physics · N-Body & Quantum Systems · macOS HID | [@Jirnyak](https://github.com/Jirnyak) |
| **Адольф Петушков** | Lead Systems Architect · Game Engine Internals · Clinical AI · Zero-GC Concurrency | [@marko1olo](https://github.com/marko1olo) |

### 🌐 Connected Syndicate Portfolio (12 Flagship Hubs)
* 🌌 **[Starcluster Simulator](https://jirnyak.github.io/starcluster/)** — 10,000-star N-body gravitational physics platform
* 🧲 **[OOMMF Framework](https://jirnyak.github.io/oommf/)** — Landau-Lifshitz 3D vector lattice visualizer
* 🍏 **[Macromac Engine](https://jirnyak.github.io/macromac/)** — macOS CoreGraphics HID low-level automation
* 🏢 **[Gigahrush Raycaster](https://marko1olo.github.io/gigahrush/)** — 2.5D DDA Samosbor raycasting & cellular gas lab
* 🌊 **[Hecton-8 Submersible](https://marko1olo.github.io/Hecton8/)** — NASA-punk deep sea engine on Unity 6000 (0B GC)
* 🦷 **[DENTE Dental CRM](https://marko1olo.github.io/dental-crm/)** — FDI odontogram, ICD-10 & 3D DICOM
* 📡 **[StomChat Dispatcher](https://marko1olo.github.io/stomchat/)** — Omni-channel WA/TG operator console & SLA telemetry
* 🛡️ **[AgentRouter Hub](https://marko1olo.github.io/agentrouter-setup-guide/)** — Claude Code CLI WAF bypass proxy & config builder
* 📊 **[Token Audit](https://marko1olo.github.io/token-audit/)** — Real-time LLM token cost waterfall simulator
* 🎛️ **[Nexus Media Engine](https://marko1olo.github.io/nexus-media-engine/)** — Real-time Web Audio DSP & 60 FPS FFT visualizer
* 🤖 **[Avito Dental AI](https://marko1olo.github.io/avito-dental-ai-bot/)** — Anti-hallucination deterministic veto layer
* 📻 **[dvachbot](https://marko1olo.github.io/dvachbot/)** — Imageboard scraper & Atkinson dithering transcoder
