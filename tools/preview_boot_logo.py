#!/usr/bin/env python3
"""Render the firmware's native 256x64 boot mark, with an optional integer zoom."""
import argparse,ctypes as C,struct,subprocess,tempfile,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def render(output,scale=1):
 with tempfile.TemporaryDirectory() as tmp:
  binary=Path(tmp)/'boot.so'
  sources=['components/graphics/streamoo_canvas.c','components/ui/streamoo_ui_boot.c']+['components/text/'+name+'.c' for name in ('streamoo_text','streamoo_text_layout','streamoo_font5x8','streamoo_bitmap_font','streamoo_ui_font')]
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',*['-Icomponents/'+name+'/include' for name in ('graphics','text','ui')],*sources,'-o',str(binary)],cwd=ROOT,check=True)
  lib=C.CDLL(str(binary));lib.streamoo_ui_boot_draw.argtypes=[C.c_void_p,C.c_size_t];lib.streamoo_ui_boot_draw.restype=C.c_bool
  guarded=(C.c_uint8*8194)(*([0xa5]*8194));frame=C.byref(guarded,1)
  assert not lib.streamoo_ui_boot_draw(frame,8191) and all(b==0xa5 for b in guarded)
  assert lib.streamoo_ui_boot_draw(frame,8192) and guarded[0]==guarded[-1]==0xa5
  pixels=bytes(guarded[1:-1]);rows=bytearray()
  for y in range(64):
   row=bytearray([0])
   for x in range(256):
    value=pixels[y*128+x//2];gray=(value&15 if x&1 else value>>4)*17
    row.extend([gray]*scale)
   rows.extend(row*scale)
 def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
 png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',256*scale,64*scale,8,0,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
 output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(png)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('output',type=Path);parser.add_argument('--scale',type=int,choices=range(1,9),default=1);args=parser.parse_args();render(args.output,args.scale)
