#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"
#include "haco8/runHaco8Game.h"
// #include "Tunes.h"
// #include "Editor.h"
// #include <PS2Keyboard.h>
// #include "Speaker_Class.hpp"
#include "Channel.hpp"
#include <esp_now.h>
#include <WiFi.h>
#include <LovyanGFX_DentaroUI.hpp>
#include <map>



#include <esp_now.h>

/// 8bit unsigned 44.1kHz mono (exclude wav header)
// extern const uint8_t wav_unsigned_8bit_click[46000];
// extern const uint8_t wav_unsigned_8bit_click[16];

/// wav data (include wav header)
// extern const uint8_t wav_with_header[230432];
// extern const uint8_t wav_with_header[16];

// File mfile;

int pre_startRow = 0;
bool drawWaitF = true;
bool drawBinaryF = false;
int gStartRow = 0;//intでないとLuaから渡せない
uint8_t targetDispRow = 0;
uint8_t writeDispRow = 0;

int8_t diffRow = 0; // 前回の差分を保存しておく

size_t buffAreaNo = 0;
size_t gEfectNo = 0;
float effectVal = 0.0f;
size_t tick = 0;
size_t instrument = 0;
size_t targetChannelNo = 0;//描画編集する効果音番号を設定（sfx(n)のnで効果音番号を指定することで作った効果音がなる）
size_t tickTime = 100;
size_t tickSpeed = 5;
size_t patternNo = 0;//0~63

size_t _octave = 4;// (124 - ui.getPos().y)>>2 / 12 + 4;
size_t _pitch = 0;
size_t _volume = 0;

int select_ui_id = TOUCH_BTN_MODE; // 0はプリセットパネル
int preFlickBtnID = 20;

static int menu_x = 2;
static int menu_y = 20;
static int menu_w = 120;
static int menu_h = 30;
static int menu_padding = 36;
size_t toolNo = 0;

#define KEYBOARD_DATA 32
#define KEYBOARD_CLK  33

// #define TFT_RUN_MODE 0
// #define TFT_EDIT_MODE 1
// #define TFT_WIFI_MODE 2
// #define TFT_SOUND_MODE 3
// #define TFT_MUSICPLAY_MODE 4

#define UI_NO_PRESET 0
#define UI_NO_CUSTOM 1

#define PRESET_BTN_NUM 20

// PS2Keyboard keyboard;

// Speaker_Class channels;
// Speaker_Class* channels = new Speaker_Class();

size_t patternID = 0;
Channel* channels = new Channel();

uint64_t frame = 0;

// ワールドマップ情報を読み込むためのファイルクラス
File wfr;

size_t isEditMode;
bool firstBootF = true;
bool difffileF = false;//前と違うファイルを開こうとしたときに立つフラグ

std::deque<int> buttonState;//ボタンの個数未定

enum struct FileType {
  LUA,
  JS,
  BMP,
  PNG,
  TXT,
  OTHER
};

//24bitRGB
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


String chatstr = "";
// std::string  chatstr = "";

//キーボード関連
// Editor editor;

char keychar;//キーボードから毎フレーム入ってくる文字

//esp-idfのライブラリを使う！
//https://qiita.com/norippy_i/items/0ed46e06427a1d574625
#include <driver/adc.h>//アナログボタンはこのヘッダファイルを忘れないように！！

using namespace std;

#define MAX_CHAR 1
#define FORMAT_SPIFFS_IF_FAILED true

#define BUF_PNG_NUM 0

uint8_t mainVol = 180;

//outputmode最終描画の仕方
// int outputMode = FAST_MODE;//50FPS程度128*128 速いけど小さい画面　速度が必要なモード
int outputMode = WIDE_MODE;//20FPS程度240*240 遅いけれどタッチしやすい画面　パズルなど

uint8_t xpos, ypos = 0;
uint8_t colValR = 0;
uint8_t colValG = 0;
uint8_t colValB = 0;

uint8_t charSpritex = 0;
uint8_t charSpritey = 0;
int pressedBtnID = -1;//この値をタッチボタン、物理ボタンの両方から操作してbtnStateを間接的に操作している

esp_now_peer_info_t slave;

int mapsprnos[16];
// int mapsprnos[16] = { 20, 11, 32, 44, 53, 49, 54, 32, 52, 41, 46, 42, 45, 50, 43, 38 };

//uiIDを変換する
int8_t convUiId[25] = {
-1,-1,-1,-1,-1,
 5, 3, 6,-1,10,
 1, 9, 2,-1,-1,
 7, 4, 8,-1,11,
11,-1,-1, 0,-1,
};
const uint8_t RGBValues[][3] PROGMEM = {//16bit用
  {0, 0, 0},     // 0: 黒色=なし
  {24, 40, 82},  // 1: 暗い青色
  {140, 24, 82}, // 2: 暗い紫色
  {0, 138, 74},  // 3: 暗い緑色
  {181, 77, 41}, // 4: 茶色 
  {99, 85, 74},  // 5: 暗い灰色
  {198, 195, 198}, // 6: 明るい灰色
  {255, 243, 231}, // 7: 白色
  {255, 0, 66},  // 8: 赤色
  {255, 162, 0}, // 9: オレンジ
  {255, 239, 0}, // 10: 黄色
  {0, 235, 0},   // 11: 緑色
  {0, 174, 255}, // 12: 水色
  {132, 117, 156}, // 13: 藍色
  {255, 105, 173}, // 14: ピンク
  {255, 203, 165}  // 15: 桃色
};
//2倍拡大表示用のパラメータ
float matrix_side[6] = {2.0,   // 横2倍
                     -0.0,  // 横傾き
                     258.0,   // X座標
                     0.0,   // 縦傾き
                     2.0,   // 縦2倍
                     0.0    // Y座標
                    };

  static constexpr int HACO3_C0    = 0x0000;
  static constexpr int HACO3_C1    = 6474;//27,42,86 
  static constexpr int HACO3_C2    = 35018;
  static constexpr int HACO3_C3    = 1097;
  static constexpr int HACO3_C4    = 45669;
  static constexpr int HACO3_C5    = 25257;
  static constexpr int HACO3_C6    = 50712;
  static constexpr int HACO3_C7    = 65436;
  static constexpr int HACO3_C8    = 63496;//0xF802;
  static constexpr int HACO3_C9    = 64768;
  static constexpr int HACO3_C10   = 65376;
  static constexpr int HACO3_C11   = 1856;
  static constexpr int HACO3_C12   = 1407;
  static constexpr int HACO3_C13   = 33715;
  static constexpr int HACO3_C14   = 64341;
  static constexpr int HACO3_C15   = 65108;

LGFX screen;//LGFXを継承

LovyanGFX_DentaroUI ui(&screen);
LGFX_Sprite tft(&screen);
static LGFX_Sprite logoSprite( &screen );//背景スプライトはディスプレイに出力

// #include "MapDictionary.h"
// MapDictionary& dict = MapDictionary::getInstance();

LGFX_Sprite sprite88_roi = LGFX_Sprite(&tft);
LGFX_Sprite sprite11_roi = LGFX_Sprite(&tft);
LGFX_Sprite sprite64 = LGFX_Sprite();
// uint8_t sprite64cnos[4096];//64*64


// uint8_t sprite64cnos[PNG_SPRITE_HEIGHT * PNG_SPRITE_WIDTH];//64*128
std::vector<uint8_t> sprite64cnos_vector;

LGFX_Sprite buffSprite = LGFX_Sprite(&tft);

LGFX_Sprite sprite88_0 = LGFX_Sprite(&tft);

// LGFX_Sprite mapTileSprites[1];
// static LGFX_Sprite sliderSprite( &tft );//スライダ用

BaseGame* game;
// Tunes tunes;
String appfileName = "";//最初に実行されるアプリ名
String savedAppfileName = "";
// String txtName = "/init/txt/sample.txt";//実行されるファイル名

uint8_t mapsx = 0;
uint8_t mapsy = 0;
String mapFileName = "/init/param/map/0.csv";
int readmapno = 0;
int divnum = 1;
bool readMapF = false;
//divnumが大きいほど少ない領域で展開できる(2の乗数)
// LGFX_Sprite spritebg[16];//16種類のスプライトを背景で使えるようにする
// LGFX_Sprite spriteMap;//地図用スプライト

// uint8_t mapArray[MAPWH][MAPWH];

uint8_t mapArray[16][20];//配列はyを先にしている（配列をcsvで手書きしたときの方向を一致させて可読性をよくするため）
// uint8_t viewArray[3][3];
// std::vector<std::vector<uint8_t>> mapArray_rl;

bool mapready = false;

int8_t sprbits[128];//fgetでアクセスするスプライト属性を格納するための配列

char buf[MAX_CHAR];
// char str[100];//情報表示用
int mode = 0;//記号モード //0はrun 1はexit
int gameState = 0;
String appNameStr = "init";
int soundNo = -1;
float soundSpeed = 1.0;
int musicNo = -1;
bool musicflag = false;
bool sfxflag = false;
bool toneflag = false;
bool firstLoopF = true;

float sliderval[2] = {0,0};
bool optionuiflag = false;
// uint64_t frame = 0;
// float radone = PI/180;

// float sinValues[90];// 0から89度までの91個の要素

int addUiNum[4];
int allAddUiNum = 0;

size_t addTones[8];
// int alladdTones = 0;

// bool downloadF = true;
// bool isCardMounted = false; // SDカードがマウントされているかのフラグ

int xtile = 0;
int ytile = 0;
float ztile = 0.0;

int xtileNo = 29100;
int ytileNo = 12909;

LGFX_Sprite sprref;
String oldKeys[BUF_PNG_NUM];


uint16_t gethaco3Col(size_t haco3ColNo) {
    uint16_t result = ((static_cast<uint16_t>(clist[haco3ColNo][0]) >> 3) << 11) |
                      ((static_cast<uint16_t>(clist[haco3ColNo][1]) >> 2) << 5) |
                      (static_cast<uint16_t>(clist[haco3ColNo][2]) >> 3);
    return result;
}

// int vol_value; //analog値を代入する変数を定義
// int statebtn_value; //analog値を代入する変数を定義
// int jsx_value; //analog値を代入する変数を定義
// int jsy_value; //analog値を代入する変数を定義
// getSign関数をMapDictionaryクラス外に移動
Vector2<int> getSign(int dirno) {
    if (dirno == -1) {
        // 方向を持たない場合、(0.0, 0.0, 0.0)を返す
        return {0, 0};
    } else {
        float dx = (dirno == 0 || dirno == 1 || dirno == 7) ? 1.0 : ((dirno == 3 || dirno == 4 || dirno == 5) ? -1.0 : 0.0);
        float dy = (dirno == 1 || dirno == 2 || dirno == 3) ? 1.0 : ((dirno == 5 || dirno == 6 || dirno == 7) ? -1.0 : 0.0);
        return {int(dx), int(dy)};
    }
}

// 送信コールバック
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // tft.print("Last Packet Sent to: ");
  // tft.println(macStr);
  // tft.print("Last Packet Send Status: ");
  // tft.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// 受信コールバック
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  char msg[1];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // tft.printf("Last Packet Recv from: %s\n", macStr);//MACアドレスを表示させる

  // tft.printf("Last Packet Recv Data(%d): ", data_len);
  for ( int i = 0 ; i < data_len ; i++ ) {
    msg[1] = data[i];
    // tft.print(msg[1]);
  }
  // tft.println("");
}

Vector2<int> getKey2Sign(String _currentKey, String _targetKey) {
    int slashPos = _currentKey.indexOf('/'); // '/'の位置を取得
    if (slashPos != -1) { // '/'が見つかった場合
        String numA_str = _currentKey.substring(0, slashPos); // '/'より前の部分を取得
        String numB_str = _currentKey.substring(slashPos + 1); // '/'より後の部分を取得
        int numA_current = numA_str.toInt(); // 数字に変換
        int numB_current = numB_str.toInt(); // 数字に変換
        
        slashPos = _targetKey.indexOf('/'); // '/'の位置を取得
        if (slashPos != -1) { // '/'が見つかった場合
            numA_str = _targetKey.substring(0, slashPos); // '/'より前の部分を取得
            numB_str = _targetKey.substring(slashPos + 1); // '/'より後の部分を取得
            int numA_target = numA_str.toInt(); // 数字に変換
            int numB_target = numB_str.toInt(); // 数字に変換
            
            int dx = numA_target - numA_current;
            int dy = numB_target - numB_current;
            
            return {dx, dy};
        }
    }
    
    return {0, 0}; // デフォルトの値
}



void showMusicInfo(){
  
    tft.setCursor(128,1);
    if(patternNo<10)tft.print("0");
    tft.print(patternNo);

    tft.setCursor(148,1);
    // tft.print("SP");
    if(tickSpeed<10)tft.print("0");
    tft.print(tickSpeed);
    
tft.drawLine(128, 9, 160, 9, gethaco3Col(6));

    // tft.fillRect(128, 10, 32, 20, gethaco3Col(13));
    tft.setCursor(128,11);
    for(int n = 0;n<4;n++){tft.print(channels->patterns[patternNo][n]);}
    tft.setCursor(128,21);
    for(int n = 0;n<4;n++){tft.print(channels->patterns[patternNo][4+n]);}

tft.drawLine(128,29, 160,29, gethaco3Col(6));

    // tft.fillRect(138, 30, 32, 40, gethaco3Col(2));
    tft.setCursor(138,31);
    tft.print("IN");
    tft.print(instrument);
    tft.fillRect(128, 30, 10, 9, gethaco3Col(instrument+4));

tft.drawLine(128, 39, 160, 39, gethaco3Col(6));

    tft.setCursor(138,41);
    tft.print(" ");
    // tft.print(instrument);
    // tft.fillRect(128, 40, 10, 9, gethaco3Col(instrument+8));

tft.drawLine(128, 49, 160, 49, gethaco3Col(6));


    tft.setCursor(138,51);
    tft.print("CH");
    tft.print(targetChannelNo);
    tft.fillRect(128, 50, 10, 9, gethaco3Col(targetChannelNo));

tft.drawLine(128, 59, 160,59, gethaco3Col(6));

    tft.setCursor(138,61);
    tft.print("EF");
    tft.print(gEfectNo);
    tft.fillRect(128, 60, 10, 9, gethaco3Col(gEfectNo));

tft.drawLine(128, 69, 160,69, gethaco3Col(6));
}


void controlMusicVisual(){
      for(int i = 0; i<32; i++)//32音書き換える
      {
        size_t efn = channels->notedata[targetChannelNo][i].effectNo;
        size_t ins = channels->notedata[targetChannelNo][i].instrument;
        size_t vol = channels->notedata[targetChannelNo][i].volume;
        size_t oct = channels->notedata[targetChannelNo][i].octave;
        size_t pit = channels->notedata[targetChannelNo][i].pitch;

        if (ui.getEvent() != NO_EVENT)
        {
          if((ui.getPos().x-40)>>2==i){//押した1音だけの音色を変える
            if(96>ui.getPos().y && 0<=ui.getPos().y){ //上の領域で
              //音程を変える
              pit = (96 - ui.getPos().y)>>2 % 12;
              oct = 4;//floor((96 - ui.getPos().y)>>2 / 12)+3;
              //音色を変える
              ins = instrument;
              //エフェクトを変える
              efn = gEfectNo;
            }
            
            if(128>ui.getPos().y && 96<=ui.getPos().y){//下の領域で
              //音量を変える
              vol = (124 - ui.getPos().y)>>2 % 8;
            }
            buffAreaNo = patternNo%2;
            channels->resetTones(i, targetChannelNo, pit, oct, vol, ins, efn, buffAreaNo);//ピッチ,オクターブなどトーンを置き換える
          }
        }

        if(tick == i){

          tft.fillRect(i*4, 92-(pit + (oct-4)*12)*4, 4,4, gethaco3Col(10));
          tft.fillRect(i*4, 124-(vol)*4, 4,4, gethaco3Col(10));

        }else{
          if(efn!=0)tft.drawLine(i*4, 92-(pit + (oct-4)*12)*4+3, i*4+3, 92-(pit + (oct-4)*12)*4+3, gethaco3Col(efn+8));

          tft.fillRect(i*4, 92-(pit + (oct-4)*12)*4, 4,3, gethaco3Col(ins+4));
          tft.fillRect(i*4, 124-(vol)*4, 4,4, gethaco3Col(3));
        }

      }
}

void showMusicVisual(){//操作できないタイプ
    for(int i = 0; i<32; i++)//32音書き換える
    {
      size_t efn = channels->notedata[targetChannelNo][i].effectNo;
      size_t ins = channels->notedata[targetChannelNo][i].instrument;
      size_t vol = channels->notedata[targetChannelNo][i].volume;
      size_t oct = channels->notedata[targetChannelNo][i].octave;
      size_t pit = channels->notedata[targetChannelNo][i].pitch;

      if(tick == i){
        tft.fillRect(i*4, 92-(pit + (oct-4)*12)*4, 4,4, gethaco3Col(10));
        tft.fillRect(i*4, 124-(vol)*4, 4,4, gethaco3Col(10));
      }else{
        if(efn!=0)tft.drawLine(i*4, 92-(pit + (oct-4)*12)*4+3, i*4+3, 92-(pit + (oct-4)*12)*4+3, gethaco3Col(efn+8));

        tft.fillRect(i*4, 92-(pit + (oct-4)*12)*4, 4,3, gethaco3Col(ins+4));
        tft.fillRect(i*4, 124-(vol)*4, 4,4, gethaco3Col(3));
      }
    }
}

Vector3<float> currentgpos = {0,0,0};;
Vector3<float> prePos= {0.0, 0.0, 0.0};
Vector3<float> currentPos = {0,0,0};
Vector3<float> diffPos = {0.0,0.0,0.0};

int dirNos[9];
int shouldNo = 0;
int downloadLimitNum = 0;
String targetKey = "";
float tileZoom = 15.0;
float bairitu = 1.0;

std::vector<String> temporaryKeys;
std::vector<String> previousKeys;
std::vector<String> writableKeys;
std::vector<String> downloadKeys;
std::vector<String> predownloadKeys;
std::vector<String> allKeys;
std::vector<String> preallKeys;

void printDownloadKeys() {
  Serial.println("Download Keys:");
  for (const auto& key : downloadKeys) {
      Serial.print(key);
  }
  Serial.println("");
}

void drawUI()
{
  logoSprite.setPsram(false);
  logoSprite.setColorDepth(16);    // 子スプライトの色深度
  logoSprite.createSprite(48, 38); // ゲーム画面用スプライトメモリ確保

  // PNGをバッファに書いて2倍出力
  logoSprite.drawPngFile(SPIFFS, "/init/logo.png", 0, 0); // 4ボタン
  logoSprite.pushAffine(matrix_side);

  // メモリを解放するのは後で行うこと

  // fillRoundRectの最適化
  uint16_t x1 = 0;
  uint16_t y1 = 128;
  uint16_t bw = 48;
  uint16_t bh = 38;
  uint16_t cornerRadius = 5;
  uint16_t color = TFT_DARKGRAY;

  
  screen.setTextColor(TFT_DARKGRAY, TFT_BLACK);
  screen.setTextSize(2);//サイズ
  // screen.setFont(&lgfxJapanGothicP_8);//日本語可
  // screen.setTextWrap(true);
  // // screen.setClipRect(160, 0, 60, 128);

  for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        if(convUiId[j*5+i] != -1){
          screen.drawRoundRect(x1 + bw * i, y1 + bh * j, bw, bh, cornerRadius, color);
          screen.setCursor(x1 + bw * i +8, y1 + bh * j +8);
          screen.print(convUiId[j*5+i]);
        }
      }
  }

  // screen.fillRoundRect(x1, 124, x2 + 30, 38, cornerRadius, TFT_LIGHTGRAY);

  // screen.fillRoundRect(x1, 164, x2, 38, cornerRadius, TFT_LIGHTGRAY);
  // screen.fillRoundRect(x1 + 30, 164, x2, 38, cornerRadius, TFT_LIGHTGRAY);

  // screen.fillRoundRect(x1, 204, x2 + 30, 38, cornerRadius, TFT_LIGHTGRAY);

  // スプライトの解放
  logoSprite.deleteSprite();

}


bool createChannels()
{

String line;
int j = 0;

// パターンファイルを読み込む
File fr = SPIFFS.open("/init/sound/patterns.csv", "r");
if (!fr)
{
  Serial.println("Failed to open patterns.csv");
  return true; // とりあえず進む
}

j = 0;
while (fr.available()) // 64行だけ読み込む
{
  line = fr.readStringUntil('\n'); // 64行文のパターン（小節）があります
  line.trim();                      // 空白を削除

  if (!line.isEmpty())
  {
    int commaIndex = line.indexOf(',');
    if (commaIndex != -1)
    {
      String val = line.substring(0, commaIndex);
      addTones[0] = val.toInt();

      for (int i = 1; i < 8; i++)
      {
        int nextCommaIndex = line.indexOf(',', commaIndex + 1);
        if (nextCommaIndex != -1)
        {
          val = line.substring(commaIndex + 1, nextCommaIndex);
          addTones[i] = val.toInt();
          commaIndex = nextCommaIndex;
        }
        else
        {
          // Handle the case where there is no trailing comma
          val = line.substring(commaIndex + 1);
          addTones[i] = val.toInt();
          break; // Exit the loop since we reached the end of the line
        }
      }

      for(size_t n=0; n<CHANNEL_NUM; n++){
        channels->setPatterns(j, n, addTones[n]);
      }
      
      j++;
    }
  }
}
fr.close();
  //すべてが終わったらtrueを返す
  return true;
}



bool readTones(size_t _patternNo, size_t buffAreaNo)
{

String line;
int j = 0;

// トーンファイルを読み込む

  for (int chno = 0; chno < CHANNEL_NUM; chno++)
    {
    j = 0;
    patternID = channels->getPatternID( _patternNo, chno);
    // File fr = SPIFFS.open("/init/sound/pattern/"+String(patternID+buffAreaNo)+".csv", "r");
    File fr = SPIFFS.open("/init/sound/pattern/"+String(patternID)+".csv", "r");
    if (!fr)
    {
      Serial.println("Failed to open tones.csv");
      return true;//とりあえず進む
    }
    while (fr.available())
    {
      line = fr.readStringUntil('\n');
      // line.trim(); // 空白を削除
      if (!line.isEmpty())
      {
        int commaIndex = line.indexOf(',');
        if (commaIndex != -1)
        {
          String val = line.substring(0, commaIndex);

          for (int i = 0; i < 8; i++)
          {
            int nextCommaIndex = line.indexOf(',', commaIndex + 1);
            if (nextCommaIndex != -1)
            {
              val = line.substring(commaIndex + 1, nextCommaIndex);
              addTones[i] = val.toInt();
              commaIndex = nextCommaIndex;
            }
          }

          channels->setTones(
              1,
              addTones[0], addTones[1],
              addTones[2], addTones[3],
              addTones[4], addTones[5],
              addTones[6], addTones[7], j, chno, buffAreaNo);
          j++;
        }
      }
    }
    fr.close();
  }

  //すべてが終わったらtrueを返す
  return true;
}

int gWx;
int gWy;
int gSpr8numX = 8;
int gSpr8numY = 8;
int gSprw = 8;
int gSprh = 8;
uint8_t sprno;
uint8_t repeatnum;


TaskHandle_t taskHandle[2];
SemaphoreHandle_t syncSemaphore;
// SemaphoreHandle_t rmapSemaphore;
// SemaphoreHandle_t drawmapSemaphore;
// SemaphoreHandle_t frameSemaphore;
SemaphoreHandle_t nextFrameSemaphore;
// std::vector<uint8_t> rowData;
std::vector<std::vector<uint8_t>> rowData(16);

// int preStartRow = 0;
// bool isFirstRead = true;
// int8_t diffRowNo = 0;

// bool readRowFromBinary(const char *filename) {

//   int _startRow = gWy;
//   //初期化
//   //グローバル変数でなく内部引数_startRowを使わないと取りこぼしが出てしまう
//   std::vector<uint8_t> buffer;
//   int bytesRead;
//   int nnum = 0;

//   if(isFirstRead){//最初だけ読み込み


//     File file = SPIFFS.open(filename, "rb");

//     if (!file) {
//       Serial.println("Error opening file");
//       return false;
//     }
//     // rowData.clear();
//     std::vector<std::vector<uint8_t>> rowData(16, std::vector<uint8_t>());//初期化

//     buffer.resize(8); // バッファのサイズを設定

//     while (nnum < _startRow + DISP_SPR_H_NUM) {
//       int bytesRead = file.read(buffer.data(), buffer.size());

//       if (bytesRead == 0) {
//         // If no bytes were read, it indicates the end of the file
//         break;
//       }

//       for (size_t i = 0; i < bytesRead; ++i) {
//         sprno = buffer[i] >> 5;
//         repeatnum = buffer[i] & 0b00011111;

//         if (_startRow > nnum) {
//           // スタート番号が行ポインタよりも小さい場合はスルーする
//         }
//         else if (_startRow <= nnum) 
//         {
//           if (nnum == _startRow + DISP_SPR_H_NUM) {
//             // 終わっていたらブレーク
//             break;
//           }
          
//               if (sprno == 0 && repeatnum == 0) {
//                 repeatnum = 20;
//               }

//               rowData[nnum-_startRow].push_back(sprno);
//               rowData[nnum-_startRow].push_back(repeatnum);
//         }

//         if (sprno == 3 && repeatnum == 31) {
//           // 改行の時は行ポインタnnumの値を上げる
//           ++nnum;
//         }
//       }
//     }
//     file.close();
//   }
//   else
//   {

//     // 2回目以降の処理（rowDataの中身を更新して描画する）
//     if( diffRow != 0)
//     {
//       File file = SPIFFS.open(filename, "rb");

//       if (!file) {
//         Serial.println("Error opening file");
//         return false;
//       }
//   //     rowData.clear();

//       buffer.resize(8); // バッファのサイズを設定

//   //     while (nnum < _startRow + DISP_SPR_H_NUM) {
//   //     int bytesRead = file.read(buffer.data(), buffer.size());

//   //     if (bytesRead == 0) {
//   //       // If no bytes were read, it indicates the end of the file
//   //       break;
//   //     }

//   //     for (size_t i = 0; i < bytesRead; ++i) {
//   //       sprno = buffer[i] >> 5;
//   //       repeatnum = buffer[i] & 0b00011111;

//   //       if (_startRow > nnum) {
//   //         // スタート番号が行ポインタよりも小さい場合はスルーする
//   //       }
//   //       else if (_startRow <= nnum) 
//   //       {
//   //         if (nnum == _startRow + DISP_SPR_H_NUM) {
//   //           // 終わっていたらブレーク
//   //           break;
//   //         }
          
//   //             if (sprno == 0 && repeatnum == 0) {
//   //               repeatnum = 20;
//   //             }

//   //             rowData[nnum-_startRow].push_back(sprno);
//   //             rowData[nnum-_startRow].push_back(repeatnum);
//   //       }

//   //       if (sprno == 3 && repeatnum == 31) {
//   //         // 改行の時は行ポインタnnumの値を上げる
//   //         ++nnum;
//   //       }
//   //     }
//   //   }

//     file.close();


//     }
    
//   }

//   isFirstRead = false;
//   diffRow =  preStartRow - _startRow; // 前回の差分を保存しておく
//   preStartRow = _startRow;
//   // gStartRow = _startRow;

//   return true;
// }

bool drawMap() {
  int ColCursor = 0;
  size_t sprno;
  size_t presprno = 0; // 初期値を設定
  size_t repeatnum;

  for (size_t k = 0; k < 16; k++) {
    int j = k;
    // int j = (k+16+targetDispRow)%16;
    for (size_t i = 0; i < rowData[k].size(); i += 2) {
      sprno = rowData[k][i];
      repeatnum = rowData[k][i + 1];

      if (sprno == 3 && repeatnum == 31) {
        repeatnum = 0;
        ColCursor = 0;
      } else {
        if (sprno == 7) {
          if (gWy >= 220 || (gWx >= 49 && gWy <= 97)) {
            sprno = 15;
          }
        }

        int sx = sprno % gSpr8numX;
        int sy = sprno / gSpr8numY;

        for (int y = 0; y < 8; y++) {
          for (int x = 0; x < 8; x++) {
            uint8_t bit4;
            int sprpos;
            sprpos = (sy * 8 * PNG_SPRITE_WIDTH + sx * 8 + y * PNG_SPRITE_WIDTH + x) / 2;
            bit4 = sprite64cnos_vector[sprpos];
            if (x % 2 == 1) bit4 = (bit4 & 0b00001111);
            if (x % 2 == 0) bit4 = (bit4 >> 4);
            sprite88_roi.drawPixel(x, y, gethaco3Col(bit4));
          }
        }

        for (int n = 0; n < repeatnum; n++) {
          int sprx = ColCursor + n;
          int displaysprx = sprx - gWx;

          if (sprx >= gWx && sprx < gWx + DISP_SPR_W_NUM) {
            mapArray[j][displaysprx] = sprno;

            // 例: 描画対象が同じ場合は描画をスキップ
            // if (presprno != sprno) {
            //   sprite88_roi.setPivot(4.0, 4.0);
            //   sprite88_roi.pushSprite(&tft, (ColCursor + n - gWx) * 8, j * 8);
            // }

            // sprite88_roi.setPivot(gSprw / 2.0, gSprh / 2.0);
            // sprite88_roi.pushSprite(&tft, (ColCursor + n - gWx) * 8, j * 8);

            sprite88_roi.setPivot(4.0, 4.0);
            sprite88_roi.pushSprite(&tft, (ColCursor + n - gWx) * 8, j * 8);
            // sprite88_roi.pushSprite(&tft, (ColCursor + n - gWx) << 3, j << 3);
            
            

            // 海岸線の場合は描写を足す
            if (displaysprx != 0) {
              if (presprno == 0 && sprno != 0) { // 海から陸ならば
                tft.fillRect((ColCursor + n - gWx - 1) * 8 + 7, j * 8, 1, 8, gethaco3Col(7));
              }
              if (presprno != 0 && sprno == 0) { // 陸から海ならば
                tft.fillRect((ColCursor + n - gWx) * 8, j * 8, 1, 8, gethaco3Col(7));
              }
            }

            if (j != 0) {
              if (sprno != 0 && mapArray[j - 1][displaysprx] == 0) { // 陸の時上が海か
                tft.fillRect((ColCursor + n - gWx) * 8, (j - 1) * 8 + 7, 8, 1, gethaco3Col(7));
              }

              if (sprno == 0 && mapArray[j - 1][displaysprx] != 0) { // 海の時上が陸か
                tft.fillRect((ColCursor + n - gWx) * 8, j * 8, 8, 1, gethaco3Col(7));
              }
            }

            presprno = sprno;
          }
        }

        ColCursor += repeatnum;
      }
    }
  }

  return true;
}


std::vector<uint8_t> buffer;
int bytesRead;

bool readRowFromBinary2(const char *filename) {
  int _startRow = gWy;
  int nnum = 0;

  File mfile = SPIFFS.open(filename, "rb");

  if (!mfile) {
    // Serial.println("Error opening file");
    return false;
  }

  std::vector<std::vector<uint8_t>> new_rowData(16, std::vector<uint8_t>());

  // buffer のクリアとリサイズ
  buffer.clear();
  buffer.resize(8);

  while (nnum < _startRow + DISP_SPR_H_NUM) {
    bytesRead = mfile.read(buffer.data(), buffer.size());

    if (bytesRead == 0) {
      break;
    }

    for (int i = 0; i < bytesRead; i++) {
      sprno = buffer[i] >> 5;
      repeatnum = buffer[i] & 0b00011111;

      if (nnum >= _startRow && nnum < _startRow + DISP_SPR_H_NUM) {
        if (sprno == 0 && repeatnum == 0) {
          repeatnum = 255;
        }

        new_rowData[nnum - _startRow].push_back(sprno);
        new_rowData[nnum - _startRow].push_back(repeatnum);
      }

      if (sprno == 3 && repeatnum == 31) {
        ++nnum;
      }
    }
  }

  mfile.close();

  // rowData を new_rowData に入れ替える
  rowData.swap(new_rowData);

  mfile = File();  // 不要な行の削除

  // drawWaitF = false;
  return true;
}

void createChannelsTask(void *pvParameters) {
    while (true) {
            while (!createChannels()) {
                delay(10);
            }
            readTones(patternNo, 0);
            readTones(patternNo + 1, 1);
            xSemaphoreGive(syncSemaphore);
            delay(10);
    }
}

void musicTask(void *pvParameters) {
    while (true) {
          // 何らかの条件が満たされるまで待機
        if (xSemaphoreTake(syncSemaphore, portMAX_DELAY)) {
            // 同期が取れたらここに入る
            channels->begin();
            channels->setVolume(200); // 0-255
            channels->setAllChannelVolume(127);
            channels->note(0, tick, patternNo);
            channels->note(1, tick, patternNo);
            channels->note(2, tick, patternNo);
            channels->note(3, tick, patternNo);

            channels->note(4, tick, patternNo);
            channels->note(5, tick, patternNo);
            channels->note(6, tick, patternNo);
            channels->note(7, tick, patternNo);
            channels->stop();
            xSemaphoreGive(syncSemaphore);
        }
            
            tick++;
            tick %= TONE_NUM;

            if (tick == 0) {
                patternNo++;

                if (patternNo >= PATTERN_NUM) {
                    patternNo = 0;
                }
            }

            // delay(1);
        }

        // 他の処理や適切な待機時間をここに追加
        // delay(10);
    
}


void reboot()
{
  ESP.restart();
}

FileType detectFileType(String *appfileName)
{
  if(appfileName->endsWith(".js")){
    return FileType::JS;
  }else if(appfileName->endsWith(".lua")){
    return FileType::LUA;
  }else if(appfileName->endsWith(".bmp")){
    return FileType::BMP;
  }else if(appfileName->endsWith(".png")){
    return FileType::PNG;
  }else if(appfileName->endsWith(".txt")){
    return FileType::TXT;
  }
  return FileType::OTHER;
}

String *targetfileName;
BaseGame* nextGameObject(String* _appfileName, int _gameState, String _mn)
{

  switch(detectFileType(_appfileName)){
    case FileType::JS:  
      // game = new RunJsGame(); 
      break;
    case FileType::LUA: 
      game = new RunHaco8Game(_gameState, _mn);
      break;
    case FileType::TXT: 
      // game = new RunJsGame(); 
      // //ファイル名がもし/init/param/caldata.txtなら
      // if(*_appfileName == CALIBRATION_FILE)
      // {
      //   ui.calibrationRun(screen);//キャリブレーション実行してcaldata.txtファイルを更新して
      //   drawUI();//サイドボタンを書き直して
      // }
      // appfileName = "/init/txt/main.js";//txtエディタで開く
      break; //txteditorを立ち上げてtxtを開く
    case FileType::BMP: // todo: error
      game = NULL;
      break;
    case FileType::PNG: // todo: error
      // game = new RunJsGame(); 
      // appfileName = "/init/png/main.js";//pngエディタで開く
      break;
    case FileType::OTHER: // todo: error
      game = NULL;
      break;
  }

  return game;

}

uint32_t preTime;

void runFileName(String s){
  
  ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
  
  appfileName = s;
  mode = 1;//exit to run

}

// タイマー
hw_timer_t * timer = NULL;

void readFile(fs::FS &fs, const char * path) {
   File file = fs.open(path);
   while(file.available()) file.read();
  //  while(file.available()) Serial.print(file.read());
  file.close();
}

//ファイル書き込み
void writeFile(fs::FS &fs, const char * path, const char * message){
    File file = fs.open(path, FILE_WRITE);
    if(!file){
      file.close();
        return;
    }
    file.print(message);
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
   Serial.print("Deleting file: ");
   Serial.println(path);
   if(fs.remove(path)) Serial.print("− file deleted\n\r");
   else { Serial.print("− delete failed\n\r"); }
}

void listDir(fs::FS &fs){
   File root = fs.open("/");
   File file = root.openNextFile();
   while(file){
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.print(file.size());
      file = root.openNextFile();
   }
   root.close();
   file.close();
}

vector<string> split(string& input, char delimiter)
{
    istringstream stream(input);
    string field;
    vector<string> result;
    while (getline(stream, field, delimiter)) {
        result.push_back(field);
    }
    return result;
}

String rFirstAppName(String _wrfile){
  File fr = SPIFFS.open(_wrfile.c_str(), "r");// ⑩ファイルを読み込みモードで開く
  String _readStr = fr.readStringUntil('\n');// ⑪改行まで１行読み出し
  fr.close();	// ⑫	ファイルを閉じる
  return _readStr;
}

// #include <Arduino.h>
// #include <SPIFFS.h>

void decompressData(const char *filename, std::vector<std::vector<uint8_t>> &output) {

}


int readMap(String _mapFileName, int startCol, int startRow) {

  return 1;
}


void getOpenConfig()
{
  File fr;

  fr = SPIFFS.open(SPRBITS_FILE, "r");
  for (int i = 0; i < 128; i++) {
    String _readStr = fr.readStringUntil(','); // ,まで１つ読み出し
    std::string _readstr = _readStr.c_str();

    // 改行を取り除く処理
    const char CR = '\r';
    const char LF = '\n';
    std::string destStr;
    for (std::string::const_iterator it = _readstr.begin(); it != _readstr.end(); ++it) {
      if (*it != CR && *it != LF && *it != '\0') {
        destStr += *it;
      }
    }

    _readstr = destStr;

    uint8_t bdata = 0b00000000;
    uint8_t bitfilter = 0b10000000; // 書き換え対象ビット指定用

    for (int j = 0; j < _readstr.length(); ++j) {
        char ch = _readstr[j];
        // Serial.print(ch);
        if (ch == '1') {
            bdata |= bitfilter; // 状態を重ね合わせて合成
        }
        bitfilter = bitfilter >> 1; // 書き換え対象ビットを一つずらす
    }

    sprbits[i] = bdata;
    // Serial.print(":");
    // Serial.print(bdata); // 0～255
    // Serial.print(":");
    // Serial.println("end");
  }
  fr.close();

  fr = SPIFFS.open("/init/param/openconfig.txt", "r");
  String line;
  while (fr.available()) {
    line = fr.readStringUntil('\n');
    if (!line.isEmpty()) {
      int commaIndex = line.indexOf(',');
        String val = line.substring(0, commaIndex);
        if(val != NULL){
          appfileName =  val;

          // Serial.print(appfileName.c_str());
          // Serial.println("<-");

        }else {
          appfileName = "/init/main.lua";//configファイルが壊れていても強制的に値を入れて立ち上げる
        }
          int nextCommaIndex = line.indexOf(',', commaIndex + 1);//一つ先のカンマ区切りの値に進める
          if (nextCommaIndex != -1) {
            val = line.substring(commaIndex + 1, nextCommaIndex);
            if(val.toInt() != NULL){
              isEditMode = val.toInt();
              Serial.print("editmode[");Serial.print(isEditMode);Serial.println("]");
            }else{
              isEditMode = 0;//configファイルが壊れていても強制的に値を入れて立ち上げる
            }
          }
    }
  }
  fr.close();

  Serial.print(appfileName.c_str());
  Serial.println("<---");

  string str1 = appfileName.c_str();
  int i=0;
  char delimiter = '/';

  std::vector<std::string> result = split(str1, delimiter);

    // 分割結果の表示
    for (const std::string& s : result) {
        if(i==1){
        appNameStr = s.c_str();

        // Serial.print("/" + appNameStr + "/mapinfo.txt");
        // Serial.println("<-------");

        fr = SPIFFS.open("/" + appNameStr + "/mapinfo.txt", "r");// ⑩ファイルを読み込みモードで開く
      }
      i++;
    }
      
  // アプリで使うマップ名を取得する
  String _readStr;
  while (fr.available()) {
      String line = fr.readStringUntil('\n');
      int j = 0; // 列のインデックス
      int startIndex = 0;

      if (!line.isEmpty()) {
          // Serial.print(line);
          // Serial.println("<--------");

          while (j < 16) {
              int commaIndex = line.indexOf(',', startIndex);

              // if (j < 16) { // 0から15番目まで
                  if (commaIndex != -1) {
                      String columnValue = line.substring(startIndex, commaIndex);
                      mapsprnos[j] = atoi(columnValue.c_str());
                  } else {
                      // 行の末尾まで達した場合
                      mapsprnos[j] = atoi(line.substring(startIndex).c_str());
                  }
              if (commaIndex == -1) {
                  // 行の末尾まで達した場合
                  break;
              }

              startIndex = commaIndex + 1;
              j++;
          }

          i++;
      }
  }
  fr.close();

  mapFileName = "/init/param/map/"+_readStr;
  // Serial.print(mapFileName);
  // Serial.println("<--------------");

  // for(int i = 0;i<16;i++){
  //   Serial.print(mapsprnos[i]);
  //   Serial.print(",");
  // }
  // Serial.println("<--------------");

}

void setOpenConfig(String fileName, size_t _isEditMode) {
  char numStr[64];//64文字まで
  sprintf(numStr, "%s,%d,", 
    fileName.c_str(), _isEditMode
  );

  // Serial.println(fileName.c_str());
  // Serial.println(_isEditMode);

  String writeStr = numStr;  // 書き込み文字列を設定
  File fw = SPIFFS.open("/init/param/openconfig.txt", "w"); // ファイルを書き込みモードで開く
  fw.println(writeStr);  // ファイルに書き込み
  // savedAppfileName = fileName;
  delay(50);
  fw.close(); // ファイルを閉じる
}


using namespace std;
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <cmath>

using namespace std;

// vector<string> split(string& input, char delimiter)
// {
//     istringstream stream(input);
//     string field;
//     vector<string> result;
//     while (getline(stream, field, delimiter)) {
//         result.push_back(field);
//     }
//     return result;
// }

void setTFTedit(size_t _iseditmode){
    tft.setPsram( false );//DMA利用のためPSRAMは切る
    tft.createSprite( TFT_WIDTH, TFT_HEIGHT );//PSRAMを使わないギリギリ
    tft.startWrite();//CSアサート開始
  // if(_iseditmode ==TFT_RUN_MODE){
  //   tft.setPsram( false );//DMA利用のためPSRAMは切る
  //   tft.createSprite( TFT_WIDTH, TFT_HEIGHT );//PSRAMを使わないギリギリ
  //   tft.startWrite();//CSアサート開始
  // }else if(_iseditmode == TFT_EDIT_MODE){
  //   tft.setPsram( false );//DMA利用のためPSRAMは切る
  //   tft.createSprite( TFT_WIDTH, TFT_HEIGHT );
  //   tft.startWrite();//CSアサート開始
  // }
  // else if(_iseditmode == TFT_WIFI_MODE){
  //   tft.setPsram( false );//DMA利用のためPSRAMは切る
  //   tft.createSprite( TFT_WIDTH, TFT_HEIGHT );
  //   tft.startWrite();//CSアサート開始
  // }
}


void createPanels(int _uiContainerID, int _read_ui_no)
{

  // 抽象UIを生成
  File fr = SPIFFS.open("/init/param/uiinfo.txt", "r");
  String line;

  while (fr.available())
  {
    line = fr.readStringUntil('\n');
    if (!line.isEmpty())
    {
      int commaIndex = line.indexOf(',');
      if (commaIndex != -1)
      {
        String val = line.substring(0, commaIndex);
        addUiNum[0] = val.toInt();
        if (addUiNum[0] == _read_ui_no)
        { // 指定されたUI以外は生成しない

          // if(addUiNum[0]<=-1){//-1位下の時は生成しない

          // }
          // else{//0の時はボタンを生成
          for (int i = 1; i < 8; i++)
          {
            int nextCommaIndex = line.indexOf(',', commaIndex + 1);
            if (nextCommaIndex != -1)
            {
              val = line.substring(commaIndex + 1, nextCommaIndex);
              addUiNum[i] = val.toInt();
              commaIndex = nextCommaIndex;
            }
          }

          ui.createPanel(
              addUiNum[0], addUiNum[1],
              addUiNum[2], addUiNum[3],
              addUiNum[4], addUiNum[5],
              addUiNum[6], addUiNum[7], ui.getTouchZoom(), MULTI_EVENT, XY_VAL, _uiContainerID);

          allAddUiNum++;

          // }
        }
      }
    }
  }
  fr.close();

  if (ui.uis[1]->panels.size() == 4)
  { // flickUIで使う4つのパネルが揃っていたら
    ui.setupFlickData(1);
  }
}

int btn(int btnno)
{
  return buttonState[btnno];//ボタンの個数未定
}


void reboot(String _fileName, size_t _isEditMode)
{
  setOpenConfig(_fileName, _isEditMode);
  // editor.setCursorConfig();//カーソルの位置を保存
  delay(100);
  ESP.restart();
}

void restart(String _fileName, size_t _isEditMode)
{
  setOpenConfig(_fileName, _isEditMode);
  
  // editor.setCursorConfig();//カーソルの位置を保存
  // delay(100);

  firstBootF = false;
  setup();

  // tunes.pause();
  game->pause();
  free(game);
  firstLoopF = true;
  toneflag = false;
  sfxflag = false;
  musicflag = false;
  // txtName = _fileName;
  game = nextGameObject(&_fileName, gameState, mapFileName);//ファイルの種類を判別して適したゲームオブジェクトを生成
  game->init();//resume()（再開処理）を呼び出し、ゲームで利用する関数などを準備
  // tunes.resume();
}

// void broadchat(String message) {

//   // ファイルを書き込みモードで開く
//   File file = SPIFFS.open("/init/chat/m.txt", FILE_WRITE);
//   if (!file) {
//     // ファイルが開けない場合は終了
//     return;
//   }

//   // ファイルにメッセージを書き込む
//   file.print(message);
//   file.close(); // ファイルを閉じる

//   // ファイルに書き込んだ内容をESP-NOWを使用して送信
//   file = SPIFFS.open("/init/chat/m.txt", FILE_READ);
//   if (!file) {
//     // ファイルが開けない場合は終了
//     return;
//   }

//   std::vector<uint8_t> data;
//   while (file.available()) {
//     char c = file.read();
//     data.push_back(c);
//     if (data.size() >= 150) {
//       esp_err_t result = esp_now_send(slave.peer_addr, data.data(), data.size());
//       data.clear(); // データを送信したらクリア
//       if (result != ESP_OK) {
//         // 送信に失敗した場合の処理を記述
//         return;
//       }
//     }
//   }

//   file.close(); // ファイルを閉じる
// }

void broadchat(String message) {

  // ファイルを書き込みモードで開く
  File fp = SPIFFS.open("/init/chat/m.txt", FILE_WRITE);
  if (!fp) {
    // ファイルが開けない場合は終了
    return;
  }
  // ファイルにメッセージを書き込む
  fp.print(message);
  fp.close(); // ファイルを閉じる

  if ("/init/chat/m.txt" == NULL) return;
  fp = SPIFFS.open("/init/chat/m.txt", FILE_READ); // SPIFFSからファイルを読み込み

  if (!fp) {
    // editor.editorSetStatusMessage("Failed to open file");
    return;
  }

  std::vector<uint8_t> data;
  while (fp.available()) {
    char c = fp.read();
    data.push_back(c);
    if (data.size() >= 150) {
      esp_err_t result = esp_now_send(slave.peer_addr, data.data(), data.size());
      data.clear(); // データを送信したらクリア
      if (result != ESP_OK) {
        // tft.println("");
        // editor.editorSetStatusMessage("Failed to send message");
        fp.close();
        return;
      }
    }
  }

  // ファイルの残りのデータを送信
  if (data.size() > 0) {
    esp_err_t result = esp_now_send(slave.peer_addr, data.data(), data.size());
    if (result != ESP_OK) {
      // editor.editorSetStatusMessage("Failed to send message");
      fp.close();
      return;
    }
  }

  fp.close();
  // editor.editorSetStatusMessage("Message sent");
}


uint8_t readpixel(int i, int j)
{
        // int k = j+(MAPWH/divnum)*(n);//マップ下部
        colValR = sprite64.readPixelRGB(i,j).R8();
        colValG = sprite64.readPixelRGB(i,j).G8();
        colValB = sprite64.readPixelRGB(i,j).B8();

  //16ビットRGB（24ビットRGB）
        if(colValR==0&&colValG==0&&colValB==0){//0: 黒色=なし
          return 0;//20;
        }else if(colValR==24&&colValG==40&&colValB==82){//{ 27,42,86 },//1: 暗い青色
          return 1;//11;//5*8+5;
        }else if(colValR==140&&colValG==24&&colValB==82){//{ 137,24,84 },//2: 暗い紫色
          return 2;//32;//5*8+5;
        }else if(colValR==0&&colValG==138&&colValB==74){//{ 0,139,75 },//3: 暗い緑色
          return 3;//44;//5*8+5;
        }else if(colValR==181&&colValG==77&&colValB==41){//{ 183,76,45 },//4: 茶色 
          return 4;//53;//5*8+5;
        }else if(colValR==99&&colValG==85&&colValB==74){//{ 97,87,78 },//5: 暗い灰色
          return 5;//49;
        }else if(colValR==198&&colValG==195&&colValB==198){//{ 194,195,199 },//6: 明るい灰色
          return 6;//54;//5*8+5;
        }else if(colValR==255&&colValG==243&&colValB==231){//{ 255,241,231 },//7: 白色
          return 7;//32;
        }else if(colValR==255&&colValG==0&&colValB==66){//{ 255,0,70 },//8: 赤色
          return 8;//52;
        }else if(colValR==255&&colValG==162&&colValB==0){//{ 255,160,0 },//9: オレンジ
          return 9;//41;//5*8+5;
        }else if(colValR==255&&colValG==239&&colValB==0){//{ 255,238,0 },//10: 黄色
          return 10;//46;
        }else if(colValR==0&&colValG==235&&colValB==0){//{ 0,234,0 },//11: 緑色
          return 11;//42;
        }else if(colValR==0&&colValG==174&&colValB==255){//{ 0,173,255 },//12: 水色
          return 12;//45;//5*8+5;
        }else if(colValR==132&&colValG==117&&colValB==156){//{ 134,116,159 },//13: 藍色
          return 13;//50;
        }else if(colValR==255&&colValG==105&&colValB==173){//{ 255,107,169 },//14: ピンク
          return 14;//43;//5*8+5;
        }else if(colValR==255&&colValG==203&&colValB==165){//{ 255,202,165}//15: 桃色
          return 15;//38;//5*8+5;
        }
}

size_t cursor_index = 0;


void safeReboot(){
  // editor.setCursorConfig(0,0,0);//カーソルの位置を強制リセット保存
      delay(50);

      ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
      appfileName = "/init/main.lua";
      
      firstLoopF = true;
      toneflag = false;
      sfxflag = false;
      musicflag = false;

      // editor.editorSave(SPIFFS);//SPIFFSに保存
      delay(100);//ちょっと待つ
      reboot(appfileName, TFT_RUN_MODE);//現状rebootしないと初期化が完全にできない
}


// void musicTask(void *pvParameters);


void setup()
{

  
    // 同期用セマフォの作成
    syncSemaphore = xSemaphoreCreateBinary();
    // rmapSemaphore = xSemaphoreCreateBinary();
    // drawmapSemaphore = xSemaphoreCreateBinary();
    // frameSemaphore = xSemaphoreCreateBinary();
    nextFrameSemaphore = xSemaphoreCreateBinary();

    // SPIFFSをマウント
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    if(isEditMode != TFT_WIFI_MODE){

      // createChannelsTask タスクの作成（コア0で実行）
      xTaskCreatePinnedToCore(
        createChannelsTask,
        "createChannelsTask",
        2048,//~3000だと非力だけど動く//2048,4096なら動く//1024だと動かない
        NULL,
        1,
        &taskHandle[0],  // タスクハンドルを取得
        0//0だと落ちる
      );

      // musicTask タスクの作成（コア1で実行）
      xTaskCreatePinnedToCore(
        musicTask,
        "musicTask",
        2048,////1024だと動く//1500だと非力だけど動く//2048だと動く
        NULL,
        2,
        &taskHandle[1],//NULL,// タスクハンドルを取得
        0 // タスクを実行するコア（0または1）
      );


      // xTaskCreatePinnedToCore(
      //   rmapTask,
      //   "rmapTask",
      //   3596,//
      //   NULL,
      //   3,
      //   &taskHandle[2],  // タスクハンドルを取得
      //   0
      // );

      // xTaskCreatePinnedToCore(
      //   drawmapTask,
      //   "drawmapTask",
      //   8000,//3596,//
      //   NULL,
      //   4,
      //   &taskHandle[3],  // タスクハンドルを取得
      //   0
      // );
    }

    // createChannelsが完了するまでmusicTaskをブロック
    while (!createChannels()) {
        delay(1);
    }
    buffAreaNo = (patternNo+1)%2;//一つ先のパターンを読み込んでおく
    readTones(patternNo+1, buffAreaNo);
    // //同期用セマフォを解放
    xSemaphoreGive(syncSemaphore);

  if(isEditMode == TFT_WIFI_MODE){//WIFIモードのときは音でない
    // ESP-NOW初期化
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() == ESP_OK) {
      tft.println("ESPNow Init Success");
    } else {
      tft.println("ESPNow Init Failed");
      ESP.restart();
    }
  }
  else{
    //音楽ループ用のタスク
    // xTaskCreate(musicTask, "MusicTask", 1000, NULL, 1, NULL);
    // xTaskCreatePinnedToCore(musicTask, "MusicTask", 4096, NULL, 1, NULL, PRO_CPU_NUM);
  }

  //------
  pinMode(OUTPIN_0, OUTPUT);
  pinMode(INPIN_0, INPUT);

  Serial.begin(115200);
  // keyboard.begin(KEYBOARD_DATA, KEYBOARD_CLK);

  // editor.getCursorConfig("/init/param/editor.txt");//エディタカーソルの位置をよみこむ

  delay(50);
  if(firstBootF == true){
    difffileF = false;

    #if !defined(__MIPSEL__)
      while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
      #endif
      Serial.println("Keyboard Start");

    if (!SPIFFS.begin(true))
    {
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  }

  getOpenConfig();//最初に立ち上げるゲームのパスとモードをSPIFFSのファイルopenconfig.txtから読み込む
  //この時点でappfileNameも更新される

  


//   File fr;
//   //アプリのパスからアプリ名を取得
//   string str1 = appfileName.c_str();
//   int i=0;

//   // for (string s : split(str1,'/')) {
//   //   if(i==1){
//   //     appNameStr = s.c_str();
//   //     fr = SPIFFS.open("/" + appNameStr + "/mapinfo.txt", "r");// ⑩ファイルを読み込みモードで開く
//   //   }
//   //    i++;
//   // }
// // String appNameStr;
//   String line;
//   String _readStr;

//   while (fr.available()) {
//     line = fr.readStringUntil('\n');
//     int j = 0; // 列のインデックス
//     int startIndex = 0;
//     if (!line.isEmpty()) {

//       Serial.println(line);
//         while (j < 17) {
//           int commaIndex = line.indexOf(',', startIndex);
//           if(j==16){//0から数えて16番目がアプリ名
//             appNameStr  = line.substring(startIndex, commaIndex);
//             fr = SPIFFS.open("/" + appNameStr + "/mapinfo.txt", "r");// ⑩ファイルを読み込みモードで開く
//           }
//         j++;
//       }

//       i++;

//     }
//   }

  
//   i = 0; // 行のインデックス

//   while (fr.available()) {
//     line = fr.readStringUntil('\n');
//     int j = 0; // 列のインデックス
//     int startIndex = 0;
//     if (!line.isEmpty()) {

//       // Serial.println(line);
//         while (j < 17) {
//           int commaIndex = line.indexOf(',', startIndex);
//           if(j<16){
//             if (commaIndex == -1) {
//               commaIndex = line.length();
//             }
//             String token = line.substring(startIndex, commaIndex);
//             mapsprnos[j] = token.toInt();
//             startIndex = commaIndex + 1;
//           }else{
//             _readStr = line.substring(startIndex, commaIndex);
//           }
//         j++;
//       }

//       i++;

//     }
//   }


  // String _readStr;
  // for(int i= 0;i<16;i++){//マップを描くときに使うスプライト番号リストを読み込む
  //   _readStr = fr.readStringUntil(',');// ⑪,まで１つ読み出し
  //   mapsprnos[i] = atoi(_readStr.c_str());
  // }

  // Serial.println("->");
  // Serial.print(_readStr);

  // _readStr = fr.readStringUntil(',');// 最後はマップのパス

  // _readStr = "2.txt";//強制的に入れる

  // mapFileName = "/init/param/map/"+_readStr;
  // fr.close();	// ⑫	ファイルを閉じる

  //この時点でmapFileNameが更新される
  // readMap(mapFileName);
  // delay(50);

  if(isEditMode == TFT_RUN_MODE){

    if(firstBootF == false){
      tft.deleteSprite();
      delay(100);
    }
    setTFTedit(TFT_RUN_MODE);

    //   //外部物理ボタンの設定
    // adc1_config_width(ADC_WIDTH_BIT_12);
    // //何ビットのADCを使うか設定する。今回は12bitにします。
    // //adc1の場合はこのように使うチャンネル全体の設定をするコマンドが用意されている。
    // adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);//39pin　4つのボタン
    // adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);//33pin　ボリューム
    // adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);//34pin　ジョイスティックX
    // adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);//35pin　ジョイスティックY
    
    // ui.begin( screen, 16, 2, 1);
    ui.begin( screen, 16, 2, 0);//タッチキャリブレーションをしない
    screen.fillScreen(TFT_BLACK);
    drawUI();//UIを表示

    ui.createUIs(2); // 最大2つのUIパネルを使用
    createPanels(0, 0); // UIコンテナ0にID０（プリセットUI）を生成
    createPanels(1, select_ui_id); // UIコンテナ1（カスタムUI）にID１～を生成

    ui.setSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, 0.3,0.5);
    
    //     while(!createChannels()){
      
    // };

    // ui.createSliders( 0, 160, 240, 160, 2, 1, XY_VAL, MULTI_EVENT );
    // ui.setBtnName( ui.getUiID("SLIDER_0"), "2DSlider0" );
    // ui.setBtnName( ui.getUiID("SLIDER_0")+1, "2DSlider1" );

    //sin 0~90度をメモリに保持する
    // for (int i = 0; i < 90; ++i) {
    //   float radians = i * M_PI / 180.0;
    //   sinValues[i] = sin(radians);
    // }
    // sinValues[90] = 1.0;
    // sinValues[270] = -1.0;

    sprite88_0.setPsram(false );
    sprite88_0.setColorDepth(16);//子スプライトの色深度
    sprite88_0.createSprite(8, 8);//ゲーム画面用スプライトメモリ確保

    //sprite88_0.drawPngFile(SPIFFS, "/init/sprite.png", -8*1, -8*0);

    sprite64.setPsram(false );
    sprite64.setColorDepth(16);//子スプライトの色深度
    sprite64.createSprite(PNG_SPRITE_WIDTH, PNG_SPRITE_HEIGHT);//ゲーム画面用スプライトメモリ確保//wroomだと64*128だとメモリオーバーしちゃう問題を色番号配列にして回避した

    sprite64.drawPngFile(SPIFFS, "/init/initspr.png", 0, 0);//一時展開する

    //色番号配列化
    // for(int y=0;y<PNG_SPRITE_HEIGHT;y++){
    //   for(int x=0;x<PNG_SPRITE_WIDTH;x++){
    //     if(x%2 == 0){
    //       sprite64cnos[(y*PNG_SPRITE_WIDTH+x)] = (readpixel(x  , y) << 4) | (readpixel(x+1, y) & 0b00001111);//ニコイチにして格納
    //     }
    //   }
    // }

    sprite64cnos_vector.clear();//初期化処理

    for(int y = 0; y < PNG_SPRITE_HEIGHT; y++) {
        for(int x = 0; x < PNG_SPRITE_WIDTH; x++) {
          if(x%2 == 0){
            uint8_t pixel_data = (readpixel(x, y) << 4) | (readpixel(x + 1, y) & 0b00001111);
            sprite64cnos_vector.push_back(pixel_data);
          }
        }
    }

    //破棄
    sprite64.deleteSprite();

    //psram使えない-------------------------------------------
    // buffSprite.setPsram( true );
    // buffSprite.setColorDepth(16);//子スプライトの色深度
    // buffSprite.createSprite(256, 256);//ゲーム画面用スプライトメモリ確保

    // for( int i = 0; i < BUF_PNG_NUM; i++ ){
    //   mapTileSprites[i].setPsram(true);
    //   mapTileSprites[i].setColorDepth(16);
    //   mapTileSprites[i].createSprite(256,256);
    //   // MapTile クラスをインスタンス化し、スプライトに描画して返す
    //   dict.copy2buff(buffSprite, &mapTileSprites[i], i);
    // }

    // //キーと紐づけ、初期設定のキー0~9と値のペアを適当に登録しておく
    // for(int j = 0; j<3; j++){
    //   for(int i = 0; i<3; i++){
    //     dict.setSprptr(i*3+j, &mapTileSprites[i]);
    //     dict.setNewKey(i*3+j, String(xtileNo+i) + "/" + String(ytileNo+j));
    //     dict.showKeyInfo(String(xtileNo+i) + "/" + String(ytileNo+j));
    //   }
    // }
    //psram使えない-------------------------------------------

    sprite88_roi.setPsram(false );
    sprite88_roi.setColorDepth(16);//子スプライトの色深度
    sprite88_roi.createSprite(8, 8);//ゲーム画面用スプライトメモリ確保

    sprite11_roi.setPsram(false );
    sprite11_roi.setColorDepth(16);//子スプライトの色深度
    sprite11_roi.createSprite(1, 1);//ゲーム画面用スプライトメモリ確保

    // spriteMap.setPsram(false );
    // spriteMap.setColorDepth(16);//子スプライトの色深度
    // spriteMap.createSprite(MAPWH, MAPWH/divnum);//マップ展開用スプライトメモリ確保
    
    if(firstBootF == true)
    {

      game = nextGameObject(&appfileName, gameState, mapFileName);//ホームゲームを立ち上げる（オブジェクト生成している）
      game->init();//（オブジェクト生成している）
      // tunes.init();//（オブジェクト生成している）
    }

    frame=0;
    }
    else if(isEditMode == TFT_EDIT_MODE)//エディットモードの時
    {
    //   if(firstBootF == false){
    //     tft.deleteSprite();
    //     delay(10);
    //   }
    //   setTFTedit(TFT_EDIT_MODE);
      
    //   ui.begin( screen, 16, 1, 1);

    //   if(firstBootF == true)
    //   {

    //     if (SPIFFS.exists(appfileName)) {
    //       File file = SPIFFS.open(appfileName, FILE_READ);
    //       if (!file) {
    //         Serial.println("ファイルを開けませんでした");
    //         return;
    //       }
    //       // ファイルからデータを読み込み、シリアルモニターに出力
    //       while (file.available()) {
    //         Serial.write(file.read());
    //       }
    //       // ファイルを閉じる
    //       file.close();
    //     }

    //   createPanels();
    //   game = nextGameObject(&appfileName, gameState, mapFileName);//ホームゲームを立ち上げる（オブジェクト生成している）
    //   game->init();//（オブジェクト生成している）
    //   // tunes.init();//（オブジェクト生成している）

    //   frame=0;

    //   // editor.initEditor(tft, EDITOR_ROWS, EDITOR_COLS);
    //   // editor.initEditor(tft);
    //   // editor.readFile(SPIFFS, appfileName.c_str());
    //   // editor.editorOpen(SPIFFS, appfileName.c_str());
    //   // editor.editorSetStatusMessage("Press ESCAPE to save file");
    // }
  }
  else if(isEditMode == TFT_WIFI_MODE)
  {
    if(firstBootF == false){
      tft.deleteSprite();
      delay(100);
    }
    setTFTedit(TFT_RUN_MODE);
    ui.begin( screen, 16, 2, 0);//タッチキャリブレーションをしない
    screen.fillScreen(TFT_BLACK);
    drawUI();//UIを表示

    ui.createUIs(2); // 最大2つのUIパネルを使用
    createPanels(0, 0); // UIコンテナ0にID０（プリセットUI）を生成
    createPanels(1, select_ui_id); // UIコンテナ1（カスタムUI）にID１～を生成
    //     while(!createChannels()){};

    sprite88_0.setPsram(false );
    sprite88_0.setColorDepth(16);//子スプライトの色深度
    sprite88_0.createSprite(8, 8);//ゲーム画面用スプライトメモリ確保

    sprite64.setPsram(false );
    sprite64.setColorDepth(16);//子スプライトの色深度
    sprite64.createSprite(PNG_SPRITE_WIDTH, PNG_SPRITE_HEIGHT);//ゲーム画面用スプライトメモリ確保//wroomだと64*128だとメモリオーバーしちゃう問題を色番号配列にして回避した

    sprite64.drawPngFile(SPIFFS, "/init/initspr.png", 0, 0);//一時展開する

    sprite64cnos_vector.clear();//初期化処理

    for(int y = 0; y < PNG_SPRITE_HEIGHT; y++) {
        for(int x = 0; x < PNG_SPRITE_WIDTH; x++) {
          if(x%2 == 0){
            uint8_t pixel_data = (readpixel(x, y) << 4) | (readpixel(x + 1, y) & 0b00001111);
            sprite64cnos_vector.push_back(pixel_data);
          }
        }
    }

    //破棄
    sprite64.deleteSprite();

    sprite88_roi.setPsram(false );
    sprite88_roi.setColorDepth(16);//子スプライトの色深度
    sprite88_roi.createSprite(8, 8);//ゲーム画面用スプライトメモリ確保

    sprite11_roi.setPsram(false );
    sprite11_roi.setColorDepth(16);//子スプライトの色深度
    sprite11_roi.createSprite(1, 1);//ゲーム画面用スプライトメモリ確保

    if(firstBootF == true)
    {
      game = nextGameObject(&appfileName, gameState, mapFileName);//ホームゲームを立ち上げる（オブジェクト生成している）
      game->init();//（オブジェクト生成している）
    }

    frame=0;

    // editor.initEditor(tft, EDITOR_ROWS, EDITOR_COLS);
    // editor.initEditor(tft);
    // editor.readFile(SPIFFS, "/init/chat/m.txt");
    // editor.editorOpen(SPIFFS, "/init/chat/m.txt");
    // editor.editorSetStatusMessage("Press ESCAPE to save file");

    // ESP-NOW初期化
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() == ESP_OK) {
      tft.println("ESPNow Init Success");
    } else {
      tft.println("ESPNow Init Failed");
      ESP.restart();
    }

    // マルチキャスト用Slave登録
    memset(&slave, 0, sizeof(slave));
    for (int i = 0; i < 6; ++i) {
      slave.peer_addr[i] = (uint8_t)0xff;
    }
    
    esp_err_t addStatus = esp_now_add_peer(&slave);
    if (addStatus == ESP_OK) {
      // Pair success
      tft.println("Pair success");
    }
    // ESP-NOWコールバック登録
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    
  }else if(isEditMode == TFT_SOUND_MODE){

    tft.setFont(&fonts::Font0);
    tft.setTextSize(1);//サイズ

    if(firstBootF == false){
      tft.deleteSprite();
      delay(100);
    }

    setTFTedit(TFT_SOUND_MODE);

    ui.begin( screen, 16, 2, 0);//タッチキャリブレーションをしない
    screen.fillScreen(TFT_BLACK);

    // drawUI();//UIを表示

    ui.createUIs(2); // 最大2つのUIパネルを使用
    // ui.deletePanels(0);
    createPanels(0, 0); // UIコンテナ0にID０（プリセットUI）を生成
    // createPanels(1, select_ui_id); // UIコンテナ1（カスタムUI）にID１～を生成
    createPanels(1, TOUCH_SLIDER_MODE);

    frame=0;
  }
  else if(isEditMode == TFT_MUSICPLAY_MODE)
  {
    if(firstBootF == false){
      tft.deleteSprite();
      delay(100);
    }

    setTFTedit(TFT_RUN_MODE);

    ui.begin( screen, 16, 2, 0);//タッチキャリブレーションをしない
    screen.fillScreen(TFT_BLACK);
    drawUI();//UIを表示

    ui.createUIs(2); // 最大2つのUIパネルを使用
    createPanels(0, 0); // UIコンテナ0にID０（プリセットUI）を生成
    createPanels(1, select_ui_id); // UIコンテナ1（カスタムUI）にID１～を生成

    ui.setSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, 0.3,0.5);

    sprite64.setPsram(false );
    sprite64.setColorDepth(16);//子スプライトの色深度
    sprite64.createSprite(PNG_SPRITE_WIDTH, PNG_SPRITE_HEIGHT);//ゲーム画面用スプライトメモリ確保//wroomだと64*128だとメモリオーバーしちゃう問題を色番号配列にして回避した

    sprite64.drawPngFile(SPIFFS, "/init/initspr.png", 0, 0);//一時展開する
    sprite64cnos_vector.clear();//初期化処理

    for(int y = 0; y < PNG_SPRITE_HEIGHT; y++) {
        for(int x = 0; x < PNG_SPRITE_WIDTH; x++) {
          if(x%2 == 0){
            uint8_t pixel_data = (readpixel(x, y) << 4) | (readpixel(x + 1, y) & 0b00001111);
            sprite64cnos_vector.push_back(pixel_data);
          }
        }
    }

    //破棄
    sprite64.deleteSprite();

    sprite88_roi.setPsram(false );
    sprite88_roi.setColorDepth(16);//子スプライトの色深度
    sprite88_roi.createSprite(8, 8);//ゲーム画面用スプライトメモリ確保

    sprite11_roi.setPsram(false );
    sprite11_roi.setColorDepth(16);//子スプライトの色深度
    sprite11_roi.createSprite(1, 1);//ゲーム画面用スプライトメモリ確保

    if(firstBootF == true)
    {
      game = nextGameObject(&appfileName, gameState, mapFileName);//ホームゲームを立ち上げる（オブジェクト生成している）
      game->init();//（オブジェクト生成している）
    }
    frame=0;
  }

  savedAppfileName = appfileName;//起動したゲームのパスを取得しておく
  firstBootF = false;

  xSemaphoreGive(nextFrameSemaphore);
}

String c = "";

void loop()
{

      

  // 次のフレーム開始を許可
  // drawWaitF = true;
  // while(!drawWaitF)
  // {
  //   if (xSemaphoreTake(nextFrameSemaphore, portMAX_DELAY))
  //   {
  //     delay(1);
  //   }
  //   delay(1);
  // }

  // delay(100);

  // ui.setConstantGetF(true);//trueだとタッチポイントのボタンIDを連続取得するモード
  // ui.update(screen); // タッチイベントを取るので、LGFXが基底クラスでないといけない
  ui.update2(screen); // タッチイベントを取るので、LGFXが基底クラスでないといけない
  // pressedBtnID = 3;

  if (ui.getEvent() != NO_EVENT)
  {
    uint16_t clist2[3] = {TFT_RED, TFT_GREEN, TFT_BLUE};
    uint8_t cNo = 0;
    uint8_t addx = 50;
    uint8_t addy = 16;

    // Serial.println(select_ui_id);

    // ui.panels[0]がカスタムパネル　ui.panels[1]がカラーパネル
    // if(select_ui_id == 0){//ボタンIDの取得
    if (ui.getEvent() != NO_EVENT)
    { // 何かイベントがあれば
      if (ui.getEvent() == TOUCH)
      { // TOUCHの時だけ
        if (ui.getTouchBtnID() == 39)//ret
        {
          pressedBtnID = 11;
        }

        if (select_ui_id == TOUCH_FLICK_MODE)
        {
          if (ui.getTouchBtnID() == 34){ui.flickShift();}
          else if (ui.getTouchBtnID() == 35){ui.kanaShift();}
          else if (ui.getTouchBtnID() == 36){
            // chatstr = ui.delChar(chatstr);
            String stdStr = ui.delChar(chatstr);
            chatstr = stdStr;
            // Serial.println(chatstr);        
          }
          else if(ui.getTouchBtnID() >= 20 && ui.getTouchBtnID() < 32)
          {
            if(preFlickBtnID != ui.getTouchBtnID()){
              ui.kanaShift(0);//ボタンを切り替えたら濁点にならないように０リセット
              // Serial.print(ui.getTouchBtnID());
              // Serial.println(preFlickBtnID);
            }
          }

          if (ui.getTouchBtnID() >= PRESET_BTN_NUM && ui.getTouchBtnID() < PRESET_BTN_NUM+12)
          {
            preFlickBtnID = ui.getTouchBtnID();//フリックボタンの値を保存しておく
            ui.readFlickDataline(PRESET_BTN_NUM, UI_NO_CUSTOM, preFlickBtnID);
          }
          else if (ui.getTouchBtnID() >= PRESET_BTN_NUM+12 && ui.getTouchBtnID() < PRESET_BTN_NUM+20)
          {
            ui.readFlickDataline(PRESET_BTN_NUM, UI_NO_CUSTOM, preFlickBtnID);//シフトの時は直前に押したフリックボタンの値を使う
          }
          ui.drawFlicks(UI_NO_CUSTOM, screen);
        } 
      }
      if (ui.getEvent() == MOVE)
      {
        if (ui.getTouchBtnID() == -1)
        {
          pressedBtnID = -1;
        }
        else
        {
          if (select_ui_id == TOUCH_SLIDER_MODE)
          {
            pressedBtnID = ui.getTouchBtnID(); // ボタンIDを取得
          }

          if (select_ui_id == TOUCH_FLICK_MODE)
          {

            //show touchEvent
            // tft.fillRect(120-20, addy+40-20, 60, 60, TFT_BLACK);//塗りなおし
            // tft.fillRect(120, addy+40, 20, 20, TFT_GREEN);//中心

            // screen.drawLine(
            //   ui.getStartPos().x,
            //   ui.getStartPos().y,
            //   ui.getPos().x,
            //   ui.getPos().y,
            //   TFT_RED);

            screen.drawLine(0, 158, 240, 158, TFT_BLACK);

            screen.drawLine((ui.getVecNo())%5 * 48, 158, (ui.getVecNo())%5 * 48+48, 158, TFT_ORANGE);

            Serial.println(ui.getVecNo());
          }

        }
      }

      // スライダ値の取得
      if (select_ui_id == TOUCH_SLIDER_MODE)
      {
        
        ui.drawSliders(UI_NO_CUSTOM, screen); // drawしながら取得している
        if(ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, X_VAL)>=0&&ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, X_VAL)<=1){

          // Serial.println("A:"+String(ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, X_VAL)));
          // Serial.println("B:"+String(ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_1, X_VAL)));

          tickSpeed = floor(ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_0, X_VAL)*16);
          tickTime = tickSpeed*25 + 10;
          effectVal = ui.getSliderVal(UI_NO_CUSTOM, 1, SLIDER_NO_1, X_VAL)*16;

          // if (ui.getSliderVal(UI_NO_CUSTOM, 0, SLIDER_NO_0, X_VAL) > 0.6)
          // {
          //   pressedBtnID = 2;
          // }
          // if (ui.getSliderVal(UI_NO_CUSTOM, 0, SLIDER_NO_0, X_VAL) < 0.4)
          // {
          //   pressedBtnID = 1;
          // }
          // if (ui.getSliderVal(UI_NO_CUSTOM, 0, SLIDER_NO_0, Y_VAL) > 0.6)
          // {
          //   pressedBtnID = 4;
          // }
          // if (ui.getSliderVal(UI_NO_CUSTOM, 0, SLIDER_NO_0, Y_VAL) < 0.4)
          // {
          //   pressedBtnID = 3;
          // }
        }
      }
      
      if (select_ui_id == TOUCH_FLICK_MODE)
      {

      }
      
      if (select_ui_id == TOUCH_BTN_MODE)
      {
        // ui.drawSliders(UI_NO_CUSTOM, screen); // drawしながら取得している
        
        if (ui.getTouchBtnID() >= PRESET_BTN_NUM && ui.getTouchBtnID() < PRESET_BTN_NUM+25)
        {
          pressedBtnID = convUiId[ui.getTouchBtnID() - PRESET_BTN_NUM];
        }
        
      }

      if (ui.getEvent() == RELEASE)
      { // RELEASEの時だけ
        // ui.setBtnID(-1);//タッチボタンIDをリセット
        // pressedBtnID = ui.getTouchBtnID()+12;//12個分の物理ボタンをタッチボタンIDに足す

        // 17~19でUIパネルを切り替える
        if (ui.getTouchBtnID() >= 17 && ui.getTouchBtnID() < PRESET_BTN_NUM)
        {
          select_ui_id = ui.getTouchBtnID() - 16;
          // select_ui_id += 1;//プリセットUI＝０を更新しないように＋１しておく
          // プリセットパネル
          //  ui.deletePanels(0);//UIコンテナ0内のパネを削除して
          //  createPanels(0, select_ui_id);//UIコンテナ0に指定したUIを生成しなおす
          // カスタムパネル
          ui.deletePanels(UI_NO_CUSTOM, PRESET_BTN_NUM);            // UIコンテナ1内のパネを削除して,ボタン番号をプリセットの数でリセット
          createPanels(UI_NO_CUSTOM, select_ui_id); // UIコンテナ１に指定したUIを生成しなおす
        }
        if(select_ui_id == TOUCH_BTN_MODE||select_ui_id == TOUCH_SLIDER_MODE)
        {
          ui.drawBtns(UI_NO_CUSTOM, screen);//ボタンとボタン番号塗り替える処理
        }
        else if (select_ui_id == TOUCH_FLICK_MODE)
        {
          if (ui.getTouchBtnID()==38){//clr
            chatstr = "";
            Serial.println("");
          }
          
          // if (ui.getTouchBtnID() >= PRESET_BTN_NUM && ui.getTouchBtnID() < PRESET_BTN_NUM+12)
          // {
          //   preFlickBtnID = ui.getTouchBtnID();//フリックボタンの値を保存しておく
          //   ui.readFlickDataline(PRESET_BTN_NUM, UI_NO_CUSTOM, preFlickBtnID);
          // }
          // else if (ui.getTouchBtnID() >= PRESET_BTN_NUM+12 && ui.getTouchBtnID() < PRESET_BTN_NUM+20)
          // {
          //   ui.readFlickDataline(PRESET_BTN_NUM, UI_NO_CUSTOM, preFlickBtnID);//シフトの時は直前に押したフリックボタンの値を使う
          // }
          // else

          if (ui.getTouchBtnID() >= PRESET_BTN_NUM && ui.getTouchBtnID() < PRESET_BTN_NUM+12)
          {
            // c = ui.getFlickChar(0);
            c = ui.getFlickChar(ui.getVecNo());

            // if( ui.getEventBit(RIGHT_FLICK))c = ui.getFlickChar(4);
            // if( ui.getEventBit(U_R_FLICK) ) c = ui.getFlickChar(3);
            // if( ui.getEventBit(UP_FLICK) )  c = ui.getFlickChar(2);
            // if( ui.getEventBit(L_U_FLICK) ) c = ui.getFlickChar(1);
            // if( ui.getEventBit(LEFT_FLICK) )c = ui.getFlickChar(0);
            // if( ui.getEventBit(D_L_FLICK) );
            // if( ui.getEventBit(DOWN_FLICK) );
            // if( ui.getEventBit(R_D_FLICK) );
            chatstr += c;
            Serial.print(c);    
          }

          if (ui.getTouchBtnID() >= PRESET_BTN_NUM+20 && ui.getTouchBtnID() <= PRESET_BTN_NUM+24)
          {
            c = ui.getFlickChar(1);
            // String c = ui.getFlickChar(PRESET_BTN_NUM, UI_NO_CUSTOM, ui.getTouchBtnID());
            chatstr += c;
            Serial.print(c);            
          } 

          //入力画面を表示
          tft.fillScreen(TFT_BLACK);
          tft.setTextSize(1);
          tft.setFont(&lgfxJapanGothicP_16);//日本語可
          tft.setCursor(0,20);
          tft.print(chatstr);

          // ui.drawFlicks(UI_NO_CUSTOM, screen);
        }

        pressedBtnID = -1; // リセット
        for(int i = 0; i < ui.getAllBtnNum(); i ++){//ボタンの強制リセット
          buttonState[i] = 0;
        }
      }
    }

  //show touchEvent
    
  // for(int i = 0; i<32; i++){
  //   if(ui.getEventBit(i)){
  //     tft.fillRect(0, addy + 8*i, 8,8, clist2[cNo]);
  //   }
  // }

  // // tft.setFont(&fonts::Font0);
  // tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // tft.setCursor(8, addy + 8 * DRAG);tft.print("DRAG");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * TAPPED);tft.print("TAPPED");//イベント定義を描画
  //   // tft.fillRect(30+addx, addy + 8 * TAPPED,10,10,TFT_RED);
  // tft.setCursor(8, addy + 8 * WTAPPED);tft.print("WTAPPED");//イベント定義を描画
  //   // tft.fillRect(30+addx, addy + 8 * WTAPPED,10,10,TFT_YELLOW);
  // tft.setCursor(8, addy + 8 * FLICK);tft.print("FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * FLICKED);tft.print("FLICKED");//イベント定義を描画

  // tft.setCursor(8, addy + 8 * TOUCH);tft.print("TOUCH");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * WAIT);tft.print("WAIT");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * MOVE);tft.print("MOVE");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * RELEASE);tft.print("RELEASE");//イベント定義を描画

  // tft.setCursor(8, addy + 8 * RIGHT_FLICK);tft.print("RIGHT_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * U_R_FLICK);tft.print("U_R_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * UP_FLICK);tft.print("UP_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * L_U_FLICK);tft.print("L_U_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * LEFT_FLICK);tft.print("LEFT_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * D_L_FLICK);tft.print("D_L_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * DOWN_FLICK);tft.print("DOWN_FLICK");//イベント定義を描画
  // tft.setCursor(8, addy + 8 * R_D_FLICK);tft.print("R_D_FLICK");//イベント定義を描画


  //   tft.fillRect(8+addx, addy + 8 * TAPPED,10,10,TFT_BLUE);
  //   tft.fillRect(8+addx, addy + 8 * WTAPPED,10,10,TFT_YELLOW);

  //   if( ui.getEventBit(TAPPED) )tft.fillRect(100, addy, 100,100, TFT_BLUE);
  //   else if( ui.getEventBit(WTAPPED) )tft.fillRect(100, addy, 100,100, TFT_YELLOW);

  //   tft.fillRect(120-20, addy+40-20, 60, 60, TFT_BLACK);//塗りなおし
  //   tft.fillRect(120, addy+40, 20, 20, TFT_GREEN);//中心

  //   if( ui.getEventBit(RIGHT_FLICK) )tft.fillRect(120+20, addy+40,    20, 20, TFT_RED);
  //   if( ui.getEventBit(U_R_FLICK) )  tft.fillRect(120+20, addy+40-20, 20, 20, TFT_RED);
  //   if( ui.getEventBit(UP_FLICK) )   tft.fillRect(120,    addy+40-20, 20, 20, TFT_RED);
  //   if( ui.getEventBit(L_U_FLICK) )  tft.fillRect(120-20, addy+40-20, 20, 20, TFT_RED);
  //   if( ui.getEventBit(LEFT_FLICK) ) tft.fillRect(120-20, addy+40,    20, 20, TFT_RED);
  //   if( ui.getEventBit(D_L_FLICK) )  tft.fillRect(120-20, addy+40+20, 20, 20, TFT_RED);
  //   if( ui.getEventBit(DOWN_FLICK) ) tft.fillRect(120,    addy+40+20, 20, 20, TFT_RED);
  //   if( ui.getEventBit(R_D_FLICK) )  tft.fillRect(120+20, addy+40+20, 20, 20, TFT_RED);

  //   for(int i = 0; i<3; i++){
  //     for(int j = 0; j<3; j++){
  //       tft.drawRect(100+i*20, addy+20+j*20, 20, 20, TFT_WHITE);//枠線
  //     }
  //   }

  // if( ui.getEvent() == RELEASE ){
  //   cNo++;
  //   if(cNo==3)cNo = 0;
  //   ui.resetEventBits();
    
  //   for(int i = 0; i<32; i++){
  //     tft.fillRect(0, addy + 8*i, 8,8, TFT_BLACK);
  //     tft.drawRect(0, addy + 8*i, 8,8, TFT_WHITE);
  //   }
  // }


  }

  uint32_t now = millis();
  uint32_t remainTime = (now >= preTime) ? (now - preTime) : (UINT32_MAX - preTime + now);
  preTime = now;

  if( isEditMode == TFT_RUN_MODE )
  {
    //ゲーム内のprint時の文字設定をしておく
    
    // tft.setTextSize(1);//サイズ
    // tft.setFont(&lgfxJapanGothicP_8);//日本語可
    // tft.setCursor(0, 0);//位置
    // tft.setTextWrap(true);

    // == tune task ==
    // tunes.run();

    // == game task ==
    game->run(remainTime);
    // mode = game->run(remainTime);//exitは1が返ってくる　mode=１ 次のゲームを起動

    // //ESCボタンで強制終了
    // if (pressedBtnID == 0)
    // { // reload

    //   // editor.setCursorConfig(0,0,0);//カーソルの位置を強制リセット保存
    //   // delay(50);

    //   ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
    //   appfileName = "/init/main.lua";
      
    //   firstLoopF = true;
    //   toneflag = false;
    //   sfxflag = false;
    //   musicflag = false;
    //   // wfr.close();//起動時に展開したワールドマップ用のファイルを閉じる

    //   mode = 1;//exit
    // }

    // if (pressedBtnID == 9999)
    // { // reload
    //   ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
    //   mode = 1;//exit
    //   pressedBtnID = -1;
    // }

    // if(mode != 0){ // exit request//次のゲームを立ち上げるフラグ値、「modeが１＝次のゲームを起動」であれば
    //   // tunes.pause();
    //   game->pause();
    //   // ui.clearAddBtns();//個別のゲーム内で追加したタッチボタンを消去する
    //   free(game);
    //   firstLoopF = true;
    //   toneflag = false;
    //   sfxflag = false;
    //   musicflag = false;
    //   // txtName = appfileName;
    //   game = nextGameObject(&appfileName, gameState, mapFileName);//ファイルの種類を判別して適したゲームオブジェクトを生成
    //   game->init();//resume()（再開処理）を呼び出し、ゲームで利用する関数などを準備
    //   // tunes.resume();
      
    // }

    // ui.showTouchEventInfo( tft, 0, 100 );//タッチイベントを視覚化する
    ui.showInfo( tft, 0, 0+8 );//ボタン情報、フレームレート情報などを表示します。
    
    
    
    // spriteMap.drawPngFile(SPIFFS, "/init/param/map/0.png", 0, 0); 
    // spriteMap.fillScreen(TFT_BLUE);

    // spriteMap.pushSprite(&tft, 0,0);

    // if(outputMode == WIDE_MODE){
      // tft.pushAffine(matrix_game);//ゲーム画面を最終描画する
      if(toolNo != 0){
        if(toolNo==1){//カラーパレット
          for(int j = 0; j<8; j++){
            for(int i = 0; i<2; i++){
              screen.fillRect(i*16,j*16,16,16,gethaco3Col(j*2+i));
            }
          }
        }

        toolNo = 0;
      }

    // tft.fillRect(channels->gettick()*4,0,4,128,TFT_YELLOW);
    // tft.fillRect(tick*4,0,4,4,TFT_YELLOW);

    Serial.println("");
    //Affineを使わない書き方
    tft.setPivot(0, 0);
    tft.pushRotateZoom(&screen, 40, 0, 0, 1, 1);

    // }
    // else if(outputMode == FAST_MODE){
    //   tft.pushSprite(&screen,TFT_OFFSET_X,TFT_OFFSET_Y);//ゲーム画面を小さく高速描画する
    // }

    // if(pressedBtnID == 5){//PAGEUP//キーボードからエディタ再起動
    //   restart(appfileName, 1);//appmodeでリスタートかけるので、いらないかも
    // }

  }
  else if(isEditMode == TFT_EDIT_MODE)
  {
    // editor.editorRefreshScreen(tft);

    // float codeunit = 128.0/float(editor.getNumRows());
    // float codelen = codeunit*10;
    
    // float curpos = codeunit*editor.getCy();
    // float codepos = codeunit*(editor.getCy() - editor.getScreenRow());
    
    // tft.fillRect(156,0, 4,128, HACO3_C5);//コードの全体の長さを表示
    // tft.fillRect(156,int(codepos), 4,codelen, HACO3_C6);//コードの位置と範囲を表示
    // if(codeunit>=1){tft.fillRect(155, int(curpos), 4, codeunit, HACO3_C8);}//コードの位置と範囲を表示
    // else{tft.fillRect(155, int(curpos), 4, 1, HACO3_C8);}//１ピクセル未満の時は見えなくなるので１に
    
    // tft.pushSprite(&screen, 60, 0);
    
    // if(pressedBtnID == 0)//ESC
    // {
    //   editor.setCursorConfig(0,0,0);//カーソルの位置を保存
    //   delay(50);
    //   restart("/init/main.lua", 0);
    // }

    // if(pressedBtnID == 6){//PAGEDOWN
    //   editor.editorSave(SPIFFS);//SPIFFSに保存
    //   delay(100);//ちょっと待つ
    //   reboot(appfileName, TFT_RUN_MODE);//現状rebootしないと初期化が完全にできない
    //   // restart(appfileName, 0);//初期化がうまくできない（スプライトなど）
    //   // broadchat();//ファイルの中身をブロードキャスト送信する（ファイルは消えない）
    // }

  }
  else if(isEditMode == TFT_WIFI_MODE)
  {
    // == tune task ==
    // tunes.run();
    // == game task ==

    // mode = game->run(remainTime);//exitは1が返ってくる　mode=１ 次のゲームを起動

    //ESCボタンで強制終了
    if (pressedBtnID == 0)
    { 
      ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
      appfileName = "/init/main.lua";
      
      firstLoopF = true;
      toneflag = false;
      sfxflag = false;
      musicflag = false;
      // wfr.close();//起動時に展開したワールドマップ用のファイルを閉じる
      mode = 1;//exit
    }

    if (pressedBtnID == 9999)
    { // reload
      ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
      mode = 1;//exit
      pressedBtnID = -1;
    }

    if(mode != 0){ // exit request//次のゲームを立ち上げるフラグ値、「modeが１＝次のゲームを起動」であれば
      // tunes.pause();
      game->pause();
      // ui.clearAddBtns();//個別のゲーム内で追加したタッチボタンを消去する
      free(game);
      firstLoopF = true;
      toneflag = false;
      sfxflag = false;
      musicflag = false;
      // txtName = appfileName;
      game = nextGameObject(&appfileName, gameState, mapFileName);//ファイルの種類を判別して適したゲームオブジェクトを生成
      game->init();//resume()（再開処理）を呼び出し、ゲームで利用する関数などを準備
      // tunes.resume();
      
    }
    

    // ui.showTouchEventInfo( tft, 0, 100 );//タッチイベントを視覚化する
    ui.showInfo( tft, 0, 0+8 );//ボタン情報、フレームレート情報などを表示します。


    //Affineを使わない書き方
    tft.setPivot(0, 0);
    tft.pushRotateZoom(&screen, 40, 0, 0, 1, 1);

    tft.setTextSize(1);//サイズ
    tft.setFont(&lgfxJapanGothicP_8);//日本語可
    // tft.setCursor(0, 0);//位置
    // tft.setTextWrap(true);
    // tft.setTextScroll(true);



    if(pressedBtnID == 11){
      // editor.editorSave(SPIFFS);//SPIFFSに保存
      // delay(100);//ちょっと待つ
      // chatstr = "test text";
      broadchat(chatstr);
      delay(500);
      
    }

    // tft.pushSprite(&screen,40,0);
  }
  else if( isEditMode == TFT_SOUND_MODE )
  {
    //ゲーム内のprint時の文字設定をしておく
    
    
    tft.setFont(&fonts::Font0);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);//サイズ
    // tft.setFont(&lgfxJapanGothicP_8);//日本語可
    // tft.setCursor(0, 0);//位置
    // tft.setTextWrap(true);

    if(ui.getTouchBtnID()>=20&&ui.getTouchBtnID()<28){//getTouchBtnIDはトグル的な扱いの時のみ使える（押したらそのままの値を保持する）
      instrument = ui.getTouchBtnID()-20;
    }

    if(ui.getTouchBtnID()>=36&&ui.getTouchBtnID()<44){//チャンネル0~7
      targetChannelNo = ui.getTouchBtnID()-36;
    }
    
    if(ui.getTouchBtnID()>=44&&ui.getTouchBtnID()<52){//サウンドエフェクト0~7
      gEfectNo = ui.getTouchBtnID()-44;
    }

    // == tune task ==
    // tunes.run();

    // == game task ==
    // tft.fillScreen(gethaco3Col(1));
    tft.fillRect(0,  0, 128, 96, gethaco3Col(1));
    tft.fillRect(0, 96, 128, 32, gethaco3Col(6));

    showMusicInfo();
    controlMusicVisual();

    

    ui.showInfo( tft, 0, 0+8 );//ボタン情報、フレームレート情報などを表示します。

    // for(size_t n=0; n<4; n++){
    //   Serial.print(channels->patterns[patternNo][n]);
    // }

    // Serial.println("");
    //Affineを使わない書き方

    // tft.drawPixel(128, channels->getbuf()[0], TFT_WHITE);

    // tft.drawPixel(128, channels->getbuf()[0], TFT_WHITE);

    // Serial.println(channels->getChannelVolume(0));

    // tft.fillRect(128, channels->getChannelVolume(0), 4,4,gethaco3Col(10));

    tft.setPivot(0, 0);
    tft.pushRotateZoom(&screen, 40, 0, 0, 1, 1);

    // if(i==0){
    //   patternNo++;
    //   if(patternNo==32)patternNo=0;
    // }

  }
  else if( isEditMode == TFT_MUSICPLAY_MODE )
  {
    //ゲーム画面を表示
    // == tune task ==
    // tunes.run();

    // == game task ==
    mode = game->run(remainTime);//exitは1が返ってくる　mode=１ 次のゲームを起動
    

    //ESCボタンで強制終了
    // if (pressedBtnID == 0)
    // { // reload

    //   // editor.setCursorConfig(0,0,0);//カーソルの位置を強制リセット保存
    //   // delay(50);

    //   ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
    //   appfileName = "/init/main.lua";
      
    //   firstLoopF = true;
    //   toneflag = false;
    //   sfxflag = false;
    //   musicflag = false;
    //   // wfr.close();//起動時に展開したワールドマップ用のファイルを閉じる

    //   mode = 1;//exit
    // }

    // if (pressedBtnID == 9999)
    // { // reload
    //   ui.setConstantGetF(false);//初期化処理 タッチポイントの常時取得を切る
    //   mode = 1;//exit
    //   pressedBtnID = -1;
    // }

    // if(mode != 0){ // exit request//次のゲームを立ち上げるフラグ値、「modeが１＝次のゲームを起動」であれば
    //   // tunes.pause();
    //   game->pause();
    //   // ui.clearAddBtns();//個別のゲーム内で追加したタッチボタンを消去する
    //   free(game);
    //   firstLoopF = true;
    //   toneflag = false;
    //   sfxflag = false;
    //   musicflag = false;
    //   // txtName = appfileName;
    //   game = nextGameObject(&appfileName, gameState, mapFileName);//ファイルの種類を判別して適したゲームオブジェクトを生成
    //   game->init();//resume()（再開処理）を呼び出し、ゲームで利用する関数などを準備
    //   // tunes.resume();
      
    // }
    //ゲーム内のprint時の文字設定をしておく

    // tft.setFont(&fonts::Font0);
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.setTextSize(1);//サイズ


    //音楽プレイ確認用画面を表示
    // tft.fillRect(0,  0, 128, 96, gethaco3Col(1));
    // tft.fillRect(0, 96, 128, 32, gethaco3Col(6));
    // showMusicVisual();

    // showMusicInfo();

    // tft.fillRect(0, writeDispRow*8, 8, 8, TFT_YELLOW);

    // tft.fillRect(0, targetDispRow*8, 8, 8, TFT_RED);

    ui.showInfo( tft, 0, 0+8 );//ボタン情報、フレームレート情報などを表示します。
    tft.setPivot(0, 0);
    tft.pushRotateZoom(&screen, 40, 0, 0, 1, 1);
    
  }

  frame++;

  // drawWaitF = true;
  
  // while(!drawWaitF){
  //   delay(1);
  // }
  

  if(frame > 18446744073709551615)frame = 0;

  // }
  delay(1);
}