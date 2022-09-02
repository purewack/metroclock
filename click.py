import sounddevice as snd
import numpy as np
from matplotlib import pyplot as plt 
import time

srate = 44100.0
f1 = 3000.0
dur = 0.02
spls = int(srate*dur)
ii = np.arange(spls)
wave = np.cos(2*np.pi*ii*f1/srate)
slope = np.linspace(1,0,spls)
click = wave * slope

for x in range(4):
    snd.play(click)
    time.sleep(0.5)
    snd.stop()
