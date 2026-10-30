"""Layout-only proof sheets of actual Blender renders; no painted/replaced pixels."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
P=Path(__file__).resolve().parent
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',19)
small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',15)
def label(im,title):
 im=im.convert('RGB');d=ImageDraw.Draw(im);d.rectangle((0,0,im.width,36),fill=(12,18,25));d.text((14,7),title,font=font,fill=(239,226,203));d.rectangle((0,im.height-28,im.width,im.height),fill=(12,18,25));d.text((14,im.height-24),'BLENDER OFFLINE / NOT UE RUNTIME',font=small,fill=(225,190,133));return im
for phase in ['before','after']:
 raw=Image.open(P/(phase+'-raw.png'));label(raw.copy(),phase.upper()+' | same camera / staged mesh preview').save(P/(phase+'.png'))
 # 60% downsample then central crop: preserves mesh scale; does not enlarge the hero.
 crop=raw.resize((960,660),Image.Resampling.LANCZOS).crop((0,60,960,600));label(crop,phase.upper()+' | 960 x 540 center crop / not a gameplay capture').save(P/(phase+'-960x540.png'))
board=Image.new('RGB',(1600,620),(12,18,25))
for i,phase in enumerate(['before','after']):
 raw=Image.open(P/(phase+'-raw.png')).resize((800,550),Image.Resampling.LANCZOS);board.paste(label(raw,phase.upper()+' | identical camera'),(i*800,0))
d=ImageDraw.Draw(board);d.text((16,562),'NPR study: original meshes + staging  /  refined meshes + cool key / warm rim / restrained depth unity',font=small,fill=(220,221,218));d.text((16,588),'No geometry, camera or actor-scale changes were made by the lighting/compositing script.',font=small,fill=(162,174,188));board.save(P/'before-after.png')
