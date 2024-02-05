#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Speaker_Class.hpp"
// #include "CRingBuffur.hpp"

#define numPitches 12  // ドレミファソラシド（12音）// + レ（13音）
#define numOctaves 16

#define PATTERN_NUM 64//64パタン用意するとメモリが足らない
#define TONE_NUM 32
#define CHANNEL_NUM 8//4チャネルにするとうまくいかない


// 新しい構造体
struct ChannelData {
    bool onoffF;
    size_t loopStart;
    size_t loopEnd;
    size_t hz;
    size_t pitch;
    size_t octave;
    size_t sfxno;
    size_t instrument;
    size_t volume;
    size_t effectNo;
    size_t tickNo;
    size_t looplen;
};

class Channel : public Speaker_Class {

public:
  ChannelData notedata[CHANNEL_NUM][TONE_NUM*2];  // 構造体をメンバ変数として持つ（バックバッファ2パタン8チャンネル分しかデータは持たない）
  size_t patterns[PATTERN_NUM][CHANNEL_NUM];//３２パターン（小節）8チャンネルにSfxNo0~63を３２ノート割り付けるための配列
  // static size_t* sound_buf32;
  //  size_t patterns[16][4];
  // size_t patterns[64][4];//ほんとは６４小節使いたい

  // 二次元浮動小数点数配列の宣言
  double hzList[numPitches][numOctaves];

  // CRingBuffur ringbuf;
  

  // コンストラクタ
  Channel();
  // デストラクタ
  virtual ~Channel();
  void setTones(size_t onoffF, size_t loopStart, size_t loopEnd, size_t instrument, size_t pitch, size_t octave, size_t sfxno, size_t volume, size_t effectNo, size_t tickNo, size_t _chno, size_t buffAreaNo);
  void setPatterns(size_t _patternNo, size_t _ch0, size_t _ch1, size_t _ch2, size_t _ch3, size_t _ch4, size_t _ch5, size_t _ch6, size_t _ch7);
  void setPatterns(size_t _patternNo, size_t _ch, size_t _patternID);
  size_t getPatternID(size_t _patternNo, size_t _chno);
  bool note(size_t channelno, size_t tick, size_t _patternNo);
  size_t gettick();
  void resetTones(size_t tickNo, size_t _sfxno, size_t _pitch, size_t _octave, size_t _volume, size_t _instrument, size_t _effectNo, size_t _buffAreaNo);

    // size_t* getSoundBuffer() const {
    //     return *sound_buf32;
    // }
  // size_t* getbuf(){ return sound_buf32_b; }//2147483647
};

#endif // CHANNEL_HPP
