//
//  WavFile.hpp
//  PG1
//
//  Created by Andrej Karadzic on 11/5/15.
//  Copyright © 2015 Andrej Karadzic. All rights reserved.
//

#define byte unsigned char

#ifndef WavFile_hpp
#define WavFile_hpp

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>

struct wavheader {
  char ChunkID[4];
  int ChunkSize;
  char Format[4];

  char Subchunk1ID[4];
  int Subchunk1Size;
  short AudioFormat; // 1 for PCM format
  short NumChannels; // channels
  int SampleRate;    // sampling frequency
  int ByteRate;
  short BlockAlign;
  short BitsPerSample;

  char Subchunk2ID[4]; // should always contain "data"
  int Subchunk2Size;
};

class WavFile {
  wavheader header;
  byte *dataL;
  byte *dataR;
  double *floatingData;
  char *path;

public:
  int numSamples() { return header.Subchunk2Size / 2; }
  double *data() { return floatingData; }

  WavFile(char *p) : path(p) {
    FILE *f = fopen(path, "rb");
    fread(&header, sizeof(header), 1, f);

    dataL = new byte[header.Subchunk2Size / 2];
    dataR = new byte[header.Subchunk2Size / 2];
    floatingData = new double[header.Subchunk2Size / 2];

    for (int i = 0; i < header.Subchunk2Size / 2; i++) {
      fread(&dataL[i], sizeof(dataL[i]), 1, f);
      fread(&dataR[i], sizeof(dataR[i]), 1, f);
      floatingData[i] = (dataL[i] - 128) / 127.0;
    }
  }
};

#endif /* WavFile_hpp */
