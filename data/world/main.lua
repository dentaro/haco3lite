function get_map_sprn(sprx,spry)
  local celx=flr(sprx/8)
  local cely=flr(spry/8)
  local celc=mget(celx,cely)
  return celc
end

x=80
y=64
tx=0
ty=0
ax=171--lcx(171)
ay=218--lcy(218)
wx=164--0
wy=210--36
px=0
py=0
d=1 -- 方向を示す  

function _init()--1回だけ

end
function _update()

--functionの後に一行開けると安定する？パーサーのバグ？
  local prex=x--これがあるとなぜがうまく動く
  local prey=y--これがあるとなぜがうまく動く
  if (btn(1)>=1) then
  wx=wx-1
  d=1
  end
  if (btn(2)>=1) then
  wx=wx+1
  d=2
  end
  if (btn(3)>=1) then
  wy=wy-1
  d=3
  end
  if (btn(4)>=1) then
  wy=wy+1
  d=4
  end
  wx=wx%256
  wy=wy%256
  fset(0,0,1)--水スプライト45を通れなく（0ビットを1に）する
end
function _draw()

  if wx~=px or wy~=py then
    rmap("/init/param/map/w2.bin",wx,wy)
  end
  spr8(d+64,(ax-wx)*8, (ay-wy)*8, 1,1,0, 0,11)--村人スプライト透明色指定のあとに色番号をつけるとふちどり
  mset(100, (ax-wx)*8, (ay-wy)*8)
  spr8(d+64,x,y,1,1,0, 0,10)--主人公スプライト透明色指定のあとに色番号をつけるとふちどり
  if d==9  then 
    tx=x - 8
    ty=y
  elseif d==10 then 
    tx=x + 8 
    ty=y
  elseif d==11 then 
    tx=x
    ty=y - 8 
  elseif d==12 then 
    tx=x
    ty=y + 8 
  end
  px=wx
  py=wy

  print(get_map_sprn(tx+x,ty+y),120,96)--スプライト値を表示
  print(wx,120,104)--ワールド座標を表示
  print(wy,120,112)--ワールド座標を表示
end