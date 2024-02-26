function collision(x,y)
  return mapf(x,y) or mapf(x+7,y) or mapf(x,y+7) or mapf(x+7,y+7)
end
x=80
y=64
tx=0
ty=0
ax=171--lcx(171)
ay=200--lcy(218)
wx=156--0
wy=187--36
px=0
py=0
wstat=1
function _init()--1回だけ
end
function _update()
  if (btn(1)>=1) then
  wx=wx-1
  end
  if (btn(2)>=1) then
  wx=wx+1
  end
  if (btn(3)>=1) then
  wy=wy-1
  end
  if (btn(4)>=1) then
  wy=wy+1
  end
  wx=wx%256
  wy=wy%256
end
function _draw()
  if wx~=px or wy~=py then
    rmap("/init/param/map/w.bin",wx,wy)
    if collision(x,y) == true then
      wx=px
      wy=py
    end
    tone(1, 440, 32)
    for j = 1, 3 do
      for i = 1, 3 do
        xx=(160-wx)*8+i*16
        yy=(190-wy)*8+j*16
        spr8(64,xx, yy, 1,1,0, 0, 7)--村人スプライト透明色指定のあとに色番号をつけるとふちどり
        mset(100, xx, yy)
      end
    end
    spr8(64,x,y,1,1,0, 0,10)--主人公スプライト透明色指定のあとに色番号をつけるとふちどり
    px=wx
    py=wy
  else
    if sprn(tx+x,ty+y)==100 then
      HPMP=1
      win(wstat, 0,   0, 1, 2, 3, HPMP,"ああああ")
      win(wstat, 40,  0, 1, 2, 3, HPMP,"いいいい")
      win(wstat, 80,  0, 1, 2, 3, HPMP,"うううう")
      win(wstat, 120, 0, 1, 2, 3, HPMP,"ええええ")
      -- win(wstat, 0, 0,2, 3, 4,0,"どうする？")
      -- win(wstat,88, 0,1, 8, 7,0,"どうぐ")
      rectfill(56, 32, 48, 90, 0, "/world/png/enemy/3.png") --160,48が敵エリアの最大値
      -- rectfill(0, 32, 160, 96, 8);
      win(wstat, 0,80,1, 4,18, 0,"むらびと")

    end
  end
  -- print(wx,120,104)--ワールド座標を表示
  -- print(wy,120,112)--ワールド座標を表示
end