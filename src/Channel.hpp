#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Speaker_Class.hpp"


#define numPitches 13  // ドレミファソラシド（12音） + レ（13音）
#define numOctaves 16


// 新しい構造体
struct ChannelData {
    bool onoffF;
    size_t loopStart;
    size_t loopEnd;
    size_t toneNo;
    size_t pitch;
    size_t octave;
    size_t sfxno;
    size_t volume;
    size_t effectNo;
    size_t tickNo;
    size_t looplen;
};


class Channel : public Speaker_Class {
private:

  ChannelData notedata[32];  // 構造体をメンバ変数として持つ

  // 二次元浮動小数点数配列の宣言
  double hzList[numPitches][numOctaves];

public:
    // コンストラクタ
    Channel();

    // デストラクタ
    virtual ~Channel();

    void setChannel(size_t onoffF, size_t loopStart, size_t loopEnd, size_t toneNo, size_t pitch, size_t octave, size_t sfxno, size_t volume, size_t effectNo, size_t tickNo);
    bool note(size_t channel, size_t tick);

};

#endif // CHANNEL_HPP
