#include "Channel.hpp"

// Channel クラスのコンストラクタの実装
Channel::Channel() {
    // ここで初期化処理を行う
}

// Channel クラスのデストラクタの実装
Channel::~Channel() {
    // ここでクリーンアップ処理を行う
}

// void Channel::test();

extern int tick;

// void Channel::setChannel(const ChannelData newData[32])
// {
//     // 配列をコピーしてメンバ変数にセット
//     for (size_t i = 0; i < 32; ++i) {
//         this->notedata[i] = newData[i];
//     }
// }

double calculateFrequency(int pitch, int octave) {
    // A4の周波数（Hz）
    double baseFrequency = 440.0;

    // ドレミの差に基づいて周波数を計算
    double frequency = baseFrequency * pow(2.0, ((pitch - 9) / 12.0 + octave - 4));

    return frequency;
}

extern int tickTime;
void Channel::setChannel( size_t onoffF, size_t loopStart, size_t loopEnd, size_t toneNo, size_t pitch, size_t octave, size_t sfxno, size_t volume, size_t effectNo, size_t tickNo)
{
    for (int pitch = 0; pitch < numPitches; ++pitch) {
        for (int octave = 0; octave < numOctaves; ++octave) {
            hzList[pitch][octave] = calculateFrequency(pitch, octave);
        }
    }
  

    // メンバ変数の初期化
    this->notedata[tickNo].onoffF = onoffF;
    this->notedata[tickNo].loopStart = loopStart;
    this->notedata[tickNo].loopEnd = loopEnd;
    this->notedata[tickNo].toneNo = toneNo;
    this->notedata[tickNo].pitch = hzList[pitch][octave];
    this->notedata[tickNo].octave = octave;
    this->notedata[tickNo].sfxno = sfxno;
    this->notedata[tickNo].volume = volume;
    this->notedata[tickNo].effectNo = effectNo;
    this->notedata[tickNo].looplen = loopEnd - loopStart; 
}

bool Channel::note(size_t channel, size_t tick){

  // if(tick%32){
      tone(this->notedata[tick%32].pitch, tickTime, 1);
       return true;
    // }

  // if(tickNo == tick%looplen){
  //   tone(pitch*100, 1000, channel);
  //   // delay(1000);
  //   // tick++;
  //   return true;
  // }
}

    