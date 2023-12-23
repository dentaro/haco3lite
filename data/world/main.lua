cl = 4
-- sound=-1
function get_map_sprn(sprx,spry)
  local celx=flr(sprx/8)
  local cely=flr(spry/8)
  local celc=mget(celx,cely)
  return celc
end

function get_map_flag(sprx,spry)
  local celx=flr(sprx/8)
  local cely=flr(spry/8)
  local celc=mget(celx,cely)
  -- return celc
  return fget(celc,0)
end

function collision(x,y)
  -- if get_map_sprn(x,y) == 0 then 
  --   return true
  -- end

  -- return get_map_flag(x-cl,y+2-cl) or get_map_flag(x+6-cl,y+2-cl) or get_map_flag(x-cl,y+7-cl) or get_map_flag(x+6-cl,y+7-cl)

  
end


x=80
y=64
tx = 0
ty = 0

ax = 171--村人a
ay = 216

wx=164--0
wy=210--36
prewx = 0
prewy = 0
v = 1
lefttop=0
righttop=0
leftbottom=0
rightbottom=0

s=1 -- スプライト番号
d=1 -- 方向を示す                                                                                  
ipf=8 -- アニメーション1フレームについての時間(1ipf = 1/30秒)
nf=2 -- アニメーションするフレーム数(足踏みは2フレーム)
t=0

pressed=false


function lcx(tx)
  return (tx-wx)*8
end

function lcy(ty)
  return (ty-wy)*8
end

-- last_sound=-1

function setup()
   music(0);--引数変えてもまだ音楽は変わりません（コメントアウトすると音楽なしに）
  -- tool(1);--ツール1はカラーパレットツールの表示
  -- rmap("/init/param/map/w.bin",7,7)
  -- view(1)--主人公の周囲の情報を取得できる範囲
end

function _init()--1回だけ
    setup()
end


function input()
  if btn(6) >= 2 then
    -- appmode(appinfo(0), 1)--エディットモードで開く
  end
  
  local prex = x
  local prey = y
  -- local pressed=0
  pressed=false
  if (btn(1)>=2) then
  -- sound=0
   wx = wx-v
   d=1
   pressed=true
  end
  if (btn(2)>=2) then
  -- sound=1
    wx = wx+v
   d=2
   pressed=true
  end
  if (btn(3)>=2) then
  -- sound=2
   wy = wy-v
   d=3
   pressed=true
  end
  if (btn(4)>=2) then
  -- sound=3
   wy = wy+v
   d=4
   pressed=true
  end

  if pressed then
   s=d+flr((t%(nf*ipf))/ipf+1)*8 --8個先が次のコマ
  else
   s=d
  end

  if collision(x,y) == true then
    -- sound=8
    wx=prewx
    wy=prewy
  end

  wx = wx%256
  wy = wy%256
 end

function _update()
  -- sound=-1
  -- fset(5,0,1)--スプライト0を通れるように（0ビットを0に）する
  fset(0,0,1)--水スプライト45を通れなく（0ビットを1に）する
  -- fset(45,2,1)--水スプライト45を通れなく（0ビットを1に）する
  -- fset(45,3,1)--水スプライト45を通れなく（0ビットを1に）する
  -- fset(45,4,1)--水スプライト45を通れなく（0ビットを1に）する
  --  fset(11,0,0)--岩スプライト11を通れるように（0ビットを0に）する
  -- fset(11,0,0)--岩スプライト11を通れなく（0ビットを1に）する
  -- fset(42,0,0)--緑スプライト11を通れるように（0ビットを0に）する
  -- fset(52,0,0)--スプライトを通れるように（0ビットを0に）する
  -- fset(100,0,1)--スプライト99を通れなく（0ビットを1に）する

  -- t = t+1
  input()
  
  -- if (btnp(5) == true) then
  --   sound=1
  -- end
  -- if (btnp(6) == true) then
  --   sound=2
  -- end
  -- if (btnp(7) == true) then
  --   sound=3
  -- end
  -- if (btnp(8) == true) then
  --   sound=4
  -- end



  -- if sound>-1 then
    -- sfx(sound, 32)
    -- last_sound=sound
  -- end

  -- tone(0, 523, 0)
  -- tone(1, 659, 0)
  -- tone(2, 784, 0)

  -- tone(0, 0)
  -- tone(1, 0)
  -- tone(2, 0)

  -- lefttop=get_map_flag(x+1-cl,y+2-cl)
  -- righttop=get_map_flag(x+5-cl,y+2-cl)
  -- leftbottom=get_map_flag(x+1-cl,y+7-cl)
  -- rightbottom=get_map_flag(x+5-cl,y+7-cl)

end

function _draw()
  -- wy = wy+1
  -- wx = wx%256
  -- wy = wy%256

  if wx~=prewx or wy~=prewy then
    cls(0)
    rmap("/init/param/map/w.bin",wx,wy)
    tone(1, 440, 32)
  end

  -- if x>144 and y>112 then go2("/haco8stage2/main.lua", 100) end--状態100で立ち上げる
  -- spr8(9,144,112)--アイテム

  -- --城を置く
  -- spr8(47, lcx(172),lcy(218), 1,1,0, 0)
  -- spr8(48, lcx(173),lcy(218), 1,1,0, 0)
  -- spr8(55, lcx(172),lcy(219), 1,1,0, 0)
  -- spr8(56, lcx(173),lcy(219), 1,1,0, 0)

  -- for j = 1, 16 do
  --   for i = 1, 20 do
  --     -- 城を置く（2つの配置に対して）
  --     spr8(47, lcx(172 + i*2), lcy(218 + j*2), 1, 1, 0, 0)
  --     spr8(48, lcx(173 + i*2), lcy(218 + j*2), 1, 1, 0, 0)
  --     spr8(55, lcx(172 + i*2), lcy(219 + j*2), 1, 1, 0, 0)
  --     spr8(56, lcx(173 + i*2), lcy(219 + j*2), 1, 1, 0, 0)
  --   end
  -- end

  -- --塔を置く
  -- spr8(17, lcx(165),lcy(217), 1,1,0, 0)
  -- spr8(25, lcx(165),lcy(218), 1,1,0, 0)

  -- --橋を置く
  -- spr8(22, lcx(168),lcy(212), 1,1,0, 0)

  ax = 171--lcx(171)
  ay = 218--lcy(218)

  spr8(s+64,lcx(ax), lcy(ay), 1,1,0, 0,11)--村人スプライト透明色指定のあとに色番号をつけるとふちどり

  -- print(ax/8,120,104)--
  -- print(ay/8,120,112)--

  -- mset(100, ax,ay, wx,wy)--村人aのスプライト情報でマップを書き換えておく
  mset(100, lcx(ax), lcy(ay))
  spr8(s+64,x,y,1,1,0, 0,10)--主人公スプライト透明色指定のあとに色番号をつけるとふちどり

  --向いている方向を取得
if s==9  then 
  tx = x - 8
  ty = y
elseif s==10 then 
  tx = x + 8 
  ty = y
elseif s==11 then 
  tx = x
  ty = y - 8 
elseif s==12 then 
  tx = x
  ty = y + 8 
end

-- print(tx,120,104)--
-- print(ty,120,112)--

  -- get_map_sprn(tx,ty)

  -- print("こんにちは",x,y-16)
  -- color(3)
  -- fillrect(x-4,y-4,8,8)

  -- if pressed then 
  -- print(pressed,120,96)--スプライト値を表示
  -- end
  print(get_map_sprn(tx,ty),120,96)--スプライト値を表示
  print(wx,120,104)--ワールド座標を表示
  print(wy,120,112)--ワールド座標を表示

  -- comand()
  -- print(gstat(),68,2,7)--ゲーム状態を表示

  -- pset(x+1-cl,y+2-cl, 8)
  -- pset(x+5-cl,y+2-cl, 8)
  -- pset(x+1-cl,y+7-cl, 8)
  -- pset(x+5-cl,y+7-cl, 8)
  -- print(lefttop.."/"..righttop,68,2,7)
  -- print(leftbottom.."/"..rightbottom,68,8,7)

  -- rectfill(0,0,127,127,0)
 -- 前回再生されたSFX番号を画面に表示する
--  if last_sound>-1 then
  -- print(last_sound, 68,14,7)
--  end

prewx = wx
prewy = wy
-- spr8(8,tx,ty,1,1,0, 0,10)--向いている方向の座標（スプライトを表示するとマップ情報が描き変わってしまうので最後に表示）
end
-------------------------------------------------------
-- firstF = true
-- function setup()--使えず、、、、
-- end

-- function loop()--update --draw
-- if firstF == true then
--   _init()
--   firstF = false
-- end
-- _update()
-- _draw()
-- end
