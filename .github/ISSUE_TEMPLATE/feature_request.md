---
name: Feature request
about: Suggest an improvement
title: ''
labels: ''
assignees: ''

---

Does the improvement align with NRFLite's design goal? (see design goal below)

Describe the feature:

## NRFLite Design Goal
NRFLite's purpose is to make using the nRF24L01 as easy as possible while exposing a few common features, but the downside to this approach is the lack of control users are given and not all features are available. The "Lite" of NRFLite is short for "Lightweight", meaning it is a small library with a limited amount of complexity. If this library allowed users to control more things, they could put the nRF24L01 into unknown configurations and create more special cases that would need to be managed. So in order to limit the number of issues users can experience, NRFLite has a limited feature set.

Please note that if you need more control over the radio than NRFLite provides, the RF24 library is a great option. Unlike NRFLite which focuses on simplicity, RF24 focuses on speed and features. So it might be harder to get working but once you do, it will allow you to use the full capability of the nRF24L01. There is also the Mirf library which takes the opposite approach and is extremely small and simple, and it could be used as a starting point for your own personalized library.

Mirf: limited features, low-complexity
- https://github.com/mberntsen/arduino/tree/master/libraries/Mirf

NRFLite: more features, medium-complexity
- https://github.com/dparson55/NRFLite

RF24: all features, high-complexity
- https://github.com/nRF24/RF24
