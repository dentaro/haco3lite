#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <LovyanGFX_DentaroUI.hpp>

#include "SPIFFS.h"
#include "baseGame.h"
// #include "Tunes.h"
// #include "Editor.h"

#include <bitset>
#include <iostream>
#include <fstream>

#include "Channel.hpp"

extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#ifndef RUN_LUA_GAME_H
#define RUN_LUA_GAME_H

#define MAX_CHAR 2048

struct LoadF {
  File f;
  char* buf; // ポインタを使用

  // コンストラクタでメモリ確保
  LoadF(size_t size) : buf(new char[size]) {}

  // デストラクタでメモリ解放
  ~LoadF() {
    delete[] buf; // ポインタを使用してメモリを解放
  }
};


inline uint16_t lua_rgb24to16(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t tmp = ((r>>3) << 11) | ((g>>2) << 5) | (b>>3);
  return tmp; //(tmp >> 8) | (tmp << 8);
}

class RunLuaGame: public BaseGame
{//クラスはデフォルトではprivata

  public:
    enum WifiPhase{
      NONE,
      SELECT,
      SHOW,
      RUN
    };

    lua_State* L;
    luaL_Buffer b;
    byte col[3] = {0,0,0};
    byte col2[3] = {0,0,0};
    int touchState;//タッチボタン
    int tp[2] ={0,0};
    // uint16_t palette[256];//メモリ不足のため不採用

    // bool wifiDebugRequest = true;//外部ファイルから書き換えテifiモードにできる
    // bool wifiDebugSelf = false;

    WifiPhase wifiMode = NONE;
    int modeSelect = 0;
    bool exitRequest = false;
    bool runError = false;
    String errorString;
    // Vector3::Vector3 boxzero;
    int boxzerox = 60;
    int boxzeroy = 60;

    std::vector<String> fileNamelist;

    // int gameState = 0;

    uint8_t clist[16][3] =
  {
  { 0,0,0},//0: 黒色
  { 27,42,86 },//1: 暗い青色
  { 137,24,84 },//2: 暗い紫色
  { 0,139,75 },//3: 暗い緑色
  { 183,76,45 },//4: 茶色
  { 97,87,78 },//5: 暗い灰色
  { 194,195,199 },//6: 明るい灰色
  { 255,241,231 },//7: 白色
  { 255,0,70 },//8: 赤色
  { 255,160,0 },//9: オレンジ
  { 255,238,0 },//10: 黄色
  { 0,234,0 },//11: 緑色
  { 0,173,255 },//12: 水色
  { 134,116,159 },//13: 藍色
  { 255,107,169 },//14: ピンク
  { 255,202,165}//15: 桃色
  };

    int loadSurface(File* fp, uint8_t* buf);
    static int l_info(lua_State* L);
    static int l_tp(lua_State* L);
    static int l_tstat(lua_State* L);
    static int l_vol(lua_State* L);
    static int l_pinw(lua_State* L);
    static int l_pinr(lua_State* L);
    static int l_tone(lua_State* L);
    static int l_spr(lua_State* L);
    static int l_scroll(lua_State* L);
    static int l_pset(lua_State* L);
    static int l_pget(lua_State* L);
    static int l_color(lua_State* L);
    static int l_text(lua_State* L);
    static int l_opmode(lua_State* L);
    static int l_drawrect(lua_State* L);
    static int l_fillrect(lua_State* L);
    static int l_fillpoly(lua_State* L);
    static int l_drawbox(lua_State* L);
    static int l_drawboxp(lua_State* L);
    static int l_fillcircle(lua_State* L);
    static int l_drawcircle(lua_State* L);
    static int l_drawtri(lua_State* L);
    static int l_filltri(lua_State* L);
    // static int l_phbtn(lua_State* L);
    static int l_key(lua_State* L);
    static int l_btn(lua_State* L);
    static int l_touch(lua_State* L);
    static int l_btnp(lua_State* L);
    static int l_sldr(lua_State* L);
    // static int l_getip(lua_State* L);
    // static int l_iswifidebug(lua_State* L);
    // static int l_wifiserve(lua_State* L);
    static int l_run(lua_State* L);
    static int l_appmode(lua_State* L);
    static int l_appinfo(lua_State* L);
    static int l_editor(lua_State* L);
    static int l_list(lua_State* L);
    // static int l_require(lua_State* L);
    // static int l_httpsget(lua_State* L);
    // static int l_httpsgetfile(lua_State* L);
    // static int l_savebmp(lua_State* L);
    static int l_reboot(lua_State* L);
    static int l_debug(lua_State* L);

    String getBitmapName(String s);
    String getPngName(String s);
    void hsbToRgb(float angle, float si, float br, int& r, int& g, int& b);
    void hsbToRgb2(float angle, float br, int& r, int& g, int& b);
    void fillFastTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c1);
    //継承先の関数を優先するものにはvirtual
    virtual void haco8resume(){};//派生クラスに書き換えられるダミー関数
    //派生クラスでのみ実行されるダミー関数（このクラスでは何の処理もしていない）

    void resume();
    void init();
    int run(int _remainTime);
    void pause();

    // void fillFastTriangle(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c1);
  
    protected://継承先でも使えるもの
    
    
};

#endif
