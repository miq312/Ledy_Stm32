# LED Animation Project

This project demonstrates a system for controlling LED animations and button interactions using STM32 microcontroller. It includes functionalities for switching between different LED animations and handling button presses to control animation behavior.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
  
## Overview

The project utilizes the STM32 HAL library to control a series of LEDs and handle button presses. The LEDs can display different animations, and the behavior can be modified using the buttons.

<p align="center">
  <img src="https://github.com/miq312/Ledy_Stm32/blob/main/img/zdj1.jpg">
</p>

## Features

- Multiple LED animations
  - Simple transition
  - Bounce animation
- Button handling for:
  - Changing animations
  - Switching between manual and automatic modes
  - Adjusting animation speed
- Easy integration and customization

## Hardware Requirements

- STM32 microcontroller (e.g., STM32F4 series)
- LEDs connected to GPIO pins
- Buttons connected to GPIO pins

<p align="center">
  <img src="https://github.com/miq312/Ledy_Stm32/blob/main/img/zdj3.png">
</p>
<p align="center">
  <img src="https://github.com/miq312/Ledy_Stm32/blob/main/img/zdj2.jpg">
</p>

## Software Requirements

- STM32CubeMX
- STM32CubeIDE or any ARM-compatible compiler
- HAL library for STM32
