#!/bin/bash
# role-response matrix: mean |pixel change| (0-255) of each role against the resting frame
cd /home/claude/DaliVisual/Tools/ShaderHarness
roles="rest kick snare hat bassline sub mid high"
for f in ../../Shaders/scenes/scene_*.frag; do
  n=$(basename $f .frag); [ "$n" = "scene_17_image_reactor" ] && continue
  for r in $roles; do
    rm -rf /tmp/rm_$r && mkdir -p /tmp/rm_$r
    DALI_ROLE=$r DALI_ONLY=$n DALI_W=320 DALI_H=180 DALI_FRAMES=2 DALI_T=6 DALI_STATE=peak /tmp/sh ../../Shaders /tmp/rm_$r >/dev/null 2>&1
  done
  python3 - "$n" <<'PY'
import sys,glob
import numpy as np
from PIL import Image
n=sys.argv[1]
def load(r): return np.asarray(Image.open(glob.glob(f'/tmp/rm_{r}/{n}*.ppm')[0]).convert('RGB'),dtype=np.float32)
base=load('rest')
vals=[np.abs(load(r)-base).mean() for r in ['kick','snare','hat','bassline','sub','mid','high']]
print(f"{n[6:]:28s}"+"".join(f"{v:7.1f}" for v in vals))
PY
done
