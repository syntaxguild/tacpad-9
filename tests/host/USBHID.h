#pragma once
bool testUsbReady = false;
struct USBHID { bool ready() { return testUsbReady; } };
