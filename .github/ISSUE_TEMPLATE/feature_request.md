---
name: Feature request
about: Suggest an improvement
title: ''
labels: ''
assignees: ''

---

1. Does the feature conflict with NRFLite's design goals shown below?

2. Describe the feature:

## NRFLite Design Goals
NRFLite's purpose is to make using the nRF24L01 simple using a relatively short and easy to understand code base. To achieve these goals, the overall speed of the library and available features within the radio are purposefully excluded. The "Lite" of NRFLite is short for "Lightweight", meaning it is a small library with a limited amount of complexity. If this library allowed users to control all aspects of the nRF24L01, they could put the radio into unexpected configurations and create more special cases that would need to be managed. So in order to limit the number of issues users can experience, NRFLite has a limited feature set.

Please note that if you need more control over the radio than NRFLite provides, the RF24 library is a great option. Unlike NRFLite which focuses on simplicity, RF24 focuses on speed and features. So it might be harder to get working but once you do, it will allow you to use the full capability of the nRF24L01. There is also the Mirf library which takes the opposite approach and is extremely basic.

Mirf: limited features, low-complexity, primary code file ~300 lines
- https://github.com/mberntsen/arduino/tree/master/libraries/Mirf

NRFLite: more features, medium-complexity, primary code file ~650 lines
- https://github.com/dparson55/NRFLite

RF24: all features, high-complexity, primary code file ~2000 lines
- https://github.com/nRF24/RF24
