"""Original procedural short game cues; standard-library only, no samples/services.
Run from anywhere. Recreates Content/MVP/AudioSource/*.wav. PCM16 mono 22050Hz.
"""
from pathlib import Path
import math, random, wave, struct, hashlib, json
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Content/MVP/AudioSource';OUT.mkdir(parents=True,exist_ok=True)
RATE=22050
SPECS={'palm':(.24,160,55,.32),'sword':(.18,1100,210,.22),'thunder':(.50,95,34,.38),'hit':(.12,220,85,.24),'pickup':(.17,750,1450,.08),'dash':(.22,480,150,.22),'upgrade':(.75,440,880,.02),'boss':(1.0,100,65,.12),'win':(1.3,330,660,.01),'loss':(.9,330,110,.03)}
records=[]
for ordinal,(name,(duration,start,end,noise)) in enumerate(SPECS.items()):
 rng=random.Random(700+ordinal); count=int(RATE*duration); samples=[];phase=0.;low=0.
 for i in range(count):
  t=i/RATE;u=i/max(1,count-1);freq=start+(end-start)*u
  phase+=2*math.pi*freq/RATE
  attack=min(1.,t/.008); env=attack*(1-u)**2
  low=.82*low+.18*rng.uniform(-1,1)
  tone=.42*math.sin(phase)+.13*math.sin(phase*1.5)
  if name in ('upgrade','win'):
   note=int(u*4);mult=(1.,1.25,1.5,2.)[min(3,note)]
   tone=.30*math.sin(2*math.pi*start*mult*t)+.10*math.sin(2*math.pi*start*mult*2*t)
  x=env*(tone*(1-noise)+low*noise*2)
  samples.append(max(-.7,min(.7,x)))
 # Smooth to exact silence; no clicks at end and no DC bias beyond tiny roundoff.
 mean=sum(samples)/len(samples);samples=[x-mean*math.sin(math.pi*i/max(1,count-1))**2 for i,x in enumerate(samples)]
 payload=b''.join(struct.pack('<h',round(max(-.8,min(.8,x))*32767)) for x in samples)
 path=OUT/(name+'.wav')
 with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(payload)
 records.append({'file':str(path.relative_to(ROOT)),'duration':duration,'peak':max(abs(x) for x in samples),'rms':math.sqrt(sum(x*x for x in samples)/len(samples)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
(ROOT/'Art/Audio/manifest.json').write_text(json.dumps({'origin':'original mathematical synthesis; no third-party audio','format':'PCM16 mono 22050 Hz','sounds':records},indent=2)+'\n')
print('Generated',len(records),'original WAV cues')
