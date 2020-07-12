# Description: 
Example showing the output waveform

# Controls:
| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 |  Frequency | Frequency 10Hz - 12kHz |
| Knob 2 |  Clipping | Clipping threshold 0% - 100% |
| Encoder |  Waveform Select | sine, triangle, band-limited sawtooth, band-limited square |

# Code Snippet  
```cpp 
// Fill buffer and transmit if full 
if (waveformBuffer.Process(sig)) {
    hw.seed.usb_handle.TransmitInternal(waveformBuffer.buffer(), waveformBuffer.bufferSize());
}
```

# Usage:
Connect Pod to host running Python 3 on internal USB port, then run waveform.py. 

# Waveform Visualizer
<img src="https://raw.githubusercontent.com/jcolicchio/DaisyExamples/waveform/pod/Waveform/resources/clipped_saw.png" style="width: 100%;"/>

# Known Bugs:
There's an issue on OSX Catalina with pygame, but installing pygame version 2.0.0.dev4 should work just fine