#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include "pin_definitions.h"

extern HardwareSerial AudioSerial;

// DFPlayer Commands
#define CMD_PLAY_TRACK     0x0F
#define CMD_SET_VOLUME     0x06

// Audio Track Indices
#define TRACK_STARTUP      1  // "I am here boss..."
#define TRACK_OBSTACLE     2  // "Obstacle detected, recalculating path..."
#define TRACK_NAV_CONFIRM  3  // "Target received, heading to destination..."
#define TRACK_GOAL_REACHED 4  // "Destination reached, boss."

void sendAudioCommand(uint8_t command, uint8_t dat1, uint8_t dat2) {
  uint8_t buffer[10] = { 0x7E, 0xFF, 0x06, command, 0x00, dat1, dat2, 0x00, 0x00, 0xEF };
  
  uint16_t sum = 0;
  for (int i = 1; i < 7; i++) {
    sum += buffer[i];
  }
  sum = -sum;
  buffer[7] = (uint8_t)(sum >> 8);
  buffer[8] = (uint8_t)(sum & 0xFF);
  
  for (int i = 0; i < 10; i++) {
    AudioSerial.write(buffer[i]);
  }
}

void setVolume(uint8_t volume) {
  volume = constrain(volume, 0, 30); // DFPlayer volume scale: 0-30
  sendAudioCommand(CMD_SET_VOLUME, 0x00, volume);
}

void playTrack(uint8_t trackIndex) {
  sendAudioCommand(CMD_PLAY_TRACK, 0x01, trackIndex);
}

#endif
